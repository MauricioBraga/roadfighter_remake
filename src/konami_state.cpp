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

const int konami_fade_time=25;

int CRoadFighter::konami_cycle(void)
{
	if (state_timmer==0) {
		output_debug_message("CRoadFighter::konami_cycle: cycle 0.\n");
		konami_state=0;
		konami_timmer=0;
	} /* if */ 

	if (konami_state==0) konami_timmer++;
	if (konami_state==1) konami_timmer--;

	if (konami_state==0 && state_timmer == 160) {
		output_debug_message("CRoadFighter::konami_cycle: loading sound/braingames.\n");
		Sound_play(Sound_create_sound("sound/braingames"));
		output_debug_message("CRoadFighter::konami_cycle: sound/braingames loaded and playing.\n");
	}

	if (konami_state==0 && 
		(state_timmer>=350 ||
		 (keyboard[fire_key] && !old_keyboard[fire_key]) ||
		 (keyboard[SDL_SCANCODE_ESCAPE] && !old_keyboard[SDL_SCANCODE_ESCAPE]))) {
		konami_state=1;
		if (konami_timmer>konami_fade_time) konami_timmer=konami_fade_time;
	} /* if */ 

	if (konami_state==1 && konami_timmer<=0) {
		output_debug_message("CRoadFighter::konami_cycle: going to MENU_STATE.\n");
		return MENU_STATE;
	} /* if */ 

	return KONAMI_STATE;
} /* CRoadFighter::konami_cycle */ 



void CRoadFighter::konami_draw(SDL_Surface *screen)
{
	SDL_Rect r;

	output_debug_message("CRoadFighter::konami_draw: entered, state_timmer=%d.\n",state_timmer);

	r.x=0;
	r.y=0;
	r.w=konami1_sfc->w;
	r.h=konami1_sfc->h;

	output_debug_message("CRoadFighter::konami_draw: about to blit konami1_sfc.\n");
	SDL_BlitSurface(konami1_sfc,&r,screen,&r);
	output_debug_message("CRoadFighter::konami_draw: konami1_sfc blitted.\n");

	r.x=0;
	r.y=0;
	r.w=konami2_sfc->w;
	r.h=state_timmer*2;

	output_debug_message("CRoadFighter::konami_draw: about to blit konami2_sfc, r.h=%d, konami2_sfc=%dx%d.\n",r.h,konami2_sfc->w,konami2_sfc->h);
	SDL_BlitSurface(konami2_sfc,&r,screen,&r);
	output_debug_message("CRoadFighter::konami_draw: konami2_sfc blitted.\n");

	{
		float f=float(konami_timmer)*(1.0F/float(konami_fade_time));
		if (f<0.0) f=0.0;
		if (f<1.0) {
			output_debug_message("CRoadFighter::konami_draw: about to call surface_fader, f=%f.\n",f);
			surface_fader(screen,f,f,f,0);
			output_debug_message("CRoadFighter::konami_draw: surface_fader done.\n");
		} /* if */ 
	} /* if */ 

	output_debug_message("CRoadFighter::konami_draw: leaving.\n");

} /* CRoadFighter::konami_draw */ 
