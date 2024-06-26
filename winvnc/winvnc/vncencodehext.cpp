/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


// vncEncodeHexT

// This file implements the vncEncoder-derived vncEncodeHexT class.
// This class overrides some vncEncoder functions to produce a
// Hextile encoder. Hextile splits all top-level update rectangles
// into smaller, 16x16 rectangles and encodes these using the
// optimised Hextile sub-encodings.
#include "stdhdrs.h"
#include "vncencodehext.h"
#include "rfb.h"
#include "rfbMisc.h"
#include <stdlib.h>
#include <time.h>

vncEncodeHexT::vncEncodeHexT()
{
}

vncEncodeHexT::~vncEncodeHexT()
{
}

void
vncEncodeHexT::Init()
{
	vncEncoder::Init();
}

UINT
vncEncodeHexT::RequiredBuffSize(UINT width, UINT height)
{
	return vncEncoder::RequiredBuffSize(width, height) + (((width/16)+1) * ((height/16)+1));
}

UINT
vncEncodeHexT::NumCodedRects(const rfb::Rect &rect)
{
	return 1;
}

/*
 * hextile.c
 *
 * Routines to implement Hextile Encoding
 */

#include <stdio.h>
#include "rfb.h"

/*
 * vncEncodeHexT::EncodeRect - send a rectangle using hextile encoding.
 */

UINT
vncEncodeHexT::EncodeRect(BYTE *source, BYTE *dest, const rfb::Rect &rect_IN)
{
	rfb::Rect rect;
	rect.tl.x=(rect_IN.tl.x < 0)? 0: rect_IN.tl.x;
	rect.tl.y=(rect_IN.tl.y < 0)? 0: rect_IN.tl.y;
	rect.br.x=(rect_IN.br.x < 0)? 0: rect_IN.br.x;
	rect.br.y=(rect_IN.br.y < 0)? 0: rect_IN.br.y;

	const UINT rectW = rect.br.x - rect.tl.x;
	const UINT rectH = rect.br.y - rect.tl.y;

	// Create the rectangle header
	rfbFramebufferUpdateRectHeader *surh=(rfbFramebufferUpdateRectHeader *)dest;
	surh->r.x = (CARD16) (rect.tl.x-monitor_Offsetx);
	surh->r.y = (CARD16) (rect.tl.y-monitor_Offsety);
	surh->r.w = (CARD16) (rectW);
	surh->r.h = (CARD16) (rectH);
	surh->r.x = Swap16IfLE(surh->r.x);
	surh->r.y = Swap16IfLE(surh->r.y);
	surh->r.w = Swap16IfLE(surh->r.w);
	surh->r.h = Swap16IfLE(surh->r.h);
	surh->encoding = Swap32IfLE(rfbEncodingHextile);

	// Do the encoding
    switch (m_remoteformat.bitsPerPixel)
	{
	case 8:
		return sz_rfbFramebufferUpdateRectHeader +
			EncodeHextiles8(source, dest + sz_rfbFramebufferUpdateRectHeader,
				rect.tl.x, rect.tl.y, rectW, rectH);
    case 16:
		return sz_rfbFramebufferUpdateRectHeader +
			EncodeHextiles16(source, dest + sz_rfbFramebufferUpdateRectHeader,
				rect.tl.x, rect.tl.y, rectW, rectH);
    case 32:
		return sz_rfbFramebufferUpdateRectHeader +
			EncodeHextiles32(source, dest + sz_rfbFramebufferUpdateRectHeader,
				rect.tl.x, rect.tl.y, rectW, rectH);
    }

	return vncEncoder::EncodeRect(source, dest, rect);
}

#define PUT_PIXEL8(pix) (dest[destoffset++] = (pix))

#define PUT_PIXEL16(pix) (dest[destoffset++] = ((char*)&(pix))[0],			\
			  dest[destoffset++] = ((char*)&(pix))[1])

#define PUT_PIXEL32(pix) (dest[destoffset++] = ((char*)&(pix))[0],			\
			  dest[destoffset++] = ((char*)&(pix))[1],						\
			  dest[destoffset++] = ((char*)&(pix))[2],						\
			  dest[destoffset++] = ((char*)&(pix))[3])

#define DEFINE_SEND_HEXTILES(bpp)											\
																			\
