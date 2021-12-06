#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "SdlWindow.hpp"

#ifdef _WIN32
    #define NOMINMAX 1
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif // _WIN32

void SdlInit()
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

    //SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 8);
    //SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 8);
    //SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 8);
    //SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
}

void SdlShutdown()
{
	SDL_Quit();
}

int main(int argc, char **argv)
{
	// Initialize SDL2 in general.
	SdlInit();

	SdlWindow *pWindow = new SdlWindow();
	if (pWindow->Init() != 0)
		return 1;

    //Enable text input
    SDL_StartTextInput();

	// Run the window.
	pWindow->Run();

    //Disable text input
    SDL_StopTextInput();

	SdlShutdown();

    return 0;
}

#ifdef __MINGW32__

// MinGW expects a WinMain().
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevIns, LPSTR lpszArgument, int iShow)
{
    return main(0, nullptr);
}

#endif // __MINGW32__
