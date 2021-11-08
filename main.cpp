#include <atomic>
#include <iostream>
#include <thread>
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <tbb/concurrent_queue.h>
#ifdef __linux__
    #include <safeclib/safe_str_lib.h>
#endif // __linux__

#ifdef _WIN32
    #define NOMINMAX 1
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif // _WIN32

// Messages to send to the GUI thread.
enum EGuiMsg { eGuiMsg_None, eGuiMsg_Log, eGuiMsg_Count};
class GuiMsg
{
private:
    EGuiMsg m_eMsg;
    enum { kiLogSpace = 256 };
    char m_aszLog[kiLogSpace];

public:
    GuiMsg() : m_eMsg(eGuiMsg_None) { m_aszLog[0] = '\0'; }
    GuiMsg(EGuiMsg a_eMsg, char const *a_pszLog)
        : m_eMsg(a_eMsg)
    {
        ::strcpy_s(m_aszLog, kiLogSpace, a_pszLog);
    }
    GuiMsg(GuiMsg const &a_rOther)
        : m_eMsg(a_rOther.m_eMsg)
    {
        ::strcpy_s(m_aszLog, kiLogSpace, a_rOther.m_aszLog);
    }

    EGuiMsg getMsg() const { return m_eMsg; }
    char const *getLog() const { return m_aszLog; }
};

// The GUI thread message queue.
class GuiThreadMessageQueue
{
public:
    GuiThreadMessageQueue(uint32_t a_uiSignalEvent);
    void post(GuiMsg const a_rMsg);
    bool tryPop(GuiMsg &ao_oMsg);

private:
    uint32_t m_uiSignalEvent;
    tbb::concurrent_queue<GuiMsg> m_lstQueuedGuiMessages;
    std::atomic<size_t> m_iQueuedMessages;
};

class Renderer
{
public:
    Renderer(SDL_Window* window,
        GuiThreadMessageQueue &a_rGuiThreadMessageQueue);
    ~Renderer();
    void stop();

    // Messages to send to the render thread.
    enum ERenderMsg { eRenderMsg_None, eRenderMsg_MoveLeft,
        eRenderMsg_MoveRight, eRenderMsg_MoveUp,
        eRenderMsg_MoveDown, eRenderMsg_Count};
    class RendererMsg
    {
    private:
        ERenderMsg m_eMsg;
        float m_fDistance;

    public:
        RendererMsg() : m_eMsg(eRenderMsg_None), m_fDistance(0.0f) {}
        RendererMsg(ERenderMsg a_eMsg, float a_fDistance)
            : m_eMsg(a_eMsg), m_fDistance(a_fDistance) {}
        RendererMsg(RendererMsg const &a_rOther)
            : m_eMsg(a_rOther.m_eMsg), m_fDistance(a_rOther.m_fDistance) {}

        ERenderMsg getMsg() const { return m_eMsg; }
        float getDistance() const { return m_fDistance; }
    };

    void postMessage(RendererMsg const &a_rMsg);

private:
    void renderJob();
    void render();

    SDL_GLContext mContext;
    SDL_Renderer* mRenderer;
    SDL_Window* mWindow;
    std::thread *mThread;
    SDL_mutex *mLock;
    SDL_cond *mCond;
    bool mFirstRun, mKeepRunning;
    tbb::concurrent_queue<RendererMsg> m_lstQueuedRenderMessages;
    float m_fX, m_fY;
    GuiThreadMessageQueue &m_rGuiThreadMessageQueue;
};

GuiThreadMessageQueue::GuiThreadMessageQueue(uint32_t a_uiSignalEvent)
    : m_uiSignalEvent(a_uiSignalEvent), m_iQueuedMessages(0)
{
}


void GuiThreadMessageQueue::post(GuiMsg const a_rMsg)
{
    // Queue this message.
    m_lstQueuedGuiMessages.push(a_rMsg);

    // If the queue was previously empty, signal that it's no longer empty.
    if (++m_iQueuedMessages == 1)
    {
        SDL_Event oEvent;
        oEvent.type = m_uiSignalEvent;
        SDL_PushEvent(&oEvent);
    }
}


bool GuiThreadMessageQueue::tryPop(GuiMsg &ao_oMsg)
{
    if (m_lstQueuedGuiMessages.try_pop(ao_oMsg))
    {
        --m_iQueuedMessages;
        return true;
    }
    else
        return false;
}


Renderer::Renderer(SDL_Window* window,
        GuiThreadMessageQueue &a_rGuiThreadMessageQueue)
    : mWindow(window),
      mRenderer(nullptr),
      mLock(nullptr),
      mCond(nullptr),
      mFirstRun(true),
      mKeepRunning(true),
      m_fX(0.0f),
      m_fY(0.0f),
      m_rGuiThreadMessageQueue(a_rGuiThreadMessageQueue)
{
    mContext = SDL_GL_GetCurrentContext();

    SDL_GL_MakeCurrent(window, nullptr);

    // Create the mutex/condition needed to signal
    // when the renderer has been initialized.
    mLock = SDL_CreateMutex();
    mCond = SDL_CreateCond();

    // Start the rendering thread.
    mThread = new std::thread (&Renderer::renderJob, this);

    // Wait for the renderer to be initialized.
    SDL_LockMutex(mLock);
    while (mFirstRun)
        SDL_CondWait(mCond, mLock);
    SDL_UnlockMutex(mLock);
}


Renderer::~Renderer()
{
    SDL_DestroyCond(mCond);
    SDL_DestroyMutex(mLock);
    delete mThread;
}


void Renderer::stop()
{
    mKeepRunning = false;
    mThread->join();
}


