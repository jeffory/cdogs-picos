/*
    PicOS SDL2 Software Renderer Implementation
    Provides real SDL rendering for C-Dogs on PicOS 320×320 display.
    Resolution: 320×240, letterboxed (40px top/bottom black bars).
*/
#include "picos_sdl_impl.h"
#include "os.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Display layout ──────────────────────────────────────────── */
#define DISPLAY_W     320
#define DISPLAY_H     320
#define GAME_W        320
#define GAME_H        240
#define LETTERBOX_Y   ((DISPLAY_H - GAME_H) / 2)  /* 40 */

/* ── Globals ─────────────────────────────────────────────────── */
static PicosRenderer s_renderer;
static int s_renderer_valid = 0;
/* Reusable RGB565 buffer for SDL_RenderPresent */
static uint16_t *s_rgb565_buf = NULL;

/* Event queue */
static SDL_Event s_event_queue[PICOS_EVENT_QUEUE_SIZE];
static int s_event_head = 0;
static int s_event_tail = 0;

/* Keyboard state array */
static Uint8 s_key_state[SDL_NUM_SCANCODES];

/* Static pixel format for ARGB8888 */
static SDL_PixelFormat s_argb8888_format = {
    .format = SDL_PIXELFORMAT_ARGB8888,
    .BitsPerPixel = 32,
    .BytesPerPixel = 4,
    .Rmask = 0x00FF0000,
    .Gmask = 0x0000FF00,
    .Bmask = 0x000000FF,
    .Amask = 0xFF000000,
    .Rshift = 16, .Gshift = 8, .Bshift = 0, .Ashift = 24,
    .Rloss = 0, .Gloss = 0, .Bloss = 0, .Aloss = 0
};

/* ── Helper: get current render target ───────────────────────── */
static inline uint32_t *get_target(PicosRenderer *r, int *w, int *h) {
    if (r->render_target) {
        *w = r->render_target->w;
        *h = r->render_target->h;
        return r->render_target->pixels;
    }
    *w = r->fb_w;
    *h = r->fb_h;
    return r->framebuf;
}

