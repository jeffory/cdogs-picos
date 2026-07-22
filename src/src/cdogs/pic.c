/*
	Copyright (c) 2013-2016, 2018-2019, 2024 Cong Xu
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	Redistributions of source code must retain the above copyright notice, this
	list of conditions and the following disclaimer.
	Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/
#include "pic.h"

#include <stdlib.h>
#include <string.h>

#include "c_hashmap/hashmap.h"
#include "defs.h"
#include "grafx.h"
#include "log.h"
#include "texture.h"
#include "utils.h"

#ifdef PICOS
#include "picos_heap.h"
#include "picos_sdl_impl.h"
#endif

map_t textureDebugger = NULL;


color_t PixelToColor(
	const SDL_PixelFormat *f, const Uint8 aShift, const Uint32 pixel)
{
	color_t c;
	SDL_GetRGBA(pixel, f, &c.r, &c.g, &c.b, &c.a);
	// Manually apply the alpha as SDL seems to always set it to 0
	c.a = (Uint8)((pixel & ~(f->Rmask | f->Gmask | f->Bmask)) >> aShift);
	return c;
}
Uint32 ColorToPixel(
	const SDL_PixelFormat *f, const Uint8 aShift, const color_t color)
{
	const Uint32 pixel = SDL_MapRGBA(f, color.r, color.g, color.b, color.a);
	// Manually apply the alpha as SDL seems to always set it to 0
	return (pixel & (f->Rmask | f->Gmask | f->Bmask)) |
		((Uint32)color.a << aShift);
}

// Get the true pixel size of the pic
static struct vec2i PicPixelSize(const Pic *p)
{
	if (p->isHD)
	{
		return svec2i_scale(p->size, 2);
	}
	return p->size;
}

// ─── Format-aware pixel accessors ──────────────────────────────────────
// The only code allowed to branch on Pic::fmt. Everything else in the
// codebase reads/writes pixels exclusively through these.
size_t PicPxBytes(const Pic *p)
{
	switch (p->fmt)
	{
	case PIC_FMT_RGB565:
	case PIC_FMT_LA8:
		return sizeof(uint16_t);
	case PIC_FMT_ARGB8888:
	default:
		return sizeof(Uint32);
	}
}
color_t PicPx(const Pic *p, int i)
{
	switch (p->fmt)
	{
#ifdef PICOS
	case PIC_FMT_RGB565:
	{
		const uint16_t px = ((const uint16_t *)p->Data)[i];
		if (px == PICOS_RGB565_CKEY)
		{
			// Matches the pre-RGB565 sentinel: a transparent source pixel
			// was stored as a whole-zero word, i.e. (r,g,b,a) = (0,0,0,0).
			return (color_t){0, 0, 0, 0};
		}
		uint32_t r, g, b;
		picos_unpack565(px, &r, &g, &b);
		return (color_t){(uint8_t)r, (uint8_t)g, (uint8_t)b, 255};
	}
	case PIC_FMT_LA8:
	{
		const uint16_t px = ((const uint16_t *)p->Data)[i];
		const uint8_t l = (uint8_t)(px & 0xFF);
		const uint8_t a = (uint8_t)((px >> 8) & 0xFF);
		return (color_t){l, l, l, a};
	}
#endif
	case PIC_FMT_ARGB8888:
	default:
		return PIXEL2COLOR(((const Uint32 *)p->Data)[i]);
	}
}
void PicPxSet(Pic *p, int i, color_t c)
{
	switch (p->fmt)
	{
#ifdef PICOS
	case PIC_FMT_RGB565:
	{
		// Reuse the existing ARGB8888->RGB565 helper (alpha<128 -> CKEY)
		// rather than re-deriving the threshold here.
		const uint32_t argb = ((uint32_t)c.a << 24) | ((uint32_t)c.r << 16) |
			((uint32_t)c.g << 8) | c.b;
		((uint16_t *)p->Data)[i] = picos_argb_to_565(argb);
		return;
	}
	case PIC_FMT_LA8:
	{
		const uint8_t l = (uint8_t)MAX(c.r, MAX(c.g, c.b));
		((uint16_t *)p->Data)[i] = (uint16_t)(l | ((uint16_t)c.a << 8));
		return;
	}
#endif
	case PIC_FMT_ARGB8888:
	default:
		((Uint32 *)p->Data)[i] = COLOR2PIXEL(c);
		return;
	}
}
bool PicPxTransparent(const Pic *p, int i)
{
	switch (p->fmt)
	{
#ifdef PICOS
	case PIC_FMT_RGB565:
		return ((const uint16_t *)p->Data)[i] == PICOS_RGB565_CKEY;
	case PIC_FMT_LA8:
		return ((const uint16_t *)p->Data)[i] == 0x0000;
#endif
	case PIC_FMT_ARGB8888:
	default:
		return ((const Uint32 *)p->Data)[i] == 0;
	}
}
void PicPxCopy(Pic *dst, int di, const Pic *src, int si)
{
	CASSERT(dst->fmt == src->fmt, "PicPxCopy format mismatch");
	const size_t bpp = PicPxBytes(src);
	memcpy((uint8_t *)dst->Data + (size_t)di * bpp,
		(const uint8_t *)src->Data + (size_t)si * bpp, bpp);
}

// Number of bytes needed to store a 2-bit-per-pixel Channels map for
// `count` pixels (4 pixels/byte). Channels is always NULL until Task 3;
// this sizing is exercised now so PicCopy/PicShrink can't forget an owner.
static size_t PicChannelsBytes(const int count)
{
	return ((size_t)count + 3) / 4;
}
static uint8_t PicChannelGet(const uint8_t *channels, const int i)
{
	return (uint8_t)((channels[i / 4] >> ((i % 4) * 2)) & 0x3);
}
static void PicChannelSet(uint8_t *channels, const int i, const uint8_t value)
{
	const int byteIdx = i / 4;
	const int shift = (i % 4) * 2;
	channels[byteIdx] = (uint8_t)(
		(channels[byteIdx] & ~(0x3 << shift)) | ((value & 0x3) << shift));
}

void PicLoad(
	Pic *p, const struct vec2i size, const struct vec2i offset, const SDL_Surface *image, const bool isHD,
	const PicFormat fmt)
{
	memset(p, 0, sizeof *p);
	p->size = size;
	// Pretend to be half the size for HD pics
	p->isHD = isHD;
	if (isHD)
	{
		p->size = svec2i_scale_divide(p->size, 2);
	}
	p->offset = svec2i_zero();
#ifdef PICOS
	p->fmt = (uint8_t)fmt;
#else
	// Desktop has no software RGB565/LA8 rendering path -- every pic stays
	// ARGB8888 regardless of what the caller asked for.
	p->fmt = PIC_FMT_ARGB8888;
#endif
	CMALLOC(p->Data, (size_t)size.x * size.y * PicPxBytes(p));
	if (p->Data == NULL)
	{
		return;
	}
#ifdef PICOS
	g_picos_pic_data_bytes += (size_t)size.x * size.y * PicPxBytes(p);
	g_picos_pic_count++;
	picos_gfx_bytes_peak_sample();
#endif
	// Manually copy the pixels and replace the alpha component,
	// since our gfx device format has no alpha
	int srcI = offset.y*image->w + offset.x;
	for (int i = 0; i < size.x * size.y; i++, srcI++)
	{
		const Uint32 pixel = ((Uint32 *)image->pixels)[srcI];
		color_t c;
		SDL_GetRGBA(pixel, image->format, &c.r, &c.g, &c.b, &c.a);
		// If completely transparent, replace rgb with black (0) too
		// This is because transparency blitting checks entire pixel
		if (c.a == 0)
		{
			PicPxSet(p, i, (color_t){0, 0, 0, 0});
		}
		else
		{
			PicPxSet(p, i, c);
		}
		if ((i + 1) % size.x == 0)
		{
			srcI += image->w - size.x;
		}
	}

	if (!PicTryMakeTex(p))
	{
		goto bail;
	}
	return;

bail:
	PicFree(p);
}
bool PicTryMakeTex(Pic *p)
{
	CASSERT(!PicIsNone(p), "cannot make tex of none pic");
	if (textureDebugger == NULL)
	{
		textureDebugger = hashmap_new();
	}
	if (p->Tex != NULL)
	{
		LOG(LM_GFX, LL_TRACE, "destroying texture %p data(%p)", p->Tex, p->Data);
		SDL_DestroyTexture(p->Tex);
		if (LL_TRACE >= LogModuleGetLevel(LM_GFX))
		{
			char key[32];
			sprintf(key, "%p", p->Tex);
			if (hashmap_get(textureDebugger, key, NULL) == MAP_OK)
			{
				if (hashmap_remove(textureDebugger, key) != MAP_OK)
				{
					LOG(LM_GFX, LL_TRACE, "Error: cannot remove tex from debugger");
				}
				else
				{
					LOG(LM_GFX, LL_TRACE, "Texture count: %d",
						hashmap_length(textureDebugger));
				}
			}
			else
			{
				LOG(LM_GFX, LL_TRACE, "Error: destroying unknown texture");
			}
		}
	}
	const struct vec2i size = PicPixelSize(p);
#ifdef PICOS
	/* No GPU: TextureCreate + SDL_UpdateTexture would allocate and memcpy a
	   byte-identical second copy of p->Data.  Borrow it instead.
	   Safe because PicFree destroys Tex before CFREE(pic->Data), and
	   PicShrink calls back here after replacing Data. */
	p->Tex = PicosTextureBorrow(p->Data, size.x, size.y, p->fmt);
	if (p->Tex == NULL)
	{
		LOG(LM_GFX, LL_ERROR, "cannot borrow texture");
		return false;
	}
