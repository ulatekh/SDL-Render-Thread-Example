#include "SdlRenderer.hpp"
#include <GL/gl.h>
#include <SDL2/SDL.h>
#include "SdlWindow.hpp"




SdlRenderer::SdlRenderer(SDL_Window* window,
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
    mThread = new std::thread (&SdlRenderer::renderJob, this);

    // Wait for the renderer to be initialized.
    SDL_LockMutex(mLock);
    while (mFirstRun)
        SDL_CondWait(mCond, mLock);
    SDL_UnlockMutex(mLock);
}


SdlRenderer::~SdlRenderer()
{
    SDL_DestroyCond(mCond);
    SDL_DestroyMutex(mLock);
    delete mThread;
}


void SdlRenderer::stop()
{
    mKeepRunning = false;
    mThread->join();
}


void SdlRenderer::postMessage(RendererMsg const &a_rMsg)
{
    m_lstQueuedRenderMessages.push(a_rMsg);
}


void SdlRenderer::render()
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
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 640, 480, 0, 1, -1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);
    glColor3f(1, 0, 0); glVertex3f(m_fX + 0, m_fY + 0, 0);
    glColor3f(1, 1, 0); glVertex3f(m_fX + 100, m_fY + 0, 0);
    glColor3f(1, 0, 1); glVertex3f(m_fX + 100, m_fY + 100, 0);
    glColor3f(1, 1, 1); glVertex3f(m_fX + 0, m_fY + 100, 0);
    glEnd();

    SDL_GL_SwapWindow(mWindow);
}

void SdlRenderer::renderJob()
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
