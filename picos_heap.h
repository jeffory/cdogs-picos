/*
    C-Dogs SDL PicOS Port — heap and graphics instrumentation

    Reporting only.  None of this is wired into allocation decisions;
    see the note on picos_heap_free_true() before changing that.
*/
#ifndef PICOS_HEAP_H
#define PICOS_HEAP_H

#include <stddef.h>

/* Never-allocated sbrk space only.  Blocks freed and recycled by newlib
   malloc are NOT counted, so this is a conservative floor.  The
   LoadImgToSurface reserve guard is tuned against exactly these
   semantics — do not "fix" it to include the free list without
   re-tuning IMG_LOAD_HEAP_RESERVE, or more images will load and the
   heap will be exhausted later and less predictably. */
size_t picos_heap_free(void);

/* Never-allocated sbrk space plus newlib's free list: the real number.
   Reporting only, deliberately not used by the reserve guard. */
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

/* Live resident-graphics accounting, maintained by pic.c. */
extern size_t g_picos_pic_data_bytes;
extern size_t g_picos_pic_tex_bytes;
extern int    g_picos_pic_count;
/* High-water mark of (data+tex). Updated via picos_gfx_bytes_peak_sample()
   below wherever either byte counter increases — a cheap comparison on
   pic.c's allocation hot path, no allocation of its own. Unlike an
   instantaneous sample at report time, this cannot land between two
   report ticks and miss a load that both starts and finishes inside the
   gap (empirically, C-Dogs' boot-time asset scan at the idle main menu
   does exactly that against picos_asset_load_tick's report cadence). */
extern size_t g_picos_pic_bytes_peak;
/* Images refused by the LoadImgToSurface reserve guard. */
extern int    g_picos_img_skip_count;

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

#endif /* PICOS_HEAP_H */