void Renderer::postMessage(RendererMsg const &a_rMsg)
{
    m_lstQueuedRenderMessages.push(a_rMsg);
}


void Renderer::render()
{
    // Get display parameters.
    int iWindowWidth, iWindowHeight;
    SDL_GL_GetDrawableSize(mWindow, &iWindowWidth, &iWindowHeight);
    float fWindowWidth(iWindowWidth), fWindowHeight(iWindowHeight);

    // Process queued messages.
    RendererMsg oMsg;
    while (m_lstQueuedRenderMessages.try_pop(oMsg))
    {
        float fDistance = oMsg.getDistance();
        switch (oMsg.getMsg())
        {
        case eRenderMsg_MoveLeft:
            m_fX = std::max(m_fX - fDistance, 0.0f);
            m_rGuiThreadMessageQueue.post(GuiMsg(eGuiMsg_Log, "Moved left"));
            break;
        case eRenderMsg_MoveRight:
            m_fX = std::min(m_fX + fDistance, fWindowWidth - 100.0f);
            m_rGuiThreadMessageQueue.post(GuiMsg(eGuiMsg_Log, "Moved right"));
            break;
        case eRenderMsg_MoveUp:
            m_fY = std::max(m_fY - fDistance, 0.0f);
            m_rGuiThreadMessageQueue.post(GuiMsg(eGuiMsg_Log, "Moved up"));
            break;
        case eRenderMsg_MoveDown:
            m_fY = std::min(m_fY + fDistance, fWindowHeight - 100.0f);
            m_rGuiThreadMessageQueue.post(GuiMsg(eGuiMsg_Log, "Moved down"));
            break;
        default:
            break;
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glBegin(GL_QUADS);
    glColor3f(1, 0, 0); glVertex3f(m_fX + 0, m_fY + 0, 0);
    glColor3f(1, 1, 0); glVertex3f(m_fX + 100, m_fY + 0, 0);
    glColor3f(1, 0, 1); glVertex3f(m_fX + 100, m_fY + 100, 0);
    glColor3f(1, 1, 1); glVertex3f(m_fX + 0, m_fY + 100, 0);
    glEnd();

    SDL_GL_SwapWindow(mWindow);
}

void Renderer::renderJob()
{
    while (mKeepRunning)
    {
        if (mFirstRun)
        {
            SDL_GL_MakeCurrent(mWindow, mContext);

            mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED);

            // Note that the renderer has been initialized.
            SDL_LockMutex(mLock);
            mFirstRun = false;
            SDL_CondSignal(mCond);
            SDL_UnlockMutex(mLock);
        }
        if (mRenderer != nullptr)
        {
            render();
        }
    }
}

void handleKeys(unsigned char key, int x, int y)
{
    //Toggle quad
    if (key == 'q')
    {
        //gRenderQuad = !gRenderQuad;
    }
}

int main(int argc, char **argv)
{
    //Use OpenGL 2.1
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    // Set up OpenGL default values.
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);

    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

    // Create the OpenGL window.
    SDL_Window *window = SDL_CreateWindow("Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    SDL_GLContext oContext = SDL_GL_CreateContext(window);

    // Set up the drawing environment.
    glClearColor(0, 0, 0, 0);
    glClearDepth(1.0f);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 640, 480, 0, 1, -1);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
    glLoadIdentity();

    // Synchronize buffer-swap with vertical-refresh.
    SDL_GL_SetSwapInterval(1);

    // Register an event for incoming messages.
    uint32_t uiEvent_DelegateCallbacks = SDL_RegisterEvents(1);

    // Prepare to receive events in the GUI thread.
    GuiThreadMessageQueue guiQ(uiEvent_DelegateCallbacks);

    // Create an OpenGL renderer that'll run in a separate thread.
    Renderer r(window, guiQ);

    //Enable text input
    SDL_StartTextInput();

    while (window != nullptr)
    {
        SDL_Event event;

        if (SDL_WaitEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                r.stop();
                SDL_DestroyWindow(window);
                window = nullptr;
                break;

                //Handle keypress with current mouse position
            case SDL_TEXTINPUT:
                {
                    int x = 0, y = 0;
                    SDL_GetMouseState(&x, &y);
                    handleKeys(event.text.text[0], x, y);
                }
                break;

            case SDL_KEYDOWN:
                {
                    SDL_Keycode sym = event.key.keysym.sym;

                    switch(sym)
                    {
                    case SDLK_LEFT:
                        r.postMessage(Renderer::RendererMsg(Renderer::eRenderMsg_MoveLeft, 10.0f));
                        break;
                    case SDLK_RIGHT:
                        r.postMessage(Renderer::RendererMsg(Renderer::eRenderMsg_MoveRight, 10.0f));
                        break;
                    case SDLK_UP:
                        r.postMessage(Renderer::RendererMsg(Renderer::eRenderMsg_MoveUp, 10.0f));
                        break;
                    case SDLK_DOWN:
                        r.postMessage(Renderer::RendererMsg(Renderer::eRenderMsg_MoveDown, 10.0f));
                        break;
                    default:
                        break;
                    }
                }

            default:
                if(event.type == uiEvent_DelegateCallbacks)
                {
                    GuiMsg oGuiMsg;
                    while (guiQ.tryPop(oGuiMsg))
                    {
                        ::fprintf(stderr, "%s\n", oGuiMsg.getLog());
                        ::fflush(stderr);
                    }
                }
            }
        }
    }

    //Disable text input
    SDL_StopTextInput();

    SDL_Quit();

    return 0;
}

#ifdef __MINGW32__

// MinGW expects a WinMain().
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevIns, LPSTR lpszArgument, int iShow)
{
    return main(0, nullptr);
}

#endif // __MINGW32__
