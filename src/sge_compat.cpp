/*
 * sge_compat.cpp
 *
 * See sge_compat.h for rationale. The collision-map algorithm
 * (sge_make_cmap/sge_cmcheck/sge_get_cx/sge_get_cy/sge_destroy_cmap) is a
 * direct port of the relevant parts of the original SGE
 * (SDL Graphics Extension) library by Anders Lindström - only the single
 * pixel read used while BUILDING the map (sge_make_cmap) was adapted to
 * the SDL3 surface API; the bit-packed collision bitmap and the
 * bounding-box/pixel-AND scan are unchanged. sge_transform() is a new,
 * from-scratch rotate+scale blitter written against the SDL3 API, since
 * the original ~700 line implementation was written directly against
 * SDL 1.2's raw pixel buffer layout.
 */

#include "sge_compat.h"
#include <cmath>
#include <cstring>
#include <new>

using namespace std;

/* ---------------- collision maps (ported from sge_collision.cpp) ---------------- */

static Uint8 sge_mask[8]={1,2,4,8,16,32,64,128};
static SDL_Rect _ua;
static Sint16 _cx=0,_cy=0;

/* Minimal raw-pixel reader, just enough for sge_make_cmap(). Mirrors the
 * approach the game's own auxiliar.cpp getpixel() uses. */
static Uint32 raw_get_pixel(SDL_Surface *s, int x, int y)
{
	int bpp = SDL_BYTESPERPIXEL(s->format);
	Uint8 *p = (Uint8 *)s->pixels + y*s->pitch + x*bpp;

	switch(bpp) {
	case 1: return *p;
	case 2: return *(Uint16 *)p;
	case 3:
		if (SDL_BYTEORDER==SDL_BIG_ENDIAN) return p[0]<<16 | p[1]<<8 | p[2];
			else return p[0] | p[1]<<8 | p[2]<<16;
	case 4: return *(Uint32 *)p;
	default: return 0;
	}
} /* raw_get_pixel */


sge_cdata *sge_make_cmap(SDL_Surface *img)
{
	sge_cdata *cdata;
	Uint8 *map;
	Sint16 x,y;
	Sint32 offs;
	int i;
	Uint32 colorkey=0;

	SDL_GetSurfaceColorKey(img,&colorkey); /* defaults to 0 if never set, matching the original */

	cdata=new(nothrow) sge_cdata;
	if (!cdata) { SDL_SetError("sge_compat - Out of memory"); return NULL; }
	cdata->w=img->w; cdata->h=img->h;
	offs=(img->w*img->h)/8;
	cdata->map=new(nothrow) Uint8[offs+2];
	if (!cdata->map) { SDL_SetError("sge_compat - Out of memory"); return NULL; }
	memset(cdata->map,0x00,offs+2);

	map=cdata->map;

	SDL_LockSurface(img);
	i=0;
	for(y=0;y<img->h;y++) {
		for(x=0;x<img->w;x++) {
			if (i>7) { i=0; map++; }
			if (raw_get_pixel(img,x,y)!=colorkey) {
				*map=*map|sge_mask[i];
			} /* if */
			i++;
		} /* for */
	} /* for */
	SDL_UnlockSurface(img);

	return cdata;
} /* sge_make_cmap */


static int sge_bbcheck(sge_cdata *cd1,Sint16 x1,Sint16 y1, sge_cdata *cd2,Sint16 x2,Sint16 y2)
{
	Sint16 w1=cd1->w, h1=cd1->h, w2=cd2->w, h2=cd2->h;

	if (x1<x2) {
		if (x1+w1>x2) {
			if (y1<y2) {
				if (y1+h1>y2) { _ua.x=x2; _ua.y=y2; return 1; }
			} else {
				if (y2+h2>y1) { _ua.x=x2; _ua.y=y1; return 1; }
			} /* if */
		} /* if */
	} else {
		if (x2+w2>x1) {
			if (y2<y1) {
				if (y2+h2>y1) { _ua.x=x1; _ua.y=y1; return 1; }
			} else {
				if (y1+h1>y2) { _ua.x=x1; _ua.y=y2; return 1; }
			} /* if */
		} /* if */
	} /* if */

	return 0;
} /* sge_bbcheck */


static int memand(Uint8 *s1, Uint8 *s2, int shift1, int shift2, int N)
{
	int b,i1=shift1,i2=shift2;

	for(b=0;b<N;b++) {
		if (i1>7) { i1=0; s1++; }
		if (i2>7) { i2=0; s2++; }
		if ((*s1&sge_mask[i1]) && (*s2&sge_mask[i2])) return b+1;
		i1++; i2++;
	} /* for */
	return 0;
} /* memand */