/* ── Helper: clamp ───────────────────────────────────────────── */
static inline int clamp_i(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/* ── Helper: alpha blend a single pixel ──────────────────────── */
static inline uint32_t blend_pixel(uint32_t dst, uint32_t src_r, uint32_t src_g,
                                   uint32_t src_b, uint32_t src_a) {
    if (src_a == 0) return dst;
    if (src_a == 255) return (0xFF000000 | (src_r << 16) | (src_g << 8) | src_b);
    uint32_t inv_a = 255 - src_a;
    uint32_t dr = (dst >> 16) & 0xFF;
    uint32_t dg = (dst >> 8) & 0xFF;
    uint32_t db = dst & 0xFF;
    uint32_t or_ = (src_r * src_a + dr * inv_a) / 255;
    uint32_t og = (src_g * src_a + dg * inv_a) / 255;
    uint32_t ob = (src_b * src_a + db * inv_a) / 255;
    return 0xFF000000 | (or_ << 16) | (og << 8) | ob;
}

/* ================================================================
   SDL INIT / QUIT
   ================================================================ */

void picos_sdl_init(const struct PicoCalcAPI *api) {
    (void)api;
    memset(&s_renderer, 0, sizeof(s_renderer));
    memset(s_key_state, 0, sizeof(s_key_state));
    s_event_head = s_event_tail = 0;
}

/* ================================================================
   WINDOW
   ================================================================ */

static int s_window_exists = 0;

SDL_Window *SDL_CreateWindow(const char *t, int x, int y, int w, int h, Uint32 f) {
    (void)t; (void)x; (void)y; (void)w; (void)h; (void)f;
    s_window_exists = 1;
    return (SDL_Window *)(uintptr_t)1;  /* non-NULL sentinel */
}

void SDL_DestroyWindow(SDL_Window *w) {
    (void)w;
    s_window_exists = 0;
}

void SDL_GetWindowSize(SDL_Window *w, int *pw, int *ph) {
    (void)w;
    if (pw) *pw = GAME_W;
    if (ph) *ph = GAME_H;
}

/* ================================================================
   RENDERER
   ================================================================ */

SDL_Renderer *SDL_CreateRenderer(SDL_Window *w, int index, Uint32 flags) {
    (void)w; (void)index; (void)flags;
    if (s_renderer_valid) return (SDL_Renderer *)&s_renderer;

    s_renderer.fb_w = GAME_W;
    s_renderer.fb_h = GAME_H;
    s_renderer.framebuf = calloc(GAME_W * GAME_H, sizeof(uint32_t));
    if (!s_renderer.framebuf) return NULL;
    s_renderer.logical_w = GAME_W;
    s_renderer.logical_h = GAME_H;
    s_renderer.draw_r = s_renderer.draw_g = s_renderer.draw_b = 0;
    s_renderer.draw_a = 255;
    s_renderer.render_target = NULL;
    s_renderer_valid = 1;

    /* Allocate RGB565 present buffer */
    if (!s_rgb565_buf) {
        s_rgb565_buf = calloc(GAME_W * GAME_H, sizeof(uint16_t));
    }

    return (SDL_Renderer *)&s_renderer;
}

void SDL_DestroyRenderer(SDL_Renderer *r) {
    if (r == (SDL_Renderer *)&s_renderer && s_renderer_valid) {
        free(s_renderer.framebuf);
        s_renderer.framebuf = NULL;
        free(s_rgb565_buf);
        s_rgb565_buf = NULL;
        s_renderer_valid = 0;
    }
}

int SDL_SetRenderDrawColor(SDL_Renderer *r, Uint8 red, Uint8 g, Uint8 b, Uint8 a) {
    PicosRenderer *pr = (PicosRenderer *)r;
    pr->draw_r = red; pr->draw_g = g; pr->draw_b = b; pr->draw_a = a;
    return 0;
}

int SDL_RenderClear(SDL_Renderer *r) {
    PicosRenderer *pr = (PicosRenderer *)r;
    int tw, th;
    uint32_t *target = get_target(pr, &tw, &th);
    uint32_t color = (0xFF000000 | ((uint32_t)pr->draw_r << 16) |
                     ((uint32_t)pr->draw_g << 8) | pr->draw_b);
    int count = tw * th;
    /* Fill with actual color (opaque black = 0xFF000000, not 0x00000000) */
    for (int i = 0; i < count; i++) target[i] = color;
    return 0;
}

int SDL_RenderSetLogicalSize(SDL_Renderer *r, int w, int h) {
    PicosRenderer *pr = (PicosRenderer *)r;
    pr->logical_w = w;
    pr->logical_h = h;
    return 0;
}

void SDL_RenderGetLogicalSize(SDL_Renderer *r, int *w, int *h) {
    PicosRenderer *pr = (PicosRenderer *)r;
    if (w) *w = pr->logical_w;
    if (h) *h = pr->logical_h;
}

int SDL_SetRenderTarget(SDL_Renderer *r, SDL_Texture *t) {
    PicosRenderer *pr = (PicosRenderer *)r;
    pr->render_target = (PicosTexture *)t;
    return 0;
}

SDL_Texture *SDL_GetRenderTarget(SDL_Renderer *r) {
    PicosRenderer *pr = (PicosRenderer *)r;
    return (SDL_Texture *)pr->render_target;
}

int SDL_SetRenderDrawBlendMode(SDL_Renderer *r, SDL_BlendMode m) {
    PicosRenderer *pr = (PicosRenderer *)r;
    pr->draw_blend_mode = m;
    return 0;
}

int SDL_GetRendererOutputSize(SDL_Renderer *r, int *w, int *h) {
    (void)r;
    if (w) *w = GAME_W;
    if (h) *h = GAME_H;
    return 0;
}

int SDL_GetRendererInfo(SDL_Renderer *r, SDL_RendererInfo *info) {
    (void)r;
    if (info) {
        memset(info, 0, sizeof(*info));
        info->name = "picos";
        info->flags = SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE;
        info->num_texture_formats = 1;
        info->texture_formats[0] = SDL_PIXELFORMAT_ARGB8888;
        info->max_texture_width = 1024;
        info->max_texture_height = 1024;
    }
    return 0;
}

/* ================================================================
   RENDER PRESENT — ARGB8888 → RGB565 → PicOS display
   ================================================================ */

void SDL_RenderPresent(SDL_Renderer *r) {
    PicosRenderer *pr = (PicosRenderer *)r;
    if (!pr->framebuf || !s_rgb565_buf || !g_picos_api) return;

    /* Debug: count non-black pixels (anything not 0xFF000000 or 0x00000000) */
    static int s_present_count = 0;
    s_present_count++;
    if (s_present_count <= 8) {
        int colored = 0;
        int first_idx = -1;
        uint32_t first_val = 0;
        int total = pr->fb_w * pr->fb_h;
        for (int i = 0; i < total; i++) {
            uint32_t px = pr->framebuf[i];
            if (px != 0 && px != 0xFF000000) {
                if (first_idx < 0) {
                    first_idx = i;
                    first_val = px;
                }
                colored++;
            }
        }
        fprintf(stderr, "RenderPresent #%d: %dx%d colored=%d/%d first@(%d,%d)=0x%08X\n",
                s_present_count, pr->fb_w, pr->fb_h, colored, total,
                first_idx >= 0 ? first_idx % pr->fb_w : -1,
                first_idx >= 0 ? first_idx / pr->fb_w : -1,
                first_val);
    }

    /* Convert ARGB8888 → RGB565 big-endian (matching PicOS ST7365P display) */
    const uint32_t *src = pr->framebuf;
    uint16_t *dst = s_rgb565_buf;
    int count = pr->fb_w * pr->fb_h;
    for (int i = 0; i < count; i++) {
        uint32_t px = src[i];
        uint32_t r_ = (px >> 16) & 0xFF;
        uint32_t g_ = (px >> 8) & 0xFF;
        uint32_t b_ = px & 0xFF;
        uint16_t rgb565 = (uint16_t)(((r_ >> 3) << 11) | ((g_ >> 2) << 5) | (b_ >> 3));
        /* Byte-swap to big-endian for PicOS display format */
        dst[i] = (rgb565 >> 8) | (rgb565 << 8);
    }

    /* Blit to PicOS display, centered vertically */
    g_picos_api->display->drawImageNN(0, LETTERBOX_Y, s_rgb565_buf,
                                       pr->fb_w, pr->fb_h, 1);
    g_picos_api->display->flush();

    /* Let PicOS process system events */
    g_picos_api->sys->poll();
}

/* ================================================================
   TEXTURE
   ================================================================ */

SDL_Texture *SDL_CreateTexture(SDL_Renderer *r, Uint32 format, int access,
                               int w, int h) {
    (void)r; (void)format;
    PicosTexture *t = calloc(1, sizeof(PicosTexture));
    if (!t) return NULL;
    t->w = w;
    t->h = h;
    t->pitch = w * 4;
    t->access = access;
    t->r_mod = t->g_mod = t->b_mod = 255;
    t->a_mod = 255;
    t->blend_mode = SDL_BLENDMODE_BLEND;
    t->pixels = calloc(w * h, sizeof(uint32_t));
    if (!t->pixels) { free(t); return NULL; }
    t->owns_pixels = true;
    return (SDL_Texture *)t;
}

SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *r, SDL_Surface *s) {
    if (!s) return NULL;
    SDL_Texture *tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STATIC, s->w, s->h);
    if (tex && s->pixels) {
        SDL_UpdateTexture(tex, NULL, s->pixels, s->pitch);
    }
    return tex;
}

