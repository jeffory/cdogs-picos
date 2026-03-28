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

/* ── Internal texture structure ──────────────────────────────── */
typedef struct PicosTexture {
    int w, h, pitch;       /* pitch in bytes */
    uint32_t *pixels;      /* ARGB8888 pixel data */
    int access;            /* SDL_TEXTUREACCESS_* */
    uint8_t r_mod, g_mod, b_mod;  /* color mod */
    uint8_t a_mod;         /* alpha mod */
    int blend_mode;
    bool owns_pixels;      /* free pixels on destroy? */
    bool locked;
} PicosTexture;

/* ── Internal renderer structure ─────────────────────────────── */
typedef struct PicosRenderer {
    uint32_t *framebuf;        /* default render target (ARGB8888) */
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
