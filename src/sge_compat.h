/*
 * sge_compat.h
 *
 * Minimal, self-contained replacement for the tiny subset of the old
 * "SDL Graphics Extension" (SGE) library that RoadFighter actually uses:
 *
 *   - sge_cdata / sge_make_cmap / sge_destroy_cmap   (pixel collision maps)
 *   - sge_cmcheck / sge_get_cx / sge_get_cy          (pixel-perfect collision)
 *   - sge_transform                                  (rotate + scale blit)
 *
 * The original SGE library (~380KB of code) worked directly on SDL 1.2
 * SDL_Surface pixel buffers. Porting all of it to SDL3 would have meant
 * touching hundreds of lines that RoadFighter never calls. Instead, this
 * file re-implements just these functions on top of the SDL3 API
 * (SDL_ReadSurfacePixel / SDL_WriteSurfacePixel / SDL_GetSurfaceColorKey),
 * keeping the exact same function names/signatures the game code expects.
 */

#ifndef SGE_COMPAT_H
#define SGE_COMPAT_H

#include <SDL3/SDL.h>

/* ---- Collision map data (same layout/semantics as the original SGE) ---- */
struct sge_cdata {
	Uint8 *map;
	Sint16 w, h;
};

/* Builds a 1-bit-per-pixel collision mask from img's alpha/colorkey.
 * Set the colorkey (or per-pixel alpha) on img BEFORE calling this. */
sge_cdata *sge_make_cmap(SDL_Surface *img);

/* Frees a collision map created by sge_make_cmap(). */
void sge_destroy_cmap(sge_cdata *cd);

/* Pixel-perfect collision test between two collision maps positioned at
 * (x1,y1) and (x2,y2) in the same coordinate space. Returns non-zero on
 * collision, and sets the collision point retrievable via sge_get_cx/cy. */
int sge_cmcheck(sge_cdata *cd1, Sint16 x1, Sint16 y1, sge_cdata *cd2, Sint16 x2, Sint16 y2);

Sint16 sge_get_cx(void);
Sint16 sge_get_cy(void);

/* Rotates (angle in degrees, clockwise) and scales src around source point
 * (px,py), placing that point at destination point (qx,qy) on dst. This
 * mirrors the original sge_transform() signature; the trailing "flags"
 * parameter is accepted for source compatibility but ignored (the original
 * anti-aliasing/safety flags aren't needed for RoadFighter's usage). */
SDL_Rect sge_transform(SDL_Surface *src, SDL_Surface *dst,
                        float angle, float xscale, float yscale,
                        Uint16 px, Uint16 py, Uint16 qx, Uint16 qy,
                        Uint8 flags = 0);

#endif /* SGE_COMPAT_H */
