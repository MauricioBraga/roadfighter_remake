#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "sdl3_compat.h"
#include <SDL3_mixer/SDL_mixer.h>
#include "sdl3_ttf_compat.h"

#include "sound.h"

#include "CTile.h"
#include "CObject.h"
#include "CGame.h"
#include "CRoadFighter.h"
#include "auxiliar.h"

#include "debug.h"


/*						GLOBAL VARIABLES INITIALIZATION:							*/

const int REDRAWING_PERIOD=27;	/* This is for 35fps */
// const int REDRAWING_PERIOD=40;	/* This is for 25fps */

int SCREEN_X=512;
int SCREEN_Y=384;
const int COLOUR_DEPTH=32;
const char *application_name="Road Fighter v1.0";

// Default to fullscreen mode on startup.
bool fullscreen=true;

int init_time=0;
SDL_Window *window_wnd=0;
SDL_Surface *screen_sfc;
CRoadFighter *game=0;

int start_level=1;


/*						AUXILIAR FUNCTION DEFINITION:							*/

/* (Re)fetches the window's surface. In SDL3 the surface returned by
 * SDL_GetWindowSurface() can change identity whenever the window is
 * resized or its fullscreen state changes, so this is called again
 * every time that might have happened. */
SDL_Surface *window_sfc = NULL; /* the real window surface */

SDL_Surface *refresh_screen_surface(void)
{
	/* Re-fetch the underlying window surface and ensure we have a
	 * 32bpp backbuffer for the game to draw into (`screen_sfc`). */
	window_sfc = SDL_GetWindowSurface(window_wnd);
	if (window_sfc==0) {
		output_debug_message("Couldn't get the window surface: %s\n", SDL_GetError());
		return NULL;
	} /* if */

	/* If backbuffer isn't created yet, or its size changed, (re)create it
	   as a 32bpp surface matching the logical game resolution. */
	if (screen_sfc==NULL || screen_sfc->w!=SCREEN_X || screen_sfc->h!=SCREEN_Y ||
		SDL_BITSPERPIXEL(screen_sfc->format)!=32) {
		if (screen_sfc!=NULL) SDL_DestroySurface(screen_sfc);
		/* create a 32bpp XRGB surface for the game's drawing */
		screen_sfc = SDL_CreateSurface(SCREEN_X, SCREEN_Y, SDL_PIXELFORMAT_XRGB8888);
		if (screen_sfc==NULL) {
			output_debug_message("Couldn't create 32bpp backbuffer: %s\n", SDL_GetError());
			return NULL;
		}
	}

	return window_sfc;
} /* refresh_screen_surface */


SDL_Surface *initializeSDL(bool start_fullscreen)
{
	if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) return 0;

	output_debug_message("Initializing SDL video subsystem.\n");
	output_debug_message("SDL video subsystem initialized.\n");
	output_debug_message("SDL video driver used: %s\n", SDL_GetCurrentVideoDriver());

	atexit(SDL_Quit);

	 Uint32 win_flags = start_fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
	 /* SDL_CreateWindow in this environment uses the legacy compatibility
		 overload: SDL_CreateWindow(const char *title, int w, int h, SDL_WindowFlags)
		 so pass width/height directly. */
	 window_wnd = SDL_CreateWindow(application_name, SCREEN_X, SCREEN_Y, win_flags);
	if (window_wnd==0) {
		output_debug_message("Couldn't create the %ix%i window: %s\n", SCREEN_X, SCREEN_Y, SDL_GetError());
		exit(-1);
	} /* if */

	if (fullscreen) SDL_HideCursor();

	Sound_initialization();

	pause(1000);

	if (refresh_screen_surface()==0) exit(-1);

	output_debug_message("Set the video resolution to: %ix%ix%i",
						  screen_sfc->w, screen_sfc->h, SDL_BITSPERPIXEL(screen_sfc->format));
	if (fullscreen) output_debug_message(",fullscreen");
	output_debug_message("\n");

	TTF_Init();

	return screen_sfc;
} /* initializeSDL */


/* Toggles fullscreen on/off and re-fetches the window surface, which is
 * the SDL3 equivalent of the old "destroy and recreate the video mode"
 * dance that SDL 1.2 required. */
