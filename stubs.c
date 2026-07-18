/*
    C-Dogs SDL PicOS Port — Newlib stubs
    Based on apps/doom/stubs.c
*/
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include <malloc.h>
#include "os.h"
#include "dirent.h"
#include "picos_heap.h"

extern const PicoCalcAPI *g_picos_api;
extern char g_app_dir[128];

/* Defined in cdogs_picos.c — longjmp target so _exit() returns to picos_main() */
extern jmp_buf g_exit_jmp;

/* --- Heap for malloc/sbrk ---
 * C-Dogs needs ~5MB for sprites, maps, AI data at 320x240.
 * This lives in BSS and inflates the ELF's p_memsz — keep it as small
 * as practical so the OS ELF loader's PSRAM allocation succeeds. */
#define HEAP_SIZE (5 * 1024 * 1024)
static uint8_t g_heap[HEAP_SIZE] __attribute__((aligned(8)));
static uint8_t *g_heap_ptr = g_heap;

/* Remaining never-allocated heap (sbrk watermark; freed blocks recycled by
 * newlib malloc are not visible here, so this is a conservative floor).
 * Semantics are load-bearing for the LoadImgToSurface reserve guard —
 * see picos_heap.h before changing. */
size_t picos_heap_free(void) {
    return (size_t)((g_heap + HEAP_SIZE) - g_heap_ptr);
}

size_t picos_heap_free_true(void) {
    struct mallinfo mi = mallinfo();
    return (size_t)((g_heap + HEAP_SIZE) - g_heap_ptr) + (size_t)mi.fordblks;
}

/* High-water mark of the sbrk arena (g_heap_ptr - g_heap) — NOT the
 * high-water mark of bytes ever *in use*. peak >= arena >= used always
 * holds; peak only diverges from the current arena size after newlib
 * trims the heap (a negative-incr _sbrk() call). Sampled on every
 * successful _sbrk() growth so it captures peaks between report ticks,
 * not just whatever happens to be current at report time. See
 * heap_peak_sample() below. */
static size_t s_heap_used_peak = 0;

/* Cheap sample-and-update of the peak tracker. Deliberately does NOT call
 * mallinfo() — this runs on the _sbrk() hot path (every malloc that grows
 * the heap), so it must stay O(1). */
static void heap_peak_sample(void) {
    const size_t used = (size_t)(g_heap_ptr - g_heap);
    if (used > s_heap_used_peak) s_heap_used_peak = used;
}

void picos_heap_report(const char *tag) {
    struct mallinfo mi = mallinfo();
    const size_t watermark = (size_t)((g_heap + HEAP_SIZE) - g_heap_ptr);
    /* Belt-and-braces only: g_heap_ptr only ever moves inside _sbrk(),
     * which already samples on every growth, so this call cannot change
     * s_heap_used_peak here. Kept so the peak doesn't silently depend on
     * _sbrk() being the sole caller if that ever changes. */
    heap_peak_sample();
    fprintf(stderr, "HEAPSTAT %s watermark=%u true=%u arena=%u used=%u peak=%u\n",
            tag,
            (unsigned)watermark,
            (unsigned)picos_heap_free_true(),
            (unsigned)mi.arena,
            (unsigned)mi.uordblks,
            (unsigned)s_heap_used_peak);
}

/* Resident-graphics accounting.  Defined here rather than in pic.c so it
 * lives alongside the rest of the PICOS-only heap/graphics instrumentation
 * in one file (stubs.c is itself PICOS-only; pic.c is shared with the
 * non-PICOS desktop build and only touches these symbols inside
 * #ifdef PICOS blocks). */
size_t g_picos_pic_data_bytes = 0;
size_t g_picos_pic_tex_bytes  = 0;
size_t g_picos_pic_bytes_peak = 0;
int    g_picos_pic_count      = 0;
int    g_picos_img_skip_count = 0;

void picos_gfx_report(const char *tag) {
    const size_t total = g_picos_pic_data_bytes + g_picos_pic_tex_bytes;
    fprintf(stderr, "GFXSTAT %s pics=%d data=%u tex=%u total=%u peak=%u skipped=%d\n",
            tag,
            g_picos_pic_count,
            (unsigned)g_picos_pic_data_bytes,
            (unsigned)g_picos_pic_tex_bytes,
            (unsigned)total,
            (unsigned)g_picos_pic_bytes_peak,
            g_picos_img_skip_count);
}

