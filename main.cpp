#include <iostream>
#include <thread>
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#ifdef _WIN32
    #define NOMINMAX 1
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif // _WIN32

class Renderer
{
public:
    Renderer(SDL_Window* window);
    ~Renderer();
    void stop();

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
};

Renderer::Renderer(SDL_Window* window)
    : mWindow(window),
      mRenderer(nullptr),
      mLock(nullptr),
      mCond(nullptr),
      mFirstRun(true),
      mKeepRunning(true)
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


void Renderer::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glBegin(GL_QUADS);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0);
    glColor3f(1, 1, 0); glVertex3f(100, 0, 0);
    glColor3f(1, 0, 1); glVertex3f(100, 100, 0);
    glColor3f(1, 1, 1); glVertex3f(0, 100, 0);
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

    Renderer r(window);

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
