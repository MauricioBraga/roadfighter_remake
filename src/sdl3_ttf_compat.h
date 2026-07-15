#ifndef SDL3_TTF_COMPAT_H
#define SDL3_TTF_COMPAT_H

#include <SDL3_ttf/SDL_ttf.h>

/* SDL_ttf for SDL3 added an explicit byte-length parameter to the render
 * functions RoadFighter uses. This overload keeps the old 3-argument call
 * sites (null-terminated C strings) compiling unchanged. */
static inline SDL_Surface *TTF_RenderText_Blended(TTF_Font *font, const char *text, SDL_Color fg)
{
	return TTF_RenderText_Blended(font, text, 0, fg);
} /* TTF_RenderText_Blended */

#endif /* SDL3_TTF_COMPAT_H */
