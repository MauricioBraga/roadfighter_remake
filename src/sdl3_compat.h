/*
 * sdl3_compat.h
 *
 * RoadFighter was written against SDL 1.2. This header bridges the small,
 * well-defined set of API differences that the rest of the (otherwise
 * ported) game code relies on, so that most call sites did not have to be
 * rewritten one by one. It is included instead of "SDL.h" everywhere.
 *
 * What it provides:
 *  - The real SDL3 API (via <SDL3/SDL.h>).
 *  - Legacy flag names used only as harmless no-op bits nowadays
 *    (SDL_HWSURFACE, SDL_SWSURFACE, SDL_DOUBLEBUF, SDL_HWPALETTE,
 *    SDL_SRCALPHA, SDL_GLSDL) so old call sites that OR them together
 *    still compile.
 *  - Old-style overloads of SDL_MapRGB/SDL_MapRGBA/SDL_GetRGB/SDL_GetRGBA
 *    that take a plain SDL_PixelFormat (as SDL 1.2's `surface->format`
 *    effectively was used) instead of the new SDL_PixelFormatDetails*.
 *    Surface->format IS an SDL_PixelFormat in SDL3, so old call sites
 *    like SDL_MapRGBA(surface->format, r,g,b,a) keep compiling unchanged.
 *  - An SDL_SetAlpha() shim mapping the old whole-surface alpha/blend
 *    flag onto SDL_SetSurfaceBlendMode()/SDL_SetSurfaceAlphaMod().
 *  - SDL_DisplayFormat()/SDL_DisplayFormatAlpha() shims, converting to a
 *    canonical 32bpp surface the way the old "convert to the fastest
 *    blittable format" calls did.
 */

#ifndef SDL3_COMPAT_H
#define SDL3_COMPAT_H

/* Let SDL3 itself provide macros for the many functions that were purely
 * renamed (not resignatured) going from SDL 1.2/2 to SDL3, e.g.
 * SDL_FreeSurface -> SDL_DestroySurface. This must be defined before
 * <SDL3/SDL.h> is included. */
#define SDL_ENABLE_OLD_NAMES

#include <SDL3/SDL.h>

/* ---- legacy flags, kept only so old bitwise-OR'd flag expressions compile ---- */
#ifndef SDL_SWSURFACE
#define SDL_SWSURFACE 0
#endif
#ifndef SDL_HWSURFACE
#define SDL_HWSURFACE 0
#endif
#ifndef SDL_DOUBLEBUF
#define SDL_DOUBLEBUF 0
#endif
#ifndef SDL_HWPALETTE
#define SDL_HWPALETTE 0
#endif
#ifndef SDL_GLSDL
#define SDL_GLSDL 0
#endif
#ifndef SDL_SRCALPHA
#define SDL_SRCALPHA 1
#endif

/* ---- pixel format helpers: old-style overloads keyed off SDL_PixelFormat ---- */

static inline Uint32 SDL_MapRGB(SDL_PixelFormat format, Uint8 r, Uint8 g, Uint8 b)
{
	return SDL_MapRGB(SDL_GetPixelFormatDetails(format), NULL, r, g, b);
} /* SDL_MapRGB */

static inline Uint32 SDL_MapRGBA(SDL_PixelFormat format, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return SDL_MapRGBA(SDL_GetPixelFormatDetails(format), NULL, r, g, b, a);
} /* SDL_MapRGBA */

static inline void SDL_GetRGB(Uint32 pixel, SDL_PixelFormat format, Uint8 *r, Uint8 *g, Uint8 *b)
{
	SDL_GetRGB(pixel, SDL_GetPixelFormatDetails(format), NULL, r, g, b);
} /* SDL_GetRGB */

static inline void SDL_GetRGBA(Uint32 pixel, SDL_PixelFormat format, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a)
{
	SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(format), NULL, r, g, b, a);
} /* SDL_GetRGBA */

/* ---- per-surface alpha (SDL 1.2's SDL_SetAlpha) ---- */

static inline int SDL_SetAlpha(SDL_Surface *surface, Uint32 flag, Uint8 alpha)
{
	if (flag & SDL_SRCALPHA) {
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	} else {
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
	} /* if */
	SDL_SetSurfaceAlphaMod(surface, alpha);
	return 0;
} /* SDL_SetAlpha */

/* ---- "convert to a fast, canonical format" (SDL 1.2's DisplayFormat calls) ---- */

static inline SDL_Surface *SDL_DisplayFormat(SDL_Surface *surface)
{
	SDL_Surface *res = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_XRGB8888);
	SDL_FreeSurface(surface);
	return res;
} /* SDL_DisplayFormat */

static inline SDL_Surface *SDL_DisplayFormatAlpha(SDL_Surface *surface)
{
	SDL_Surface *res = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);
	SDL_FreeSurface(surface);
	return res;
} /* SDL_DisplayFormatAlpha */

/* ---- SDL_CreateRGBSurface (SDL 1.2/2) -> SDL_CreateSurface (SDL3) ----
 * RoadFighter only ever creates 32bpp surfaces, either with an alpha
 * channel (non-zero amask, for sprites that need per-pixel transparency)
 * or without one (all masks zero, meaning "opaque native surface" back
 * when SDL_CreateRGBSurface auto-picked masks). depth/rmask/gmask/bmask
 * are therefore ignored; only whether amask is zero or not matters. */
static inline SDL_Surface *SDL_CreateRGBSurface(Uint32 /*flags*/, int w, int h, int /*depth*/,
                                                 Uint32 /*rmask*/, Uint32 /*gmask*/, Uint32 /*bmask*/, Uint32 amask)
{
	return SDL_CreateSurface(w, h, amask!=0 ? SDL_PIXELFORMAT_ARGB8888 : SDL_PIXELFORMAT_XRGB8888);
} /* SDL_CreateRGBSurface */

/* ---- misc renamed calls used in a handful of places ---- */

static inline SDL_Rect SDL_GetClipRect_compat(SDL_Surface *surface)
{
	SDL_Rect r;
	SDL_GetSurfaceClipRect(surface, &r);
	return r;
} /* SDL_GetClipRect_compat */
#undef SDL_GetClipRect
#define SDL_GetClipRect(surface, rect) (*(rect) = SDL_GetClipRect_compat(surface))
#undef SDL_SetClipRect
#define SDL_SetClipRect(surface, rect) SDL_SetSurfaceClipRect(surface, rect)
#define SDL_BlitSurface SDL_BlitSurface /* unchanged: (src,srcrect,dst,dstrect), now returns bool */

#endif /* SDL3_COMPAT_H */