#else
	p->Tex = TextureCreate(
		gGraphicsDevice.gameWindow.renderer, SDL_TEXTUREACCESS_STATIC,
						   size, SDL_BLENDMODE_NONE, 255);
	if (p->Tex == NULL)
	{
		LOG(LM_GFX, LL_ERROR, "cannot create texture: %s", SDL_GetError());
		return false;
	}
	if (SDL_UpdateTexture(
		p->Tex, NULL, p->Data, size.x * sizeof(Uint32)) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot update texture: %s", SDL_GetError());
		return false;
	}
	if (SDL_SetTextureBlendMode(p->Tex, SDL_BLENDMODE_BLEND) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot set texture blend mode: %s",
			SDL_GetError());
		return false;
	}
#endif
	LOG(LM_GFX, LL_TRACE, "made texture %p data(%p) count(%d)",
		p->Tex, p->Data, hashmap_length(textureDebugger));
	if (LL_TRACE >= LogModuleGetLevel(LM_GFX))
	{
		char key[32];
		sprintf(key, "%p", p->Tex);
		if (hashmap_get(textureDebugger, key, NULL) != MAP_MISSING)
		{
			LOG(LM_GFX, LL_TRACE, "Error: repeated texture loc");
		}
		if (hashmap_put(textureDebugger, key, (any_t)0) != MAP_OK)
		{
			LOG(LM_GFX, LL_TRACE, "Error: cannot add texture to debugger");
		}
	}
	return true;
}

