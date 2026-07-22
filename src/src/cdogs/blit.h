/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2014, 2016-2020, 2023 Cong Xu
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

#include <SDL_stdinc.h>

#include "grafx.h"
#include "pic.h"
#include "vector.h"

typedef struct
{
	color_t Skin;
	color_t Arms;
	color_t Body;
	color_t Legs;
	color_t Hair;
	color_t Feet;
	color_t Facehair;
	color_t Hat;
	color_t Glasses;
} CharColors;
typedef enum
{
	CHAR_COLOR_SKIN,
	CHAR_COLOR_ARMS,
	CHAR_COLOR_BODY,
	CHAR_COLOR_LEGS,
	CHAR_COLOR_HAIR,
	CHAR_COLOR_FEET,
	CHAR_COLOR_FACEHAIR,
	CHAR_COLOR_HAT,
	CHAR_COLOR_GLASSES,
	CHAR_COLOR_COUNT
} CharColorType;
color_t *CharColorGetByType(CharColors *c, const CharColorType t);

void BlitClearBuf(GraphicsDevice *g);
void BlitFillBuf(GraphicsDevice *g, const color_t c);
void BlitUpdateFromBuf(GraphicsDevice *g, SDL_Texture *t);

uint8_t CharColorTypeAlpha(const CharColorType t);
CharColorType CharColorTypeFromColor(const color_t c, const CharColorType headPartColor);
// Amendment B (cdogs Stage 2C pic-formats plan, 2026-07-22): classifies a
// single opaque chars/ pixel for PicLoad's per-pic format pre-pass (pic.c).
// CharColorTypeFromColor alone can't distinguish "near-grey, not a colour
// key" from "genuinely chromatic, not a colour key" -- both return
// CHAR_COLOR_COUNT -- but that distinction is exactly what decides whether a
// chars/ pic can go LA8 (loses no colour: nothing chromatic-but-unkeyed is
// in it) or must keep real RGB (gun accents, hat decorations, the explosion
// fire palette). Reuses CharColorTypeFromColor's own CHAR_COLOR_THRESHOLD so
// there is exactly one definition of "near-grey" in the codebase.
typedef enum
{
	CHAR_PIXEL_CLASS_GREY,        // COUNT and near-grey: safe to fold to LA8
	CHAR_PIXEL_CLASS_CHANNEL_KEY, // recognised colour key (SKIN/ARMS/etc.)
	CHAR_PIXEL_CLASS_CHROMATIC,   // COUNT but NOT near-grey ("chromatic-COUNT")
} CharPixelClass;
CharPixelClass CharColorClassifyPixel(
	const color_t c, const CharColorType headPartColor);
CharColors CharColorsFromOneColor(const color_t color);
color_t CharColorsGetChannelMask(const CharColors *c, const uint8_t alpha);
void CharColorsGetMaskedName(char *buf, const char *base, const CharColors *c);

#define BLIT_BRIGHTNESS_MIN (-10)
#define BLIT_BRIGHTNESS_MAX 10
