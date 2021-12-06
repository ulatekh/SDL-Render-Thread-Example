#ifndef __SDL_WINDOW_HPP__
#define __SDL_WINDOW_HPP__

#include <atomic>
#include <cstdint>
#include <tbb/concurrent_queue.h>
#ifndef _WIN32
	#include <safeclib/safe_str_lib.h>
#endif // _WIN32



// Forward declarations.
typedef struct SDL_Window SDL_Window;
class GuiThreadMessageQueue;
class SdlRenderer;
		


class SdlWindow
{
public:
	SdlWindow();
	~SdlWindow();
	int Init();
	void Run();
	void Shutdown();
	
private:
	SDL_Window *mWindow;
	uint32_t m_uiEvent_DelegateCallbacks;
	GuiThreadMessageQueue *mGuiQ;
	SdlRenderer *mRenderer;
	
	void handleKeys(unsigned char key, int x, int y);
};



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



#endif // __SDL_WINDOW_HPP__
