/*
 * Copyright (c) 2000, 2010 Greg Haerr <greg@censoft.com>
 *
 * Right portrait mode subdriver for Microwindows
 */
#include <string.h>
#include "device.h"
#include "fb.h"

static void
fbportrait_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	psd->orgsubdriver->DrawPixel(psd, psd->yvirtres-y-1, x, c);
}

static MWPIXELVAL
fbportrait_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	return psd->orgsubdriver->ReadPixel(psd, psd->yvirtres-y-1, x);
}

static void
fbportrait_drawhorzline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	/*x2 = x2;
	while (x2 <= x1)
		fbportrait_drawpixel(psd, psd->yvirtres-y-1, x2++, c);*/

	psd->orgsubdriver->DrawVertLine(psd, psd->yvirtres-y-1, x1, x2, c);
}

static void
fbportrait_drawvertline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	/*while (y1 <= y2)
		fbportrait_drawpixel(psd, psd->yvirtres-1-y1++, x, c);*/

	psd->orgsubdriver->DrawHorzLine(psd, psd->yvirtres-y2-1, psd->yvirtres-y1-1, x, c);
}

static void
fbportrait_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
	MWPIXELVAL c)
{
	while(x1 <= x2)
		psd->orgsubdriver->DrawHorzLine(psd, psd->yvirtres-y2-1, psd->yvirtres-y1-1, x1++, c);
}

static void
fbportrait_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
	PSD srcpsd, MWCOORD srcx,MWCOORD srcy,long op)
{
	dstpsd->orgsubdriver->Blit(dstpsd, dstpsd->yvirtres-desty-h, destx,
		h, w, srcpsd, srcpsd->yvirtres-srcy-h, srcx, op);
}

static void
fbportrait_stretchblit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD dstw,
	MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw,
	MWCOORD srch, long op)
{
	dstpsd->orgsubdriver->StretchBlit(dstpsd, dstpsd->yvirtres-desty-dsth, destx,
		dsth, dstw, srcpsd, srcpsd->yvirtres-srcy-srch, srcx, srch, srcw, op);
}

#if MW_FEATURE_PSDOP_ALPHACOL
static void
fbportrait_drawarea_alphacol(PSD dstpsd, driver_gc_t *gc)
{
	ADDR8 alpha_in, alpha_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_w, out_h;
	driver_gc_t	l_gc;

	l_gc = *gc;
	l_gc.dstx = dstpsd->yvirtres - gc->dsty - gc->dsth;
	l_gc.dsty = gc->dstx;
	l_gc.dstw = gc->dsth;
	l_gc.dsth = gc->dstw;

	l_gc.srcx = 0;
	l_gc.srcy = 0;
	l_gc.src_linelen = l_gc.dstw;
	if (!(l_gc.misc = ALLOCA(l_gc.dstw * l_gc.dsth)))
		return;

	alpha_in = ((ADDR8)gc->misc) + gc->src_linelen * gc->srcy + gc->srcx;
	in_w = gc->dstw;
	in_h = gc->dsth;

	alpha_out = l_gc.misc;
	out_w = l_gc.dstw;
	out_h = l_gc.dsth;

	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = in_x;
			out_x = (out_w - 1) - in_y;

			alpha_out[(out_y * out_w) + out_x] = alpha_in[(in_y * in_w) + in_x];
		}
	}

	dstpsd->orgsubdriver->DrawArea(dstpsd, &l_gc, PSDOP_ALPHACOL);

	FREEA(l_gc.misc);
}
#endif

#if MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST
static void
fbportrait_drawarea_bitmap_bytes_msb_first(PSD psd, driver_gc_t * gc)
{
	ADDR8 pixel_in, pixel_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_w, out_h;
	driver_gc_t	l_gc;

	l_gc = *gc;
	l_gc.dstx = psd->yvirtres - gc->dsty - gc->dsth;
	l_gc.dsty = gc->dstx;
	l_gc.dstw = gc->dsth;
	l_gc.dsth = gc->dstw;
	l_gc.srcx = 0;
	l_gc.srcy = 0;

	l_gc.src_linelen = (l_gc.dstw + 7) / 8;
	if (!(l_gc.pixels = ALLOCA(l_gc.dsth * l_gc.src_linelen)))
		return;
	memset(l_gc.pixels, 0, l_gc.dsth * l_gc.src_linelen);

	pixel_in = ((ADDR8)gc->pixels) + gc->src_linelen * gc->srcy + gc->srcx;
	in_w = gc->dstw;
	in_h = gc->dsth;

	pixel_out = l_gc.pixels;
	out_w = l_gc.dstw;
	out_h = l_gc.dsth;

	/* rotate_right_1bpp*/
	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = in_x;
			out_x = (out_w - 1) - in_y;

			//pixel_out[(out_y * out_w) + out_x] = pixel_in[(in_y * in_w) + in_x];
			if (pixel_in[in_y*gc->src_linelen + (in_x >> 3)] & (0x80 >> (in_x&7)))
				pixel_out[out_y*l_gc.src_linelen + (out_x >> 3)] |= (0x80 >> (out_x&7));
		}
	}

	psd->orgsubdriver->DrawArea(psd, &l_gc, PSDOP_BITMAP_BYTES_MSB_FIRST);

	FREEA(l_gc.pixels);
}
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST */

static void
fbportrait_drawarea(PSD dstpsd, driver_gc_t * gc, int op)
{
	if (!dstpsd->orgsubdriver->DrawArea)
		return;

	switch(op) {
#if MW_FEATURE_PSDOP_ALPHACOL
	case PSDOP_ALPHACOL:
		fbportrait_drawarea_alphacol(dstpsd, gc);
		break;
#endif

#if MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST
	case PSDOP_BITMAP_BYTES_MSB_FIRST:
		fbportrait_drawarea_bitmap_bytes_msb_first(dstpsd, gc);
		break;
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST */
	}
}

SUBDRIVER fbportrait_right = {
	NULL,
	fbportrait_drawpixel,
	fbportrait_readpixel,
	fbportrait_drawhorzline,
	fbportrait_drawvertline,
	fbportrait_fillrect,
	fbportrait_blit,
	fbportrait_drawarea,
	fbportrait_stretchblit
};