void SDL_DestroyTexture(SDL_Texture *t) {
    PicosTexture *pt = (PicosTexture *)t;
    if (!pt) return;
    if (pt->owns_pixels) free(pt->pixels);
    free(pt);
}

int SDL_UpdateTexture(SDL_Texture *t, const SDL_Rect *rect, const void *pixels,
                      int pitch) {
    PicosTexture *pt = (PicosTexture *)t;
    if (!pt || !pt->pixels || !pixels) return -1;

    int dx = rect ? rect->x : 0;
    int dy = rect ? rect->y : 0;
    int dw = rect ? rect->w : pt->w;
    int dh = rect ? rect->h : pt->h;

    for (int row = 0; row < dh; row++) {
        int ty = dy + row;
        if (ty < 0 || ty >= pt->h) continue;
        const uint8_t *src_row = (const uint8_t *)pixels + row * pitch;
        uint32_t *dst_row = pt->pixels + ty * pt->w + dx;
        int copy_w = dw;
        if (dx + copy_w > pt->w) copy_w = pt->w - dx;
        if (copy_w > 0)
            memcpy(dst_row, src_row, copy_w * sizeof(uint32_t));
    }
    return 0;
}

int SDL_LockTexture(SDL_Texture *t, const SDL_Rect *rect, void **pixels,
                    int *pitch) {
    PicosTexture *pt = (PicosTexture *)t;
    if (!pt || !pt->pixels) return -1;
    int x = rect ? rect->x : 0;
    int y = rect ? rect->y : 0;
    if (pixels) *pixels = pt->pixels + y * pt->w + x;
    if (pitch) *pitch = pt->pitch;
    pt->locked = true;
    return 0;
}

void SDL_UnlockTexture(SDL_Texture *t) {
    PicosTexture *pt = (PicosTexture *)t;
    if (pt) pt->locked = false;
}

int SDL_SetTextureBlendMode(SDL_Texture *t, SDL_BlendMode m) {
    PicosTexture *pt = (PicosTexture *)t;
    if (!pt) return -1;
    pt->blend_mode = m;
    return 0;
}

int SDL_SetTextureAlphaMod(SDL_Texture *t, Uint8 a) {
    PicosTexture *pt = (PicosTexture *)t;
    if (!pt) return -1;
    pt->a_mod = a;
    return 0;
}

int SDL_SetTextureColorMod(SDL_Texture *t, Uint8 r, Uint8 g, Uint8 b) {
    PicosTexture *pt = (PicosTexture *)t;
    if (!pt) return -1;
    pt->r_mod = r; pt->g_mod = g; pt->b_mod = b;
    return 0;
}

