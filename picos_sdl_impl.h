/*
    PicOS SDL2 Implementation — Internal Header
    Software renderer for C-Dogs SDL on PicOS
*/
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "picos_sdl.h"

/* Forward declare the PicOS API type */
struct PicoCalcAPI;

/* ── Pixel formats ───────────────────────────────────────────────
   The shim renders in RGB565 (host byte order — display->drawImageNN
   byte-swaps to the panel's big-endian order itself).  Textures that
   BORROW Pic->Data keep aliasing ARGB8888, because Pic->Data stays
   ARGB8888 until sub-project 2C; textures the shim OWNS are RGB565. */
#define PICOS_TEXFMT_ARGB8888  0   /* 4 bytes/px, borrowed Pic->Data  */
#define PICOS_TEXFMT_RGB565    1   /* 2 bytes/px, shim-owned          */

/* RGB565 has no alpha channel.  This sentinel (pure magenta: r=31,
   g=0, b=31) marks a transparent pixel.  It decodes to exactly the
   (r,g,b,a) = (0,0,0,0) that an ARGB alpha-0 pixel produced, so the
   blended path and the opaque path both behave as they did in 32-bit. */
#define PICOS_RGB565_CKEY  ((uint16_t)0xF81F)

/* Pack 8-bit channels into host-order RGB565, nudging away from the
   colour key so an opaque magenta never reads back as transparent. */
static inline uint16_t picos_pack565(uint32_t r, uint32_t g, uint32_t b) {
    uint16_t p = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
    if (p == PICOS_RGB565_CKEY) p = (uint16_t)(PICOS_RGB565_CKEY - 1);
    return p;
}

/* ARGB8888 -> RGB565.  Alpha is thresholded at 128: the design measured
   99.76% of source pixels as fully transparent or fully opaque, and the
   one buffer the shim converts wholesale (g->buf) is provably binary —
   BlitClearBuf memsets it to 0 and BlitFillBuf writes opaque colours,
   and nothing else writes it. */
static inline uint16_t picos_argb_to_565(uint32_t argb) {
    if (((argb >> 24) & 0xFF) < 128) return PICOS_RGB565_CKEY;
    return picos_pack565((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF);
}

/* RGB565 -> 8-bit channels, replicating high bits into the low ones so
   full-scale stays full-scale (31 -> 255, not 248). */
static inline void picos_unpack565(uint16_t p, uint32_t *r, uint32_t *g,
                                   uint32_t *b) {
    const uint32_t r5 = (p >> 11) & 0x1Fu;
    const uint32_t g6 = (p >> 5) & 0x3Fu;
    const uint32_t b5 = p & 0x1Fu;
    *r = (r5 << 3) | (r5 >> 2);
    *g = (g6 << 2) | (g6 >> 4);
    *b = (b5 << 3) | (b5 >> 2);
}

/* ── Internal texture structure ──────────────────────────────── */
typedef struct PicosTexture {
    int w, h, pitch;       /* pitch in bytes: w*4 ARGB8888, w*2 RGB565 */
    void *pixels;          /* PICOS_TEXFMT_* selects how to read these */
    uint8_t fmt;           /* PICOS_TEXFMT_ARGB8888 | PICOS_TEXFMT_RGB565 */
    int access;            /* SDL_TEXTUREACCESS_* */
    uint8_t r_mod, g_mod, b_mod;  /* color mod */
    uint8_t a_mod;         /* alpha mod */
    int blend_mode;
    bool owns_pixels;      /* free pixels on destroy? */
    bool locked;
} PicosTexture;

/* ── Internal renderer structure ─────────────────────────────── */
typedef struct PicosRenderer {
    uint16_t *framebuf;        /* default render target (RGB565, host order) */
    int fb_w, fb_h;            /* framebuffer dimensions */
    uint8_t draw_r, draw_g, draw_b, draw_a;
    int draw_blend_mode;
    PicosTexture *render_target;  /* non-NULL = rendering to texture */
    int logical_w, logical_h;
} PicosRenderer;

/* ── Event queue ─────────────────────────────────────────────── */
#define PICOS_EVENT_QUEUE_SIZE 32

/* ── Initialize the PicOS SDL layer ──────────────────────────── */
void picos_sdl_init(const struct PicoCalcAPI *api);

/* ── Access to global renderer (for SDL_RenderPresent) ───────── */
extern const struct PicoCalcAPI *g_picos_api;
