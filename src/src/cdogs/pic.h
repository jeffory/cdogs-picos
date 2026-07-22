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
// memory. "Final" pics (everything that isn't chars/ or a
// wall|tile|door|exits|keys style pic) and style pics are always
// PIC_FMT_RGB565. chars/ pics are the exception: PicLoad decides their
// format itself, per pic, from a load-time pre-pass over the exact ARGB8888
// pixels (PicLoadClassifyCharsFormat, pic.c) -- most land on PIC_FMT_LA8
// (lossless for them), a few keep PIC_FMT_RGB565 or full PIC_FMT_ARGB8888
// where real, unkeyed colour would otherwise be lost (Amendment B to the
// cdogs Stage 2C pic-formats plan). See PicManagerAdd's classification
// (pic_manager.c, a placeholder for chars/) and font.c (always RGB565).
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

// Per-pixel classification packed into Pic::Channels (2 bits/px, 4 px/byte),
// computed once at load time (PicLoad) from the exact ARGB8888 surface
// pixels, before any lossy format conversion. Lets
// PicManagerGenerateMaskedPic (pic_manager.c) recover the classification
// that its RGB565-quantized channel tests can no longer compute correctly
// after round-tripping through RGB565. Allocated only for style-prefixed
// pics (wall/tile/door/exits/keys); Channels == NULL elsewhere, in which
// case every pixel reads as PIC_CH_LITERAL.
enum
{
	PIC_CH_LITERAL = 0,
	PIC_CH_PRIMARY = 1,
	PIC_CH_ALT = 2,
	PIC_CH_ALT_GRAY = 3,
};
int PicChannelGet(const Pic *p, int i);
void PicChannelSet(Pic *p, int i, int ch);
void PicChannelsFree(Pic *p); // frees/NULLs Channels only, e.g. for masked outputs

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

// `charHeadPart` is a `CharColorType` value (see blit.h) or -1.  Pass -1 for
// every non-chars/ pic (font.c, style pics, "final" pics): PicLoad skips the
// colour-key classification entirely, stores the source pixel verbatim, and
// uses `fmt` exactly as given. For chars/ pics (pic_manager.c's
// PicManagerAdd), pass the head-part base colour (HAIR, or the
// FACEHAIR/HAT/GLASSES override for those sub-prefixes) -- `fmt` is then
// IGNORED: PicLoad decides the real format itself (Amendment B) from a
// pre-pass over the exact ARGB8888 pixels, because pure LA8 (one luma byte)
// would grey out chromatic pixels the colour-key classifier can't name (gun
// accents, hat decorations, the explosion fire palette). Whatever format it
// picks, PicLoad reproduces -- once, at load time -- what used to be a
// separate post-load pass over the pic: classify each opaque pixel's colour
// key (CharColorTypeFromColor) and write back grey + a special
// "channel index" alpha (CharColorTypeAlpha), UNLESS the pixel is
// genuinely chromatic and unkeyed, in which case its real colour is kept
// (only representable when the resolved format is RGB565 or ARGB8888).
// pic.h stays free of blit.h (it would create an include cycle -- blit.h
// includes pic.h), hence the plain int rather than the CharColorType type
// itself.
void PicLoad(
	Pic *p, const struct vec2i size, const struct vec2i offset,
	const SDL_Surface *image, const bool isHD, const PicFormat fmt,
	const bool buildChannelMap, const int charHeadPart);
bool PicTryMakeTex(Pic *p);
Pic PicCopy(const Pic *src);
// Like PicCopy, but never allocates/copies a Channels map even if `src` has
// one -- for cache outputs that are cached-by-name finals, never re-masked,
// so a Channels map would just be dead weight (see PicManagerGenerateMaskedPic,
// pic_manager.c, which used to PicCopy + immediately PicChannelsFree its own
// copy before Stage 2D Task 3's cleanup).
Pic PicCopyNoChannels(const Pic *src);
// Like PicCopy, but the copy is converted to a different pixel format
// (per-pixel, via PicPx/PicPxSet) rather than a raw memcpy of same-format
// bytes. Used for cache outputs whose *destination* role calls for a
// different format than their source -- e.g. PicManagerGetCharSprites'
// per-CharColors recoloured sprite cache (desktop only as of Stage 2D; PICOS
// recolours at blit time instead -- see draw_actor.c), which reads an LA8
// source but stores a real-colour RGB565 final (see pic_manager.c). The
// output never carries a Channels map, same reasoning as PicCopyNoChannels
// above, since these are cached-by-name finals, never re-masked. Byte
// accounting is sized by the DESTINATION format, not the source's.
Pic PicCopyToFormat(const Pic *src, const PicFormat fmt);
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