int SDL_QueryTexture(SDL_Texture *t, Uint32 *format, int *access,
                     int *w, int *h) {
    PicosTexture *pt = (PicosTexture *)t;
    if (!pt) return -1;
    if (format) *format = SDL_PIXELFORMAT_ARGB8888;
    if (access) *access = pt->access;
    if (w) *w = pt->w;
    if (h) *h = pt->h;
    return 0;
}

/* ================================================================
   RENDER OPERATIONS — software blitting
   ================================================================ */

int SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *srcrect,
                   const SDL_Rect *dstrect) {
    return SDL_RenderCopyEx(r, t, srcrect, dstrect, 0, NULL, SDL_FLIP_NONE);
}

int SDL_RenderCopyEx(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *srcrect,
                     const SDL_Rect *dstrect, double angle, const SDL_Point *center,
                     SDL_RendererFlip flip) {
    (void)angle; (void)center; /* rotation not supported yet */
    PicosRenderer *pr = (PicosRenderer *)r;
    PicosTexture *pt = (PicosTexture *)t;
    if (!pr || !pt || !pt->pixels) return -1;

    int tw, th;
    uint32_t *target = get_target(pr, &tw, &th);
    if (!target) return -1;

    /* Source rect */
    int sx = srcrect ? srcrect->x : 0;
    int sy = srcrect ? srcrect->y : 0;
    int sw = srcrect ? srcrect->w : pt->w;
    int sh = srcrect ? srcrect->h : pt->h;

    /* Dest rect */
    int dx = dstrect ? dstrect->x : 0;
    int dy = dstrect ? dstrect->y : 0;
    int dw = dstrect ? dstrect->w : tw;
    int dh = dstrect ? dstrect->h : th;

    if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0) return 0;

    /* Color/alpha mod */
    uint32_t rm = pt->r_mod, gm = pt->g_mod, bm = pt->b_mod;
    uint32_t am = pt->a_mod;
    bool do_color_mod = (rm != 255 || gm != 255 || bm != 255);
    bool do_blend = (pt->blend_mode == SDL_BLENDMODE_BLEND);

    /* Blit with scaling */
    for (int j = 0; j < dh; j++) {
        int ty = dy + j;
        if (ty < 0 || ty >= th) continue;

        /* Source Y with flip */
        int src_j = (flip & SDL_FLIP_VERTICAL) ? (dh - 1 - j) : j;
        int src_y = sy + src_j * sh / dh;
        if (src_y < 0 || src_y >= pt->h) continue;

        uint32_t *dst_row = target + ty * tw;
        const uint32_t *src_row = pt->pixels + src_y * pt->w;

        for (int i = 0; i < dw; i++) {
            int tx = dx + i;
            if (tx < 0 || tx >= tw) continue;

            /* Source X with flip */
            int src_i = (flip & SDL_FLIP_HORIZONTAL) ? (dw - 1 - i) : i;
            int src_x = sx + src_i * sw / dw;
            if (src_x < 0 || src_x >= pt->w) continue;

            uint32_t pixel = src_row[src_x];
            uint32_t pa = (pixel >> 24) & 0xFF;
            uint32_t pr_ = (pixel >> 16) & 0xFF;
            uint32_t pg = (pixel >> 8) & 0xFF;
            uint32_t pb = pixel & 0xFF;

            /* Apply color modulation */
            if (do_color_mod) {
                pr_ = (pr_ * rm) / 255;
                pg = (pg * gm) / 255;
                pb = (pb * bm) / 255;
            }

            /* Apply alpha modulation */
            pa = (pa * am) / 255;

            if (do_blend) {
                dst_row[tx] = blend_pixel(dst_row[tx], pr_, pg, pb, pa);
            } else {
                dst_row[tx] = 0xFF000000 | (pr_ << 16) | (pg << 8) | pb;
            }
        }
    }
    return 0;
}

int SDL_RenderFillRect(SDL_Renderer *r, const SDL_Rect *rect) {
    PicosRenderer *pr = (PicosRenderer *)r;
    int tw, th;
    uint32_t *target = get_target(pr, &tw, &th);
    if (!target) return -1;

    int x0 = rect ? rect->x : 0;
    int y0 = rect ? rect->y : 0;
    int x1 = rect ? (rect->x + rect->w) : tw;
    int y1 = rect ? (rect->y + rect->h) : th;
    x0 = clamp_i(x0, 0, tw);
    y0 = clamp_i(y0, 0, th);
    x1 = clamp_i(x1, 0, tw);
    y1 = clamp_i(y1, 0, th);

    uint32_t color = 0xFF000000 | ((uint32_t)pr->draw_r << 16) |
                     ((uint32_t)pr->draw_g << 8) | pr->draw_b;

    if (pr->draw_blend_mode == SDL_BLENDMODE_BLEND && pr->draw_a < 255) {
        for (int y = y0; y < y1; y++)
            for (int x = x0; x < x1; x++)
                target[y * tw + x] = blend_pixel(target[y * tw + x],
                    pr->draw_r, pr->draw_g, pr->draw_b, pr->draw_a);
    } else {
        for (int y = y0; y < y1; y++)
            for (int x = x0; x < x1; x++)
                target[y * tw + x] = color;
    }
    return 0;
}