// Note: does not copy the texture
Pic PicCopy(const Pic *src)
{
	Pic p = *src;
	const struct vec2i psize = PicPixelSize(src);
	const size_t size = (size_t)psize.x * psize.y * PicPxBytes(src);
	CMALLOC(p.Data, size);
	memcpy(p.Data, src->Data, size);
	p.Channels = NULL;
	if (src->Channels != NULL)
	{
		const size_t channelsSize = PicChannelsBytes(psize.x * psize.y);
		CMALLOC(p.Channels, channelsSize);
		memcpy(p.Channels, src->Channels, channelsSize);
	}
#ifdef PICOS
	g_picos_pic_data_bytes += size;
	g_picos_pic_count++;
	picos_gfx_bytes_peak_sample();
#endif
	p.Tex = NULL;
	p.isHD = src->isHD;
	return p;
}

void PicFree(Pic *pic)
{
	if (pic->Tex != NULL)
	{
		LOG(LM_GFX, LL_TRACE, "freeing texture %p data(%p)", pic->Tex, pic->Data);
		SDL_DestroyTexture(pic->Tex);
		if (LL_TRACE >= LogModuleGetLevel(LM_GFX))
		{
			char key[32];
			sprintf(key, "%p", pic->Tex);
			if (hashmap_get(textureDebugger, key, NULL) == MAP_OK)
			{
				if (hashmap_remove(textureDebugger, key) != MAP_OK)
				{
					LOG(LM_GFX, LL_TRACE, "Error: cannot remove tex from debugger");
				}
				else
				{
					LOG(LM_GFX, LL_TRACE, "Texture count: %d",
						hashmap_length(textureDebugger));
				}
			}
			else
			{
				LOG(LM_GFX, LL_TRACE, "Error: destroying unknown texture");
			}
		}
	}
#ifdef PICOS
	if (pic->Data != NULL)
	{
		const struct vec2i dataSize = PicPixelSize(pic);
		g_picos_pic_data_bytes -=
			(size_t)dataSize.x * dataSize.y * PicPxBytes(pic);
		g_picos_pic_count--;
	}
#endif
	pic->size = svec2i_zero();
	CFREE(pic->Data);
	pic->Data = NULL;
	CFREE(pic->Channels);
	pic->Channels = NULL;
}

bool PicIsNone(const Pic *pic)
{
	return pic->size.x == 0 || pic->size.y == 0 || pic->Data == NULL;
}