/* OS tick during bulk asset loading: feeds the hardware watchdog and keeps
 * the dev console responsive while no frames are being rendered.  Rate-
 * limited because poll() includes an I2C keyboard read — ticking on every
 * file operation would visibly slow loading.  Called from the file-op stubs
 * below so ANY long file-heavy stretch (campaign loads, asset scans) feeds
 * the watchdog without needing per-loop instrumentation in game code. */
void picos_asset_load_tick(void) {
    static uint32_t s_last_tick_ms = 0;
    static uint32_t s_last_report_ms = 0;
    if (!g_picos_api || !g_picos_api->sys)
        return;
    uint32_t now = g_picos_api->sys->getTimeMs();
    if (now - s_last_tick_ms < 500)
        return;
    s_last_tick_ms = now;
    /* Report on a slower cadence than the watchdog tick: mallinfo() walks
     * the free list, so it is O(free blocks) and not worth doing at 2Hz.
     * 1000ms (not lower) balances catching short bursts of allocation
     * against that cost — see heap_peak_sample() for how the peak is
     * still captured between ticks regardless of this cadence. */
    if (now - s_last_report_ms >= 1000) {
        s_last_report_ms = now;
        picos_heap_report("load");
        picos_gfx_report("load");
    }
    g_picos_api->sys->poll();
}

void * _sbrk(ptrdiff_t incr) {
    uint8_t *prev_ptr = g_heap_ptr;
    if (g_heap_ptr + incr < g_heap) { errno = ENOMEM; return (void *)-1; }
    if (g_heap_ptr + incr > g_heap + HEAP_SIZE) {
        errno = ENOMEM;
        fprintf(stderr, "HEAP EXHAUSTED: need %d, used %d/%d\n",
                (int)incr, (int)(g_heap_ptr - g_heap), HEAP_SIZE);
        return (void *)-1;
    }
    g_heap_ptr += incr;
    heap_peak_sample();
    return prev_ptr;
}

/* --- Serial output buffering --- */
static char s_log_buf[256];
static int  s_log_pos = 0;

static void log_flush(void) {
    if (s_log_pos > 0) {
        s_log_buf[s_log_pos] = '\0';
        g_picos_api->sys->log(s_log_buf);
        s_log_pos = 0;
    }
}

/* --- File System Stubs (mapped to PicOS FS API) --- */

static pcfile_t g_fd_table[16] = {0};

void picos_asset_load_tick(void); /* defined below */

