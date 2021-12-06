#include "SdlWindow.hpp"
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include "SdlRenderer.hpp"



SdlWindow::SdlWindow()
	: mWindow(nullptr),
	  m_uiEvent_DelegateCallbacks(0u),
	  mGuiQ(nullptr),
	  mRenderer(nullptr)
{
}



SdlWindow::~SdlWindow()
{
	delete mRenderer;
	delete mGuiQ;
}



int SdlWindow::Init()
{
	// Create the OpenGL window.
    mWindow = SDL_CreateWindow("Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (mWindow == nullptr)
    {
        printf ("Could not create window: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext oContext = SDL_GL_CreateContext(mWindow);

    // What does this return on various platforms?
    // Technique taken from https://wiki.libsdl.org/SDL_GetWindowWMInfo
    // and https://stackoverflow.com/questions/24117983/ .
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(mWindow, &wmInfo);
    #ifdef _WIN32
    HWND hwnd = wmInfo.info.win.window;
    #else // _WIN32
    auto hdis = wmInfo.info.x11.display;
    auto hwnd = wmInfo.info.x11.window;
    #endif // _WIN32

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
    m_uiEvent_DelegateCallbacks = SDL_RegisterEvents(1);
	
	// Prepare to receive events in the GUI thread.
    mGuiQ = new GuiThreadMessageQueue(m_uiEvent_DelegateCallbacks);
	if (mGuiQ == nullptr)
		return 1;

    // Create an OpenGL renderer that'll run in a separate thread.
    mRenderer = new SdlRenderer(mWindow, *mGuiQ);
	if (mRenderer == nullptr)
		return 1;
	
	return 0;
}



void SdlWindow::handleKeys(unsigned char key, int x, int y)
{
    //Toggle quad
    if (key == 'q')
    {
        //gRenderQuad = !gRenderQuad;
    }
}



void SdlWindow::Run()
{
	while (mWindow != nullptr)
    {
        SDL_Event event;

        if (SDL_WaitEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                mRenderer->stop();
                SDL_DestroyWindow(mWindow);
                mWindow = nullptr;
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
                        mRenderer->postMessage(SdlRenderer::RendererMsg(SdlRenderer::eRenderMsg_MoveLeft, 10.0f));
                        break;
                    case SDLK_RIGHT:
                        mRenderer->postMessage(SdlRenderer::RendererMsg(SdlRenderer::eRenderMsg_MoveRight, 10.0f));
                        break;
                    case SDLK_UP:
                        mRenderer->postMessage(SdlRenderer::RendererMsg(SdlRenderer::eRenderMsg_MoveUp, 10.0f));
                        break;
                    case SDLK_DOWN:
                        mRenderer->postMessage(SdlRenderer::RendererMsg(SdlRenderer::eRenderMsg_MoveDown, 10.0f));
                        break;
                    default:
                        break;
                    }
                }

            default:
                if(event.type == m_uiEvent_DelegateCallbacks)
                {
                    GuiMsg oGuiMsg;
                    while (mGuiQ->tryPop(oGuiMsg))
                    {
                        ::fprintf(stderr, "%s\n", oGuiMsg.getLog());
                        ::fflush(stderr);
                    }
                }
            }
        }
    }
}



void SdlWindow::Shutdown()
{
}



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