static UINT subrectEncode##bpp(CARD##bpp *src, BYTE *dest,					\
				int w, int h, CARD##bpp bg,									\
			    CARD##bpp fg, BOOL mono);									\
static void testColours##bpp(CARD##bpp *data, int size, BOOL *mono,			\
			     BOOL *solid, CARD##bpp *bg, CARD##bpp *fg);				\
																			\
																			\
/*																			\
 * rfbSendHextiles															\
 */																			\
																			\
UINT																		\
vncEncodeHexT::EncodeHextiles##bpp(BYTE *source, BYTE *dest,				\
				  int rx, int ry, int rw, int rh)							\
{																			\
    int x, y, w, h;															\
    int rectoffset, destoffset;												\
    CARD##bpp bg = 0, fg = 0, newBg, newFg;									\
    BOOL mono, solid;														\
    BOOL validBg = FALSE;													\
    CARD##bpp clientPixelData[16*16*(bpp/8)];								\
    BOOL validFg = FALSE;													\
																			\
	destoffset = 0;															\
																			\
    for (y = ry; y < ry+rh; y += 16)										\
	{																		\
		for (x = rx; x < rx+rw; x += 16)									\
		{																	\
		    w = h = 16;														\
		    if (rx+rw - x < 16)												\
				w = rx+rw - x;												\
		    if (ry+rh - y < 16)												\
				h = ry+rh - y;												\
																			\
			rfb::Rect hexrect;												\
			hexrect.tl.x = x;												\
			hexrect.tl.y = y;												\
			hexrect.br.x = x+w;												\
			hexrect.br.y = y+h;												\
			Translate(source, (BYTE *) &clientPixelData, hexrect);			\
																			\
			rectoffset = destoffset;										\
			dest[rectoffset] = 0;											\
			destoffset++;													\
																			\
			testColours##bpp(clientPixelData, w * h,						\
			     &mono, &solid, &newBg, &newFg);							\
																			\
			if (!validBg || (newBg != bg))									\
			{																\
				validBg = TRUE;												\
				bg = newBg;													\
				dest[rectoffset] |= rfbHextileBackgroundSpecified;			\
				PUT_PIXEL##bpp(bg);											\
			}																\
																			\
			if (solid)														\
				continue;													\
																			\
			dest[rectoffset] |= rfbHextileAnySubrects;						\
																			\
			if (mono)														\
			{																\
				if (!validFg || (newFg != fg))								\
				{															\
					validFg = TRUE;											\
					fg = newFg;												\
					dest[rectoffset] |= rfbHextileForegroundSpecified;		\
					PUT_PIXEL##bpp(fg);										\
				}															\
			}																\
			else															\
			{																\
				validFg = FALSE;											\
				dest[rectoffset] |= rfbHextileSubrectsColoured;			    \
			}																\
																			\
			int encodedbytes = subrectEncode##bpp(clientPixelData,			\
								   dest + destoffset,						\
								   w, h, bg, fg, mono);						\
			destoffset += encodedbytes;										\
			if (encodedbytes == 0)											\
			{																\
				/* encoding was too large, use raw */						\
				validBg = FALSE;											\
				validFg = FALSE;											\
				destoffset = rectoffset;									\
				dest[destoffset++] = rfbHextileRaw;							\
																			\
				Translate(source, (BYTE *) &clientPixelData, hexrect);		\
																			\
				memcpy(dest + destoffset, (char *)clientPixelData,			\
			       w * h * (bpp/8));										\
																			\
				destoffset += w * h * (bpp/8);								\
		    }																\
		}																	\
    }																		\
																			\
    return destoffset;														\
}																			\
																			\