int SDL_RenderDrawRect(SDL_Renderer *r, const SDL_Rect *rect) {
    if (!rect) return SDL_RenderFillRect(r, NULL);
    /* Draw 4 lines */
    SDL_RenderDrawLine(r, rect->x, rect->y, rect->x + rect->w - 1, rect->y);
    SDL_RenderDrawLine(r, rect->x, rect->y + rect->h - 1, rect->x + rect->w - 1, rect->y + rect->h - 1);
    SDL_RenderDrawLine(r, rect->x, rect->y, rect->x, rect->y + rect->h - 1);
    SDL_RenderDrawLine(r, rect->x + rect->w - 1, rect->y, rect->x + rect->w - 1, rect->y + rect->h - 1);
    return 0;
}

int SDL_RenderDrawPoint(SDL_Renderer *r, int x, int y) {
    PicosRenderer *pr = (PicosRenderer *)r;
    int tw, th;
    uint32_t *target = get_target(pr, &tw, &th);
    if (!target || x < 0 || y < 0 || x >= tw || y >= th) return -1;

    uint32_t color = 0xFF000000 | ((uint32_t)pr->draw_r << 16) |
                     ((uint32_t)pr->draw_g << 8) | pr->draw_b;

    if (pr->draw_blend_mode == SDL_BLENDMODE_BLEND && pr->draw_a < 255) {
        target[y * tw + x] = blend_pixel(target[y * tw + x],
            pr->draw_r, pr->draw_g, pr->draw_b, pr->draw_a);
    } else {
        target[y * tw + x] = color;
    }
    return 0;
}

int SDL_RenderDrawLine(SDL_Renderer *r, int x0, int y0, int x1, int y1) {
    /* Bresenham's line algorithm */
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        SDL_RenderDrawPoint(r, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return 0;
}

/* ================================================================
   SURFACE
   ================================================================ */

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth,
                                  Uint32 Rmask, Uint32 Gmask, Uint32 Bmask,
                                  Uint32 Amask) {
    (void)flags; (void)depth; (void)Rmask; (void)Gmask; (void)Bmask; (void)Amask;
    SDL_Surface *s = calloc(1, sizeof(SDL_Surface));
    if (!s) return NULL;
    s->w = w;
    s->h = h;
    s->pitch = w * 4;
    s->pixels = calloc(w * h, 4);
    if (!s->pixels) { free(s); return NULL; }
    s->format = &s_argb8888_format;
    s->refcount = 1;
    return s;
}

SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int w, int h, int depth,
                                       int pitch, Uint32 Rmask, Uint32 Gmask,
                                       Uint32 Bmask, Uint32 Amask) {
    (void)depth; (void)Rmask; (void)Gmask; (void)Bmask; (void)Amask;
    SDL_Surface *s = calloc(1, sizeof(SDL_Surface));
    if (!s) return NULL;
    s->w = w;
    s->h = h;
    s->pitch = pitch;
    s->pixels = pixels;
    s->format = &s_argb8888_format;
    s->flags = SDL_PREALLOC;  /* don't free pixels */
    s->refcount = 1;
    return s;
}

SDL_Surface *SDL_CreateRGBSurfaceWithFormat(Uint32 flags, int w, int h,
                                            int depth, Uint32 format) {
    return SDL_CreateRGBSurface(flags, w, h, depth, 0, 0, 0, 0);
}

SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *pixels, int w, int h,
                                                 int depth, int pitch,
                                                 Uint32 format) {
    return SDL_CreateRGBSurfaceFrom(pixels, w, h, depth, pitch, 0, 0, 0, 0);
}

void SDL_FreeSurface(SDL_Surface *s) {
    if (!s) return;
    s->refcount--;
    if (s->refcount <= 0) {
        if (s->pixels && !(s->flags & SDL_PREALLOC)) free(s->pixels);
        free(s);
    }
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, const SDL_PixelFormat *fmt,
                                Uint32 flags) {
    (void)fmt; (void)flags;
    if (!src) return NULL;
    SDL_Surface *dst = SDL_CreateRGBSurface(0, src->w, src->h, 32, 0, 0, 0, 0);
    if (dst && src->pixels) {
        for (int y = 0; y < src->h; y++) {
            memcpy((uint8_t *)dst->pixels + y * dst->pitch,
                   (uint8_t *)src->pixels + y * src->pitch,
                   src->w * 4);
        }
    }
    return dst;
}

