/*
    PicOS — per-blit CharColors LUT setter (Stage 2D, Task 1).

    Deliberately its OWN header rather than living in picos_sdl.h. CharColors
    is an anonymous-struct typedef (blit.h) with no struct tag, so it cannot
    be forward-declared -- any header exposing this prototype by value must
    see blit.h's real definition. blit.c's very first line is
    `#include "blit.h"` (before <SDL.h>), so if picos_sdl.h itself pulled in
    blit.h, that translation unit would re-enter blit.h's own #pragma-once
    guard through the nested SDL.h -> picos_sdl.h -> blit.h chain *before*
    blit.h's own includes finish and the CharColors typedef is reached --
    a real ordering cycle, not just a theoretical one. Keeping the
    declaration here avoids it: this header is only pulled in by
    picos_sdl_impl.c (after picos_sdl.h is already fully parsed, so no
    cycle) and, later, by Task 2's cdogs-side caller(s), which already see
    CharColors via blit.h and can include this at zero extra cost.
*/
#pragma once

#include "blit.h"

/* Sets/clears the CharColors applied per pixel to LA8/ARGB8888 char-sprite
   sources by SDL_RenderCopyEx. Builds a 256-entry colour LUT once per call
   by invoking the real CharColorsGetChannelMask(colors, i) for i in 0..255
   (never duplicates its channel-index mapping). Passing NULL clears --
   SDL_RenderCopyEx's LA8/ARGB8888 branches then behave exactly as they did
   before this mechanism existed. */
void PicosBlitSetCharColors(const CharColors *colors);