static UINT																	\
subrectEncode##bpp(CARD##bpp *src, BYTE *dest, int w, int h, CARD##bpp bg,	\
		   CARD##bpp fg, BOOL mono)											\
{																			\
    CARD##bpp cl;															\
    int x,y;																\
    int i,j;																\
    int hx=0,hy,vx=0,vy;													\
    int hyflag;																\
    CARD##bpp *seg;															\
    CARD##bpp *line;														\
    int hw,hh,vw,vh;														\
    int thex,they,thew,theh;												\
    int numsubs = 0;														\
    int newLen;																\
    int rectoffset;															\
	int destoffset;															\
																			\
	destoffset = 0;															\
    rectoffset = destoffset;												\
    destoffset++;															\
																			\
    for (y=0; y<h; y++)														\
	{																		\
		line = src+(y*w);													\
		for (x=0; x<w; x++)													\
		{																	\
		    if (line[x] != bg)												\
			{																\
				cl = line[x];												\
				hy = y-1;													\
				hyflag = 1;													\
				for (j=y; j<h; j++)											\
				{															\
					seg = src+(j*w);										\
					if (seg[x] != cl) {break;}								\
					i = x;													\
					while ((i < w) && (seg[i] == cl)) i += 1;				\
					i -= 1;													\
					if (j == y) vx = hx = i;								\
					if (i < vx) vx = i;										\
					if ((hyflag > 0) && (i >= hx))							\
					{														\
						hy += 1;											\
					}														\
					else													\
					{														\
						hyflag = 0;											\
					}														\
				}															\
				vy = j-1;													\
																			\
				/* We now have two possible subrects: (x,y,hx,hy) and		\
				 * (x,y,vx,vy). We'll choose the bigger of the two.		\
				 */															\
				hw = hx-x+1;												\
				hh = hy-y+1;												\
				vw = vx-x+1;												\
				vh = vy-y+1;												\
																			\
				thex = x;													\
				they = y;													\
																			\
				if ((hw*hh) > (vw*vh))										\
				{															\
				    thew = hw;												\
				    theh = hh;												\
				}															\
				else														\
				{															\
				    thew = vw;												\
				    theh = vh;												\
				}															\
																			\
				if (mono)													\
				{															\
				    newLen = destoffset - rectoffset + 2;					\
				}															\
				else														\
				{															\
				    newLen = destoffset - rectoffset + bpp/8 + 2;			\
				}															\
																			\
				if (newLen > (w * h * (bpp/8)))								\
				    return 0;												\
																			\
				numsubs += 1;												\
																			\
				if (!mono) PUT_PIXEL##bpp(cl);								\
																			\
				dest[destoffset++] = rfbHextilePackXY(thex,they);			\
				dest[destoffset++] = rfbHextilePackWH(thew,theh);			\
																			\
				/*															\
				 * Now mark the subrect as done.							\
				 */															\
				for (j=they; j < (they+theh); j++)							\
				{															\
					for (i=thex; i < (thex+thew); i++)						\
					{														\
						src[j*w+i] = bg;									\
					}														\
				}															\
		    }																\
		}																	\
    }																		\
																			\
    dest[rectoffset] = numsubs;												\
																			\
    return destoffset;														\
}																			\
																			\
																			\
/*																			\
 * testColours() tests if there are one (solid), two (mono) or more			\
 * colours in a tile and gets a reasonable guess at the best background	    \
 * pixel, and the foreground pixel for mono.								\
 */																			\
																			\
static void																	\
testColours##bpp(CARD##bpp *data, int size,									\
				 BOOL *mono, BOOL *solid,									\
				 CARD##bpp *bg, CARD##bpp *fg)								\
{																			\
    CARD##bpp colour1, colour2;												\
    int n1 = 0, n2 = 0;														\
    *mono = TRUE;															\
    *solid = TRUE;															\
	colour1=0;																\
	colour2=0;																\
																			\
    for (; size > 0; size--, data++)										\
	{																		\
																			\
		if (n1 == 0)														\
		    colour1 = *data;												\
																			\
		if (*data == colour1)												\
		{																	\
		    n1++;															\
		    continue;														\
		}																	\
																			\
		if (n2 == 0)														\
		{																	\
		    *solid = FALSE;													\
		    colour2 = *data;												\
		}																	\
																			\
		if (*data == colour2)												\
		{																	\
		    n2++;															\
		    continue;														\
		}																	\
																			\
		*mono = FALSE;														\
		break;																\
	}																		\
																			\
    if (n1 > n2)															\
	{																		\
		*bg = colour1;														\
		*fg = colour2;														\
    }																		\
	else																	\
	{																		\
		*bg = colour2;														\
		*fg = colour1;														\
    }																		\
}

DEFINE_SEND_HEXTILES(8)
DEFINE_SEND_HEXTILES(16)
DEFINE_SEND_HEXTILES(32)