SDL_Surface *SDL_ConvertSurfaceFormat(SDL_Surface *src, Uint32 fmt, Uint32 flags) {
    return SDL_ConvertSurface(src, NULL, flags);
}

int SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color) {
    if (!dst || !dst->pixels) return -1;
    int x0 = rect ? rect->x : 0;
    int y0 = rect ? rect->y : 0;
    int x1 = rect ? (rect->x + rect->w) : dst->w;
    int y1 = rect ? (rect->y + rect->h) : dst->h;
    x0 = clamp_i(x0, 0, dst->w);
    y0 = clamp_i(y0, 0, dst->h);
    x1 = clamp_i(x1, 0, dst->w);
    y1 = clamp_i(y1, 0, dst->h);
    for (int y = y0; y < y1; y++) {
        uint32_t *row = (uint32_t *)((uint8_t *)dst->pixels + y * dst->pitch);
        for (int x = x0; x < x1; x++) row[x] = color;
    }
    return 0;
}

/* SDL_LoadBMP — stub, icon loading not needed */
SDL_Surface *SDL_LoadBMP_RW(SDL_RWops *src, int freesrc) {
    (void)src; (void)freesrc;
    return NULL;
}

/* ================================================================
   PIXEL FORMAT
   ================================================================ */

SDL_PixelFormat *SDL_AllocFormat(Uint32 format) {
    (void)format;
    return &s_argb8888_format;
}

void SDL_FreeFormat(SDL_PixelFormat *format) {
    (void)format; /* static, don't free */
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b) {
    (void)format;
    return 0xFF000000 | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
}

