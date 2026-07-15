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


CEnemyNormalCarObject::CEnemyNormalCarObject(int nx,int ny,CTile *t,int start_delay,CGame *g) : CEnemyCarObject(nx,ny,t,start_delay,g)
{
} /* CEnemyNormalCarObject::CEnemyNormalCarObject */ 


CEnemyNormalCarObject::~CEnemyNormalCarObject(void)
{
} /* CEnemyNormalCarObject::~CEnemyNormalCarObject */ 