int _open(const char *name, int flags, int mode) {
    (void)mode;
    picos_asset_load_tick();
    char full_path[256];
    if (name[0] != '/') {
        snprintf(full_path, sizeof(full_path), "%s/%s", g_app_dir, name);
    } else {
        strncpy(full_path, name, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    }

    const char *picos_mode = "rb";
    if ((flags & 0x3) == 1) picos_mode = "wb";
    else if ((flags & 0x3) == 2) picos_mode = "w+b";

    for (int i = 0; i < 16; i++) {
        if (g_fd_table[i] == NULL) {
            g_fd_table[i] = g_picos_api->fs->open(full_path, picos_mode);
            if (g_fd_table[i]) return i + 3;
            return -1;
        }
    }
    return -1;
}

int _read(int file, char *ptr, int len) {
    if (file < 3) return 0;
    pcfile_t f = g_fd_table[file - 3];
    if (!f) return -1;
    return g_picos_api->fs->read(f, ptr, len);
}

int _write(int file, char *ptr, int len) {
    if (file == 1 || file == 2) {
        for (int i = 0; i < len; i++) {
            if (ptr[i] == '\n' || s_log_pos >= (int)sizeof(s_log_buf) - 1) {
                log_flush();
            } else {
                s_log_buf[s_log_pos++] = ptr[i];
            }
        }
        return len;
    }
    if (file < 3) return -1;
    pcfile_t f = g_fd_table[file - 3];
    if (!f) return -1;
    return g_picos_api->fs->write(f, ptr, len);
}

int _close(int file) {
    if (file < 3) return 0;
    pcfile_t f = g_fd_table[file - 3];
    if (!f) return -1;
    g_picos_api->fs->close(f);
    g_fd_table[file - 3] = NULL;
    return 0;
}

int _lseek(int file, int ptr, int dir) {
    if (file < 3) return 0;
    pcfile_t f = g_fd_table[file - 3];
    if (!f) return -1;
    uint32_t target = ptr;
    if (dir == 1) target = g_picos_api->fs->tell(f) + ptr;
    else if (dir == 2) target = g_picos_api->fs->fsize(f) + ptr;
    g_picos_api->fs->seek(f, target);
    return g_picos_api->fs->tell(f);
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFREG;
    if (file < 3) st->st_mode = S_IFCHR;
    st->st_size = (file >= 3 && g_fd_table[file-3]) ? g_picos_api->fs->fsize(g_fd_table[file-3]) : 0;
    return 0;
}

int _isatty(int file) {
    if (file < 3) return 1;
    return 0;
}

int _unlink(const char *name) { (void)name; return -1; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; return -1; }

void _exit(int status) {
    char buf[64];
    snprintf(buf, sizeof(buf), "CDOGS _exit(%d) called", status);
    log_flush();
    if (g_picos_api) g_picos_api->sys->log(buf);
    longjmp(g_exit_jmp, status ? status : -1);
    __builtin_unreachable();
}

/* getenv stub — provide HOME so C-Dogs can find config paths */
char *getenv(const char *name) {
    if (name && strcmp(name, "HOME") == 0) return g_app_dir;
    if (name && strcmp(name, "CDOGS_CONFIG_DIR") == 0) return g_app_dir;
    return NULL;
}

/* Override __assert_func so CASSERT prints before aborting */
void __assert_func(const char *file, int line, const char *func, const char *expr) {
    char buf[256];
    snprintf(buf, sizeof(buf), "ASSERT FAIL: %s:%d %s: %s", file, line, func ? func : "?", expr);
    if (g_picos_api) g_picos_api->sys->log(buf);
    fprintf(stderr, "%s\n", buf);
    _exit(1);
}

int mkdir(const char *path, mode_t mode) { (void)path; (void)mode; return 0; }
int _link(const char *old, const char *new_) { (void)old; (void)new_; return -1; }

/* --- Additional POSIX stubs needed by C-Dogs / tinydir --- */

int stat(const char *path, struct stat *buf) {
    if (!buf) return -1;
    picos_asset_load_tick();
    memset(buf, 0, sizeof(*buf));
    /* Try to open the file to check existence */
    char full_path[256];
    if (path[0] != '/') {
        snprintf(full_path, sizeof(full_path), "%s/%s", g_app_dir, path);
    } else {
        strncpy(full_path, path, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    }

    /* fs->exists() is true for BOTH files and directories, so distinguish by
     * attempting to open as a file: directories are not openable.  The old
     * logic here reported every existing directory as S_IFREG, which made
     * tinydir's is_dir false for all campaign folders (and reported missing
     * paths as directories). */
    if (g_picos_api->fs->exists(full_path)) {
        pcfile_t f = g_picos_api->fs->open(full_path, "rb");
        if (f) {
            buf->st_mode = S_IFREG | 0644;
            buf->st_size = g_picos_api->fs->fsize(f);
            g_picos_api->fs->close(f);
        } else {
            buf->st_mode = S_IFDIR | 0755;
        }
        return 0;
    }

    errno = ENOENT;
    return -1;
}

int access(const char *path, int mode) {
    (void)mode;
    char full_path[256];
    if (path[0] != '/') {
        snprintf(full_path, sizeof(full_path), "%s/%s", g_app_dir, path);
    } else {
        strncpy(full_path, path, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    }
    return g_picos_api->fs->exists(full_path) ? 0 : -1;
}

char *getcwd(char *buf, size_t size) {
    if (buf && size > 0) {
        strncpy(buf, g_app_dir, size - 1);
        buf[size - 1] = '\0';
    }
    return buf;
}

char *realpath(const char *path, char *resolved_path) {
    if (!resolved_path) return NULL;
    if (path[0] == '/') {
        strncpy(resolved_path, path, 255);
    } else {
        snprintf(resolved_path, 256, "%s/%s", g_app_dir, path);
    }
    return resolved_path;
}

int rename(const char *old, const char *new_) { (void)old; (void)new_; return -1; }
int remove(const char *path) { (void)path; return -1; }
int rmdir(const char *path) { (void)path; return -1; }
int lstat(const char *path, struct stat *buf) { return stat(path, buf); }
int chdir(const char *path) { (void)path; return 0; }

/* --- Directory iteration stubs (for tinydir) --- */

/* Pool of directory states to support nested opendir (tinydir_file_open
 * calls opendir on the parent directory, so we need at least 2 active).
 * MAX_DIR_ENTRIES must cover the largest game dir: data/graphics has 344
 * top-level entries — at the old cap of 128, two thirds of the sprites were
 * silently never loaded. Each slot: 512 × 64B = 32KB, pool of 4 = 128KB BSS. */
#define MAX_DIR_ENTRIES 512
#define MAX_DIR_NAME 64
#define DIR_POOL_SIZE 4
typedef struct {
    char entries[MAX_DIR_ENTRIES][MAX_DIR_NAME];
    int count;
    int pos;
    int in_use;
} picos_dir_t;

static picos_dir_t s_dir_pool[DIR_POOL_SIZE];

static void dir_list_cb(const char *name, bool is_dir, uint32_t size, void *user) {
    picos_dir_t *d = (picos_dir_t *)user;
    (void)is_dir;
    (void)size;
    if (d->count < MAX_DIR_ENTRIES) {
        strncpy(d->entries[d->count], name, MAX_DIR_NAME - 1);
        d->entries[d->count][MAX_DIR_NAME - 1] = '\0';
        d->count++;
    }
}

DIR *opendir(const char *name) {
    picos_asset_load_tick();
    /* Find a free slot in the pool */
    picos_dir_t *d = NULL;
    int slot = -1;
    for (int i = 0; i < DIR_POOL_SIZE; i++) {
        if (!s_dir_pool[i].in_use) {
            d = &s_dir_pool[i];
            slot = i;
            break;
        }
    }
    if (!d) {
        fprintf(stderr, "opendir POOL EXHAUSTED for '%s' (leaked closedir?)\n", name);
        errno = ENOMEM;
        return NULL;
    }

    char full_path[256];
    if (name[0] != '/') {
        snprintf(full_path, sizeof(full_path), "%s/%s", g_app_dir, name);
    } else {
        strncpy(full_path, name, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    }

    d->count = 0;
    d->pos = 0;
    d->in_use = 1;
    g_picos_api->fs->listDir(full_path, dir_list_cb, d);
    /* Only log anomalies — per-dir logging at 115200 baud adds seconds of
     * blocking printf to every asset scan. */
    if (d->count >= MAX_DIR_ENTRIES)
        fprintf(stderr, "opendir '%s' slot=%d count=%d (CAP HIT)\n",
                full_path, slot, d->count);
    (void)slot;

    return (DIR *)d;
}

static struct dirent s_dirent;

struct dirent *readdir(DIR *dirp) {
    picos_dir_t *d = (picos_dir_t *)dirp;
    if (!d || d->pos >= d->count) return NULL;

    memset(&s_dirent, 0, sizeof(s_dirent));
    strncpy(s_dirent.d_name, d->entries[d->pos], sizeof(s_dirent.d_name) - 1);
    d->pos++;
    return &s_dirent;
}

int closedir(DIR *dirp) {
    picos_dir_t *d = (picos_dir_t *)dirp;
    if (d) d->in_use = 0;
    return 0;
}

/* --- Additional POSIX stubs --- */
#include <sys/time.h>
#include <sys/times.h>

char *dirname(char *path) {
    if (!path || !*path) return ".";
    char *last_slash = strrchr(path, '/');
    if (!last_slash) return ".";
    if (last_slash == path) return "/";
    *last_slash = '\0';
    return path;
}

char *basename(char *path) {
    if (!path || !*path) return ".";
    char *last_slash = strrchr(path, '/');
    if (last_slash) return last_slash + 1;
    return path;
}

int _gettimeofday(struct timeval *tv, void *tz) {
    (void)tz;
    if (tv && g_picos_api) {
        uint32_t ms = g_picos_api->sys->getTimeMs();
        tv->tv_sec = ms / 1000;
        tv->tv_usec = (ms % 1000) * 1000;
    }
    return 0;
}

clock_t _times(struct tms *buf) {
    if (buf) memset(buf, 0, sizeof(*buf));
    return 0;
}
