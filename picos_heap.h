/*
    C-Dogs SDL PicOS Port — heap and graphics instrumentation

    Mostly reporting, with one exception: picos_heap_free_true() feeds
    the LoadImgToSurface reserve guard (utils.c) — see the notes on the
    two free-space functions before changing either one's semantics.
*/
#ifndef PICOS_HEAP_H
#define PICOS_HEAP_H

#include <stddef.h>

/* Never-allocated sbrk space only.  Blocks freed and recycled by newlib
   malloc are NOT counted, so this is a conservative floor.  Reporting
   only (the HEAPSTAT watermark field) since sub-project 2B moved the
   reserve guard to picos_heap_free_true(). */
size_t picos_heap_free(void);

/* Never-allocated sbrk space plus newlib's free list: the real number.
   This is what the LoadImgToSurface reserve guard (utils.c) compares
   against IMG_LOAD_HEAP_RESERVE — changing its semantics means
   re-tuning that reserve against a measured mission start on
   hardware. */
size_t picos_heap_free_true(void);

/* Emit one HEAPSTAT line to stderr:
     HEAPSTAT <tag> watermark=<u> true=<u> arena=<u> used=<u> peak=<u>
   peak is the high-water mark of the sbrk arena (g_heap_ptr - g_heap),
   tracked internally in stubs.c and sampled on every _sbrk() growth — not
   just at report time — so it survives between report ticks regardless of
   cadence. It is NOT the high-water mark of bytes-in-use (that's `used`,
   above): peak >= arena >= used always holds, and peak only diverges from
   the current arena size after newlib trims the heap. */
void picos_heap_report(const char *tag);

/* Live resident-graphics accounting. data_bytes and pic_count are
   maintained by pic.c; tex_bytes is maintained by the SDL texture shim
   (picos_sdl_impl.c's SDL_CreateTexture/SDL_DestroyTexture), which is the
   only code that knows whether a given texture actually owns a pixel
   buffer rather than borrowing one. */
extern size_t g_picos_pic_data_bytes;
extern size_t g_picos_pic_tex_bytes;
extern int    g_picos_pic_count;
/* High-water mark of (data+tex), each sample computed from the CURRENT
   values of both counters. The sampler, picos_gfx_bytes_peak_sample()
   below, is only called from pic.c's data-changing sites (PicLoad/
   PicCopy/PicShrink) — a tex_bytes update in the texture shim
   (picos_sdl_impl.c) does not by itself trigger a sample. In practice this
   still captures the combined total: GraphicsInitialize creates C-Dogs'
   window-sized owning textures (bumping tex_bytes) before PicManagerLoad
   loads a single sprite, and peak only ever increases, so the first
   data-changing sample after that point already sees tex_bytes at its
   settled value. Unlike an instantaneous sample at report time, this
   cannot land between two report ticks and miss a load that both starts
   and finishes inside the gap (empirically, C-Dogs' boot-time asset scan
   at the idle main menu does exactly that against
   picos_asset_load_tick's report cadence). */
extern size_t g_picos_pic_bytes_peak;
/* Images refused by the LoadImgToSurface reserve guard. */
extern int    g_picos_img_skip_count;

/* Per-PicLoad-call tally of the chars/ format pre-pass's decision (Stage 2C
 * Amendment B, pic.c's PicLoadClassifyCharsFormat) -- one increment per
 * frame, so a spritesheet contributes once per frame, not once per file.
 * Reported once at the end of asset load (picos_charsfmt_report) rather than
 * per-item, same rationale as GFXSTAT/HEAPSTAT above. */
extern int    g_picos_chars_fmt_la8;
extern int    g_picos_chars_fmt_rgb565;
extern int    g_picos_chars_fmt_argb8888;

/* Emit one CHARSFMT line to stderr:
     CHARSFMT <tag> la8=<d> rgb565=<d> argb8888=<d>
   Lets the per-pic tri-state split (Amendment B) be observed from a normal
   load without per-item tracing. */
void picos_charsfmt_report(const char *tag);

static inline void picos_gfx_bytes_peak_sample(void) {
    const size_t total = g_picos_pic_data_bytes + g_picos_pic_tex_bytes;
    if (total > g_picos_pic_bytes_peak)
    {
        g_picos_pic_bytes_peak = total;
    }
}

/* Emit one GFXSTAT line to stderr:
     GFXSTAT <tag> pics=<d> data=<u> tex=<u> total=<u> peak=<u> skipped=<d> */
void picos_gfx_report(const char *tag);

/* Per-item load tracing (every pic added, every JSON file read).
 *
 * Off by default because stderr is USB CDC at 115200 baud, where a ~20-byte
 * line costs roughly 1.7ms of BLOCKING serial write. Asset loading adds pics
 * in the low thousands, so tracing per add spent multiple seconds of the
 * startup budget doing nothing but talking to the host — and that budget is
 * also what the OS watchdog is measuring.
 *
 * Build with -DPICOS_VERBOSE_LOAD=1 to get it back. Deliberately does NOT
 * cover HEAPSTAT/GFXSTAT, the RenderPresent census or the main-menu marker:
 * the e2e suite parses those. */
#ifndef PICOS_VERBOSE_LOAD
#define PICOS_VERBOSE_LOAD 0
#endif
#if PICOS_VERBOSE_LOAD
#define PICOS_LOADLOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define PICOS_LOADLOG(...) ((void)0)
#endif

#endif /* PICOS_HEAP_H */
