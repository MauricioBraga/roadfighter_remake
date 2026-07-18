#include "math.h"

#include "sdl3_compat.h"
#include "sdl3_ttf_compat.h"
#include <SDL3_mixer/SDL_mixer.h>

#include "sound.h"

#include "CTile.h"
#include "CObject.h"
#include "CGame.h"

extern int ENEMY_SPEED;
extern int ENEMY_HSPEED;
extern int PLAYING_WINDOW;


CFuelObject::CFuelObject(int nx,int ny,CTile *t,CGame *g) : CObject(nx,ny,t,CONSTITUTION_FUEL,g)
{
	y_speed=0;
	y_precision=0;
	

	state=0;

	x_speed=0;
	x_precision=0;

	blink_timmer=0;

} /* CFuelObject::CFuelObject */ 


CFuelObject::~CFuelObject(void)
{
} /* CFuelObject::~CFuelObject */ 



bool CFuelObject::cycle(unsigned char *keyboard,unsigned char *old_keyboard)
{
	// count time to blink the fuel object (for visual effect)
	blink_timmer++;
	
	y_precision+=y_speed;
	while(y_precision>(1<<8)) {
		y++;
		y_precision-=1<<8;
	} /* while */ 
	while(y_precision<((-1)<<8)) {
		y--;
		y_precision+=1<<8;
	} /* while */ 

	x_precision+=x_speed;
	while(x_precision>(1<<8)) {
		x++;
		x_precision-=1<<8;
	} /* while */ 
	while(x_precision<((-1)<<8)) {
		x--;
		x_precision+=1<<8;
	} /* while */ 

	y_speed=-ENEMY_SPEED;

	y_speed=-ENEMY_SPEED;
	x_speed=0;
	if (game->object_collision(16,0,this,CONSTITUTION_SOLID)!=0) {
		x_speed=-ENEMY_HSPEED;
	} else {
		if (game->object_collision(-16,0,this,CONSTITUTION_SOLID)!=0) {
			x_speed=ENEMY_HSPEED;
		} /* if */ 
	} /* if */ 

	if (game->min_distance_to_players(y)>PLAYING_WINDOW) return false;
	
	return true;
} /* CFuelObject::cycle */ 


// override draw method to implement any kind of animation / blinking effect for fuel objects
void CFuelObject::draw(int sx,int sy,SDL_Surface *screen)
{
	// removed for now, since the effect kind of sucked. :p
	/* Blink: visible for 2 frames, invisible for the next 2, and so on.  
	if (((blink_timmer/2)%2)==0) {
		CObject::draw(sx,sy,screen);
	} if */ 
	
	CObject::draw(sx,sy,screen);
	
} /* CFuelObject::draw */