void PicTrim(Pic *pic, const bool xTrim, const bool yTrim)
{
	// Scan all pixels looking for the min/max of x and y
	const struct vec2i size = PicPixelSize(pic);
	struct vec2i min = size;
	struct vec2i max = svec2i_zero();
	for (struct vec2i pos = svec2i_zero(); pos.y < size.y; pos.y++)
	{
		for (pos.x = 0; pos.x < size.x; pos.x++)
		{
			const int idx = pos.x + pos.y * size.x;
			if (!PicPxTransparent(pic, idx))
			{
				min.x = MIN(min.x, pos.x);
				min.y = MIN(min.y, pos.y);
				max.x = MAX(max.x, pos.x);
				max.y = MAX(max.y, pos.y);
			}
		}
	}
	// If no opaque pixels found, don't trim
	struct vec2i newSize = size;
	struct vec2i offset = svec2i_zero();
	if (min.x < max.x && min.y < max.y)
	{
		if (xTrim)
		{
			newSize.x = max.x - min.x + 1;
			offset.x = min.x;
		}
		if (yTrim)
		{
			newSize.y = max.y - min.y + 1;
			offset.y = min.y;
		}
	}
	PicShrink(pic, newSize, offset);
}
void PicShrink(Pic *pic, const struct vec2i size, const struct vec2i offset)
{
	// Trim by copying pixels
	void *newData;
	const size_t bpp = PicPxBytes(pic);
	CMALLOC(newData, (size_t)size.x * size.y * bpp);
	if (newData == NULL)
	{
		return;
	}
	// Wrap the new buffer in a same-format Pic so PicPxCopy can move raw
	// pixels into it; only fmt/Data are read by PicPxCopy.
	Pic newPic;
	memset(&newPic, 0, sizeof newPic);
	newPic.fmt = pic->fmt;
	newPic.Data = newData;
	uint8_t *newChannels = NULL;
	if (pic->Channels != NULL)
	{
		CMALLOC(newChannels, PicChannelsBytes(size.x * size.y));
	}
	for (struct vec2i pos = svec2i_zero(); pos.y < size.y; pos.y++)
	{
		for (pos.x = 0; pos.x < size.x; pos.x++)
		{
			const int dstIdx = pos.x + pos.y * size.x;
			const int srcIdx =
				pos.x + offset.x + (pos.y + offset.y) * pic->size.x;
			PicPxCopy(&newPic, dstIdx, pic, srcIdx);
			if (newChannels != NULL)
			{
				PicChannelSet(
					newChannels, dstIdx,
					PicChannelGet(pic->Channels, srcIdx));
			}
		}
	}
	// Replace the old data
#ifdef PICOS
	{
		const struct vec2i oldSize = PicPixelSize(pic);
		g_picos_pic_data_bytes -=
			(size_t)oldSize.x * oldSize.y * PicPxBytes(pic);
		g_picos_pic_data_bytes += (size_t)size.x * size.y * bpp;
		picos_gfx_bytes_peak_sample();
	}
#endif
	CFREE(pic->Data);
	pic->Data = newData;
	CFREE(pic->Channels);
	pic->Channels = newChannels;
	pic->size = size;
	if (pic->isHD)
	{
		pic->size = svec2i_scale_divide(pic->size, 2);
	}
	pic->offset = svec2i_zero();
	PicTryMakeTex(pic);
}

color_t PicGetRandomColor(const Pic *p)
{
	// Get a random non-transparent pixel from the pic
	const struct vec2i size = PicPixelSize(p);
	for (;;)
	{
		const int i = rand() % (size.x * size.y);
		const color_t c = PicPx(p, i);
		if (c.a > 0)
		{
			return c;
		}
	}
}

void PicRender(
	const Pic *p, SDL_Renderer *r, const struct vec2i pos, const color_t mask,
	const double radians, const struct vec2 scale, const SDL_RendererFlip flip,
	const Rect2i srcRect)
{
	Rect2i src = Rect2iNew(
		svec2i_max(srcRect.Pos, svec2i_zero()), svec2i_zero()
	);
	const struct vec2i srcSize = PicPixelSize(p);
	src.Size = svec2i_is_zero(srcRect.Size) ? srcSize :
		svec2i_min(svec2i_subtract(srcRect.Size, src.Pos), srcSize);
	Rect2i dest = Rect2iNew(pos, src.Size);
	// Apply scale to render dest
	const bool unscaled = svec2_is_equal(scale, svec2_one());
	const struct vec2 destScale = svec2_scale(scale, p->isHD ? 0.5f : 1);
	if (!unscaled)
	{
		dest.Pos.x -= (mint_t)MROUND((destScale.x - 1) * src.Size.x / 2);
		dest.Pos.y -= (mint_t)MROUND((destScale.y - 1) * src.Size.y / 2);
		dest.Size.x = (mint_t)MROUND(src.Size.x * destScale.x);
		dest.Size.y = (mint_t)MROUND(src.Size.y * destScale.y);
	}
	else if (p->isHD)
	{
		dest.Size.x = (mint_t)MROUND(src.Size.x * destScale.x);
		dest.Size.y = (mint_t)MROUND(src.Size.y * destScale.y);
	}
	const double angle = ToDegrees(radians);
	TextureRender(p->Tex, r, src, dest, mask, angle, flip);
}
