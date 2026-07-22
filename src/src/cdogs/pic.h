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
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <SDL_render.h>

#include "vector.h"

// Pixel storage format of a Pic's Data buffer.
// Stage 2C introduces this so individual pics can move off ARGB8888 to save
// memory; this task (2C-1) keeps every pic ARGB8888 and only routes access
// through the accessors below, so behaviour is unchanged.
typedef enum
{
	PIC_FMT_ARGB8888 = 0, // 4 B/px -- desktop always; PICOS until converted
	PIC_FMT_RGB565 = 1,   // 2 B/px + PICOS_RGB565_CKEY transparency
	PIC_FMT_LA8 = 2,      // 2 B/px: low byte L, high byte A (channel index); 0x0000 = transparent
} PicFormat;

typedef struct
{
	struct vec2i size;
	struct vec2i offset;
	bool isHD;
	uint8_t fmt;       // PicFormat; desktop build keeps PIC_FMT_ARGB8888
	void *Data;        // Uint32* (ARGB8888) or uint16_t* (RGB565/LA8)
	uint8_t *Channels; // style pics only (Task 3): 2-bit/px packed map; else NULL
	SDL_Texture *Tex;
} Pic;

color_t PixelToColor(
	const SDL_PixelFormat *f, const Uint8 aShift, const Uint32 pixel);
Uint32 ColorToPixel(
	const SDL_PixelFormat *f, const Uint8 aShift, const color_t color);
#define PIXEL2COLOR(_p) \
	PixelToColor(gGraphicsDevice.Format, gGraphicsDevice.Format->Ashift, _p)
#define COLOR2PIXEL(_c) \
	ColorToPixel(gGraphicsDevice.Format, gGraphicsDevice.Format->Ashift, _c)

// Format-aware pixel accessors -- every read/write of a Pic's Data buffer
// must go through these (never index Data directly) so a pic's storage
// format can change without touching call sites. Only these accessors'
// implementations (pic.c) are allowed to branch on p->fmt.
size_t PicPxBytes(const Pic *p);                          // bytes per pixel: 4 or 2
color_t PicPx(const Pic *p, int i);                        // format-aware read
void PicPxSet(Pic *p, int i, color_t c);                   // format-aware write
bool PicPxTransparent(const Pic *p, int i);                // whole-word 0 / CKEY / A==0
void PicPxCopy(Pic *dst, int di, const Pic *src, int si);  // raw same-format copy

void PicLoad(
	Pic *p, const struct vec2i size, const struct vec2i offset,
	const SDL_Surface *image, const bool isHD);
bool PicTryMakeTex(Pic *p);
Pic PicCopy(const Pic *src);
void PicFree(Pic *pic);
bool PicIsNone(const Pic *pic);

// Detect unused edges and update size and offset to fit
void PicTrim(Pic *pic, const bool xTrim, const bool yTrim);
void PicShrink(Pic *pic, const struct vec2i size, const struct vec2i offset);

color_t PicGetRandomColor(const Pic *p);

void PicRender(
	const Pic *p, SDL_Renderer *r, const struct vec2i pos, const color_t mask,
	const double radians, const struct vec2 scale, const SDL_RendererFlip flip,
	const Rect2i src);