void toggle_fullscreen(void)
{
	Stop_playback();

	fullscreen = !fullscreen;
	SDL_SetWindowFullscreen(window_wnd, fullscreen);

	if (refresh_screen_surface()==0) {
		/* if we can't get a surface back, bail out gracefully */
		fullscreen = !fullscreen;
	} else {
		if (fullscreen) SDL_HideCursor();
			else SDL_ShowCursor();
	} /* if */

	Resume_playback();
} /* toggle_fullscreen */


void finalizeSDL()
{
	TTF_Quit();
//	Sound_release();
	SDL_Quit();
} /* finalizeSDL */



#ifdef _WIN32
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow)
{
	{
		int tmp;
		if (1==sscanf(lpCmdLine,"%i",&tmp)) {
			start_level=tmp;
			if (start_level<1 || start_level>6) start_level=1;
		} /* if */
	}

#else
int main(int argc, char** argv)
{
	setupTickCount();

	{
		int tmp;
		if (argc==2 &&
			1==sscanf(argv[1],"%i",&tmp)) {
			start_level=tmp;
			if (start_level<1 || start_level>6) start_level=1;
		} /* if */
	}
#endif

	int time,act_time;
	SDL_Event event;
    bool quit = false;


	time=init_time=GetTickCount();
	screen_sfc = initializeSDL(fullscreen);
	if (screen_sfc==0) return 0;

	game=new CRoadFighter();

	while (!quit) {
		while( SDL_PollEvent( &event ) ) {
            switch( event.type ) {
                /* Keyboard event */
                case SDL_EVENT_KEY_DOWN:
					// quit
					if (event.key.key==SDLK_F12) {
						quit = true;
					}
					if (event.key.key==SDLK_F4) {
						SDL_Keymod modifiers;
						modifiers=SDL_GetModState();
						if ((modifiers&SDL_KMOD_ALT)!=0) {
							quit=true;
						}
					}
#ifdef __APPLE__
                    // different quit shortcut on OSX: apple+Q
                    if (event.key.key == SDLK_Q) {
                        SDL_Keymod modifiers;
                        modifiers = SDL_GetModState();
                        if ((modifiers&SDL_KMOD_GUI) != 0) {
                            quit = true;
                        }
                    }
#endif
					// fullscreen
/*
FIXME: the code below is a big copy/paste; it should be in a separate function in stead
*/

#ifdef __APPLE__
					if (event.key.key == SDLK_F) {
						SDL_Keymod modifiers;

						modifiers=SDL_GetModState();

						if ((modifiers&SDL_KMOD_GUI) != 0) {
							toggle_fullscreen();
						} /* if */
					} /* if */
#endif

					if (event.key.key==SDLK_RETURN) {
						SDL_Keymod modifiers;

						modifiers=SDL_GetModState();

						if ((modifiers&SDL_KMOD_ALT)!=0) {
							toggle_fullscreen();
						} /* if */
					} /* if */
                    break;

                /* SDL_EVENT_QUIT (window close) */
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;
            } /* switch */
        } /* while */

		act_time=GetTickCount();
		if (act_time-time>=REDRAWING_PERIOD) {
			if ((act_time-init_time)>=1000) init_time=act_time;

			time+=REDRAWING_PERIOD;
			if ((act_time-time)>2*REDRAWING_PERIOD) time=act_time;

			if (!game->cycle()) quit=true;
			SDL_SetSurfaceClipRect(screen_sfc, 0);
			game->draw(screen_sfc);
			/* Scale the 512x384 backbuffer to the window surface size.
			   This ensures fullscreen mode renders scaled graphics instead of
			   drawing the game at native resolution only. */
			if (window_sfc!=NULL && screen_sfc!=NULL) {
				SDL_Rect dstrect = {0, 0, window_sfc->w, window_sfc->h};
				SDL_BlitSurfaceScaled(screen_sfc, NULL, window_sfc, &dstrect, SDL_SCALEMODE_PIXELART);
			}
			SDL_UpdateWindowSurface(window_wnd);
		} /* if */
		SDL_Delay(1);
	}

	delete game;

	finalizeSDL();

	return 0;
} /* main */