Uint32 SDL_MapRGBA(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b,
                    Uint8 a) {
    (void)format;
    return ((Uint32)a << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
}

void SDL_GetRGBA(Uint32 pixel, const SDL_PixelFormat *format,
                 Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a) {
    (void)format;
    if (a) *a = (pixel >> 24) & 0xFF;
    if (r) *r = (pixel >> 16) & 0xFF;
    if (g) *g = (pixel >> 8) & 0xFF;
    if (b) *b = pixel & 0xFF;
}

/* ================================================================
   TIMING
   ================================================================ */

Uint32 SDL_GetTicks(void) {
    if (g_picos_api && g_picos_api->sys)
        return (Uint32)g_picos_api->sys->getTimeMs();
    return 0;
}

Uint64 SDL_GetPerformanceCounter(void) {
    return (Uint64)SDL_GetTicks() * 1000;
}

Uint64 SDL_GetPerformanceFrequency(void) {
    return 1000000;
}

void SDL_Delay(Uint32 ms) {
    if (!g_picos_api || !g_picos_api->sys) return;
    uint32_t start = g_picos_api->sys->getTimeMs();
    while (g_picos_api->sys->getTimeMs() - start < ms) {
        /* busy wait — no sleep on bare metal */
    }
}

/* ================================================================
   EVENTS / INPUT
   ================================================================ */

static void picos_push_event(const SDL_Event *ev) {
    int next = (s_event_head + 1) % PICOS_EVENT_QUEUE_SIZE;
    if (next == s_event_tail) return; /* queue full, drop */
    s_event_queue[s_event_head] = *ev;
    s_event_head = next;
}

/* Map PicOS key codes to SDL scancodes */
static SDL_Scancode picos_key_to_scancode(int key) {
    if (key >= 'a' && key <= 'z') return (SDL_Scancode)(SDL_SCANCODE_A + (key - 'a'));
    if (key >= 'A' && key <= 'Z') return (SDL_Scancode)(SDL_SCANCODE_A + (key - 'A'));
    if (key >= '1' && key <= '9') return (SDL_Scancode)(SDL_SCANCODE_1 + (key - '1'));
    if (key == '0') return SDL_SCANCODE_0;
    switch (key) {
        case '\r': case '\n': return SDL_SCANCODE_RETURN;
        case 27:   return SDL_SCANCODE_ESCAPE;
        case '\b': return SDL_SCANCODE_BACKSPACE;
        case '\t': return SDL_SCANCODE_TAB;
        case ' ':  return SDL_SCANCODE_SPACE;
        case '-':  return SDL_SCANCODE_MINUS;
        case '=':  return SDL_SCANCODE_EQUALS;
        case ',':  return SDL_SCANCODE_COMMA;
        case '.':  return SDL_SCANCODE_PERIOD;
        case '/':  return SDL_SCANCODE_SLASH;
        default:   return SDL_SCANCODE_UNKNOWN;
    }
}

static SDL_Keycode scancode_to_keycode(SDL_Scancode sc) {
    /* Simple mapping for common keys */
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        return 'a' + (sc - SDL_SCANCODE_A);
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        return '1' + (sc - SDL_SCANCODE_1);
    if (sc == SDL_SCANCODE_0) return '0';
    switch (sc) {
        case SDL_SCANCODE_RETURN: return SDLK_RETURN;
        case SDL_SCANCODE_ESCAPE: return SDLK_ESCAPE;
        case SDL_SCANCODE_BACKSPACE: return SDLK_BACKSPACE;
        case SDL_SCANCODE_TAB: return SDLK_TAB;
        case SDL_SCANCODE_SPACE: return SDLK_SPACE;
        case SDL_SCANCODE_UP: return SDLK_UP;
        case SDL_SCANCODE_DOWN: return SDLK_DOWN;
        case SDL_SCANCODE_LEFT: return SDLK_LEFT;
        case SDL_SCANCODE_RIGHT: return SDLK_RIGHT;
        case SDL_SCANCODE_F1: return SDLK_F1;
        case SDL_SCANCODE_F2: return SDLK_F2;
        case SDL_SCANCODE_F3: return SDLK_F3;
        case SDL_SCANCODE_F4: return SDLK_F4;
        case SDL_SCANCODE_F5: return SDLK_F5;
        default: return SDLK_UNKNOWN;
    }
}

void SDL_PumpEvents(void) {
    if (!g_picos_api || !g_picos_api->input) return;

    /* Poll PicOS button states and generate key events */
    /* Check directional buttons */
    static uint32_t prev_buttons = 0;
    uint32_t buttons = g_picos_api->input->getButtons();

    /* Button-to-scancode mapping */
    struct { uint32_t btn; SDL_Scancode sc; } btn_map[] = {
        { BTN_UP,    SDL_SCANCODE_UP },
        { BTN_DOWN,  SDL_SCANCODE_DOWN },
        { BTN_LEFT,  SDL_SCANCODE_LEFT },
        { BTN_RIGHT, SDL_SCANCODE_RIGHT },
        { BTN_ENTER, SDL_SCANCODE_RETURN },
        { BTN_ESC,   SDL_SCANCODE_ESCAPE },
        { BTN_F1,    SDL_SCANCODE_F1 },
        { BTN_F2,    SDL_SCANCODE_F2 },
        { BTN_F3,    SDL_SCANCODE_F3 },
        { BTN_F4,    SDL_SCANCODE_F4 },
        { BTN_F5,    SDL_SCANCODE_F5 },
    };
    int nmap = sizeof(btn_map) / sizeof(btn_map[0]);

    for (int i = 0; i < nmap; i++) {
        bool was = (prev_buttons & btn_map[i].btn) != 0;
        bool now = (buttons & btn_map[i].btn) != 0;
        if (now && !was) {
            /* Key pressed */
            SDL_Event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = SDL_KEYDOWN;
            ev.key.state = SDL_PRESSED;
            ev.key.keysym.scancode = btn_map[i].sc;
            ev.key.keysym.sym = scancode_to_keycode(btn_map[i].sc);
            s_key_state[btn_map[i].sc] = 1;
            picos_push_event(&ev);
        } else if (was && !now) {
            /* Key released */
            SDL_Event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = SDL_KEYUP;
            ev.key.state = SDL_RELEASED;
            ev.key.keysym.scancode = btn_map[i].sc;
            ev.key.keysym.sym = scancode_to_keycode(btn_map[i].sc);
            s_key_state[btn_map[i].sc] = 0;
            picos_push_event(&ev);
        }
    }

    /* Also check character input for letter keys */
    char ch;
    while ((ch = g_picos_api->input->getChar()) != 0) {
        SDL_Scancode sc = picos_key_to_scancode(ch);
        if (sc != SDL_SCANCODE_UNKNOWN && sc != SDL_SCANCODE_UP &&
            sc != SDL_SCANCODE_DOWN && sc != SDL_SCANCODE_LEFT &&
            sc != SDL_SCANCODE_RIGHT && sc != SDL_SCANCODE_RETURN &&
            sc != SDL_SCANCODE_ESCAPE) {
            /* Key down */
            SDL_Event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = SDL_KEYDOWN;
            ev.key.state = SDL_PRESSED;
            ev.key.keysym.scancode = sc;
            ev.key.keysym.sym = scancode_to_keycode(sc);
            s_key_state[sc] = 1;
            picos_push_event(&ev);
            /* Immediate key up */
            ev.type = SDL_KEYUP;
            ev.key.state = SDL_RELEASED;
            s_key_state[sc] = 0;
            picos_push_event(&ev);
        }
    }

    prev_buttons = buttons;
}

int SDL_PollEvent(SDL_Event *event) {
    SDL_PumpEvents();
    if (s_event_head == s_event_tail) return 0;
    if (event) *event = s_event_queue[s_event_tail];
    s_event_tail = (s_event_tail + 1) % PICOS_EVENT_QUEUE_SIZE;
    return 1;
}

const Uint8 *SDL_GetKeyboardState(int *numkeys) {
    if (numkeys) *numkeys = SDL_NUM_SCANCODES;
    return s_key_state;
}

Uint32 SDL_GetMouseState(int *x, int *y) {
    if (x) *x = 0;
    if (y) *y = 0;
    return 0;
}

/* ================================================================
   RWOPS — File I/O through PicOS filesystem
   ================================================================ */

typedef struct {
    SDL_RWops ops;
    void *fd;   /* pcfile_t */
    int size;
    int pos;
} PicosRWops;

static Sint64 picos_rw_size(SDL_RWops *ctx) {
    PicosRWops *rw = (PicosRWops *)ctx;
    return rw->size;
}

static Sint64 picos_rw_seek(SDL_RWops *ctx, Sint64 offset, int whence) {
    PicosRWops *rw = (PicosRWops *)ctx;
    int newpos;
    switch (whence) {
        case RW_SEEK_SET: newpos = (int)offset; break;
        case RW_SEEK_CUR: newpos = rw->pos + (int)offset; break;
        case RW_SEEK_END: newpos = rw->size + (int)offset; break;
        default: return -1;
    }
    if (newpos < 0) newpos = 0;
    rw->pos = newpos;
    /* Seek in PicOS FS */
    if (g_picos_api && g_picos_api->fs)
        g_picos_api->fs->seek(rw->fd, newpos);
    return newpos;
}

static size_t picos_rw_read(SDL_RWops *ctx, void *ptr, size_t size, size_t maxnum) {
    PicosRWops *rw = (PicosRWops *)ctx;
    if (!g_picos_api || !g_picos_api->fs) return 0;
    size_t total = size * maxnum;
    int avail = rw->size - rw->pos;
    if ((int)total > avail) total = avail > 0 ? avail : 0;
    if (total == 0) return 0;
    int got = g_picos_api->fs->read(rw->fd, ptr, total);
    if (got <= 0) return 0;
    rw->pos += got;
    return got / size;
}

static size_t picos_rw_write(SDL_RWops *ctx, const void *ptr, size_t size,
                              size_t num) {
    PicosRWops *rw = (PicosRWops *)ctx;
    if (!g_picos_api || !g_picos_api->fs) return 0;
    size_t total = size * num;
    int written = g_picos_api->fs->write(rw->fd, ptr, total);
    if (written <= 0) return 0;
    rw->pos += written;
    return written / size;
}

static int picos_rw_close(SDL_RWops *ctx) {
    PicosRWops *rw = (PicosRWops *)ctx;
    if (g_picos_api && g_picos_api->fs && rw->fd >= 0)
        g_picos_api->fs->close(rw->fd);
    free(rw);
    return 0;
}

SDL_RWops *SDL_RWFromFile(const char *file, const char *mode) {
    if (!g_picos_api || !g_picos_api->fs || !file) return NULL;

    void *fd = g_picos_api->fs->open(file, mode);
    if (!fd) return NULL;

    PicosRWops *rw = calloc(1, sizeof(PicosRWops));
    if (!rw) { g_picos_api->fs->close(fd); return NULL; }

    rw->fd = fd;
    rw->pos = 0;
    rw->size = g_picos_api->fs->fsize(fd);
    rw->ops.size = picos_rw_size;
    rw->ops.seek = picos_rw_seek;
    rw->ops.read = picos_rw_read;
    rw->ops.write = picos_rw_write;
    rw->ops.close = picos_rw_close;

    return &rw->ops;
}

SDL_RWops *SDL_RWFromMem(void *mem, int size) {
    (void)mem; (void)size;
    return NULL; /* not needed yet */
}

int SDL_RWclose(SDL_RWops *ctx) {
    if (ctx && ctx->close) return ctx->close(ctx);
    return -1;
}

size_t SDL_RWread(SDL_RWops *ctx, void *ptr, size_t size, size_t maxnum) {
    if (ctx && ctx->read) return ctx->read(ctx, ptr, size, maxnum);
    return 0;
}

size_t SDL_RWwrite(SDL_RWops *ctx, const void *ptr, size_t size, size_t num) {
    if (ctx && ctx->write) return ctx->write(ctx, ptr, size, num);
    return 0;
}

Sint64 SDL_RWseek(SDL_RWops *ctx, Sint64 offset, int whence) {
    if (ctx && ctx->seek) return ctx->seek(ctx, offset, whence);
    return -1;
}

Sint64 SDL_RWtell(SDL_RWops *ctx) {
    if (ctx && ctx->seek) return ctx->seek(ctx, 0, RW_SEEK_CUR);
    return -1;
}

Sint64 SDL_RWsize(SDL_RWops *ctx) {
    if (ctx && ctx->size) return ctx->size(ctx);
    return -1;
}

/* ================================================================
   EVENT STATE / MISC
   ================================================================ */

Uint8 SDL_EventState(Uint32 type, int state) {
    (void)type; (void)state;
    return SDL_ENABLE;
}