static int _sge_cmcheck(sge_cdata *cd1,Sint16 x1,Sint16 y1, sge_cdata *cd2,Sint16 x2,Sint16 y2)
{
	if (cd1->map==NULL || cd2->map==NULL) return 0;

	Sint16 w1=cd1->w,h1=cd1->h,w2=cd2->w,h2=cd2->h;

	Sint32 x1o=0,x2o=0,y1o=0,y2o=0,offs;
	int i1=0,i2=0;

	Uint8 *map1=cd1->map;
	Uint8 *map2=cd2->map;

	if (_ua.x==x2 && _ua.y==y2) {
		x1o=x2-x1; y1o=y2-y1;
		offs=w1*y1o+x1o;
		map1+=offs/8; i1=offs%8;
	} else if (_ua.x==x2 && _ua.y==y1) {
		x1o=x2-x1; y2o=y1-y2;
		map1+=x1o/8; i1=x1o%8;
		offs=w2*y2o;
		map2+=offs/8; i2=offs%8;
	} else if (_ua.x==x1 && _ua.y==y1) {
		x2o=x1-x2; y2o=y1-y2;
		offs=w2*y2o+x2o;
		map2+=offs/8; i2=offs%8;
	} else if (_ua.x==x1 && _ua.y==y2) {
		x2o=x1-x2; y1o=y2-y1;
		offs=w1*y1o;
		map1+=offs/8; i1=offs%8;
		map2+=x2o/8; i2=x2o%8;
	} else {
		return 0;
	} /* if */

	Sint16 y,length;

	if (x1+w1<x2+w2) length=w1-x1o;
		else length=w2-x2o;

	for(y=_ua.y;y<=y1+h1 && y<=y2+h2;y++) {
		offs=memand(map1,map2,i1,i2,length);
		if (offs) { _cx=_ua.x+offs-1; _cy=y; return 1; }

		offs=(y-y1)*w1+x1o;
		map1=cd1->map; map1+=offs/8; i1=offs%8;

		offs=(y-y2)*w2+x2o;
		map2=cd2->map; map2+=offs/8; i2=offs%8;
	} /* for */

	return 0;
} /* _sge_cmcheck */


int sge_cmcheck(sge_cdata *cd1,Sint16 x1,Sint16 y1, sge_cdata *cd2,Sint16 x2,Sint16 y2)
{
	if (!sge_bbcheck(cd1,x1,y1,cd2,x2,y2)) return 0;
	if (cd1->map==NULL || cd2->map==NULL) return 1;
	return _sge_cmcheck(cd1,x1,y1,cd2,x2,y2);
} /* sge_cmcheck */


Sint16 sge_get_cx(void) { return _cx; }
Sint16 sge_get_cy(void) { return _cy; }


void sge_destroy_cmap(sge_cdata *cd)
{
	if (cd==0) return;
	if (cd->map!=NULL) delete [] cd->map;
	delete cd;
} /* sge_destroy_cmap */


/* ---------------- rotate + scale blit (new, written against SDL3) ---------------- */

SDL_Rect sge_transform(SDL_Surface *src, SDL_Surface *dst,
                        float angle, float xscale, float yscale,
                        Uint16 px, Uint16 py, Uint16 qx, Uint16 qy,
                        Uint8 /*flags*/)
{
	SDL_Rect result={0,0,0,0};

	if (src==0 || dst==0 || xscale==0.0F || yscale==0.0F) return result;

	double a = angle * (M_PI/180.0);
	double ca = cos(a), sa = sin(a);

	/* Corners of the source, relative to the hotspot (px,py), scaled and
	 * rotated, to find the destination bounding box. */
	double cornersx[4]={(double)0-px, (double)src->w-px, (double)0-px, (double)src->w-px};
	double cornersy[4]={(double)0-py, (double)0-py, (double)src->h-py, (double)src->h-py};

	double minx=1e30,maxx=-1e30,miny=1e30,maxy=-1e30;
	int i;
	for(i=0;i<4;i++) {
		double sx = cornersx[i]*xscale;
		double sy = cornersy[i]*yscale;
		double rx = sx*ca - sy*sa;
		double ry = sx*sa + sy*ca;
		if (rx<minx) minx=rx;
		if (rx>maxx) maxx=rx;
		if (ry<miny) miny=ry;
		if (ry>maxy) maxy=ry;
	} /* for */

	int dst_x0 = (int)floor(qx+minx);
	int dst_x1 = (int)ceil(qx+maxx);
	int dst_y0 = (int)floor(qy+miny);
	int dst_y1 = (int)ceil(qy+maxy);

	if (dst_x0<0) dst_x0=0;
	if (dst_y0<0) dst_y0=0;
	if (dst_x1>dst->w) dst_x1=dst->w;
	if (dst_y1>dst->h) dst_y1=dst->h;

	if (dst_x0>=dst_x1 || dst_y0>=dst_y1) return result;

	SDL_LockSurface(src);
	SDL_LockSurface(dst);

	int dx,dy;
	for(dy=dst_y0;dy<dst_y1;dy++) {
		for(dx=dst_x0;dx<dst_x1;dx++) {
			double rx = dx-qx;
			double ry = dy-qy;

			/* undo rotation */
			double ux = rx*ca + ry*sa;
			double uy = -rx*sa + ry*ca;

			/* undo scale, then move back relative to the source hotspot */
			double sxf = ux/xscale + px;
			double syf = uy/yscale + py;

			int sx=(int)floor(sxf+0.5);
			int sy=(int)floor(syf+0.5);

			if (sx<0 || sx>=src->w || sy<0 || sy>=src->h) continue;

			Uint8 r,g,b,a8;
			if (!SDL_ReadSurfacePixel(src,sx,sy,&r,&g,&b,&a8)) continue;
			if (a8==0) continue;

			if (a8==255) {
				SDL_WriteSurfacePixel(dst,dx,dy,r,g,b,255);
			} else {
				Uint8 dr,dg,db,da;
				SDL_ReadSurfacePixel(dst,dx,dy,&dr,&dg,&db,&da);
				Uint8 nr=(Uint8)((r*a8 + dr*(255-a8))/255);
				Uint8 ng=(Uint8)((g*a8 + dg*(255-a8))/255);
				Uint8 nb=(Uint8)((b*a8 + db*(255-a8))/255);
				SDL_WriteSurfacePixel(dst,dx,dy,nr,ng,nb,255);
			} /* if */
		} /* for */
	} /* for */

	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);

	result.x=dst_x0; result.y=dst_y0;
	result.w=dst_x1-dst_x0; result.h=dst_y1-dst_y0;
	return result;
} /* sge_transform */
