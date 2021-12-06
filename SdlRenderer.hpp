#ifndef __SDL_RENDERER_HPP__
#define __SDL_RENDERER_HPP__



#include <atomic>
#include <SDL2/SDL.h>
#include <tbb/concurrent_queue.h>
#include <thread>

// Forward declarations.
typedef struct SDL_Window SDL_Window;
class GuiThreadMessageQueue;



class SdlRenderer
{
public:
    SdlRenderer(SDL_Window* window,
        GuiThreadMessageQueue &a_rGuiThreadMessageQueue);
    ~SdlRenderer();
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



#endif // !__SDL_RENDERER_HPP__
