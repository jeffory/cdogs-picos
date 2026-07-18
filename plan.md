# C-Dogs SDL PicOS Port — Implementation Plan

## Context

Porting C-Dogs SDL (overhead run-and-gun shooter) to PicOS. Two deliverables:
1. **C-Dogs app** (`apps/cdogs/`) — native ELF game port
2. **MOD player** — firmware-integrated reusable music player for all apps

Previous feasibility assessment concluded: **feasible, 5-8 weeks total effort**. This plan details the concrete implementation steps starting with Phase 0 scaffolding.

---

## Part 1: MOD Player as Reusable Firmware Component

### Why firmware-integrated (not `system/lib/`)

- `system/lib/` currently holds only **Lua libraries** (e.g., `widgets.lua`)
- The native ELF loader supports **only `R_ARM_RELATIVE` relocations** — no dynamic linking, no symbol resolution (`native_loader.c:528,550`)
- Native apps **cannot** import functions from a `.a` on SD card at runtime
- The natural pattern is **firmware-integrated**, mirroring the MP3 player exactly:
  - `mp3_player.c/.h` → `mod_player.c/.h`
  - `g_api.soundplayer` already exposes MP3/fileplayer/sample → add `g_api.modplayer`
  - Core 1 calls `mod_player_update()` every 5ms alongside `mp3_player_update()`
  - Lua gets `picocalc.modplayer` via `lua_bridge_mod.c`

### MOD decoder choice

Candidates (single-file or small footprint):
- **pocketmod** (~800 lines, public domain, MOD only) — smallest, MOD-only
- **libxm** (~3K lines, WTFPL, XM+MOD) — small, handles XM format too
- **jar_mod** (~1.5K lines, public domain) — single-header MOD player

**Recommendation**: Start with **pocketmod** for minimal flash footprint. Can upgrade to libxm later if XM support is needed.

### Files to create/modify

| File | Action | Description |
|------|--------|-------------|
| `third_party/pocketmod/pocketmod.h` | Create | Single-header MOD decoder |
| `src/drivers/mod_player.h` | Create | API: `mod_player_t`, create/load/play/stop/destroy/update |
| `src/drivers/mod_player.c` | Create | Implementation mirroring `mp3_player.c` pattern |
| `src/os/lua_bridge_mod.c` | Create | Lua bindings: `picocalc.modplayer.{create,load,play,stop,destroy}` |
| `src/os/os.h` | Modify | Add `picocalc_modplayer_t` struct, add to `PicoCalcAPI` |
| `sdk/native/os.h` | Modify | Mirror the `picocalc_modplayer_t` addition |
| `src/os/lua_bridge.c` | Modify | Call `lua_bridge_mod_register()` in `lua_bridge_register()` |
| `src/main.c` | Modify | Wire `g_api.modplayer = &s_modplayer_impl;`, call `mod_player_update()` in Core 1 loop |
| `CMakeLists.txt` | Modify | Add `mod_player.c`, `lua_bridge_mod.c`, pocketmod include path |

### API design (`picocalc_modplayer_t`)

```c
typedef struct mod_player mod_player_t;

typedef struct {
    mod_player_t* (*create)(void);
    void (*destroy)(mod_player_t *p);
    int  (*load)(mod_player_t *p, const char *path);  // returns 0 on success
    void (*play)(mod_player_t *p, bool loop);
    void (*stop)(mod_player_t *p);
    void (*pause)(mod_player_t *p);
    void (*resume)(mod_player_t *p);
    bool (*isPlaying)(mod_player_t *p);
    void (*setVolume)(mod_player_t *p, int vol);       // 0-100
} picocalc_modplayer_t;
```

### Core 1 integration

In `src/main.c` `core1_entry()` loop (runs every 5ms):
```c
mod_player_update();   // alongside mp3_player_update(), fileplayer_update()
```

`mod_player_update()` renders PCM samples into a ring buffer. The audio DMA ISR reads from a small SRAM staging buffer, same pattern as MP3.

### Memory budget for MOD player

| Item | Size | Location |
|------|------|----------|
| pocketmod code | ~4KB | Flash (.text) |
| `mod_player.c` | ~3KB | Flash (.text) |
| MOD file buffer | ~100-500KB | PSRAM (umm_malloc) |
| PCM ring buffer | 8KB | PIO PSRAM or PSRAM |
| Staging buffer | 1KB | SRAM |

### DOOM integration

DOOM currently uses OPL synthesis for music (`opl.c`, `mus_player.c`). The firmware MOD player opens the door for DOOM to optionally play MOD-format music packs via `g_api.modplayer`, similar to community music replacements. This would require minimal changes to `i_picos_sound.c` — call `g_api.modplayer->load()/play()` instead of OPL when a `.mod` file is present for a given music lump.

---

## Part 2: C-Dogs Port Scaffolding (Phase 0)

### Step 0.1: Clone and strip cdogs-sdl

```bash
git clone https://github.com/cxong/cdogs-sdl.git apps/cdogs/src
cd apps/cdogs/src
```

**Delete these directories entirely:**
- `src/cdogsed/` — campaign editor (OpenGL)
- `src/tests/` — unit tests
- `build/` — build artifacts
- `.github/` — CI config

**Delete these source files:**
- `src/cdogs/net_client.*`, `src/cdogs/net_server.*`, `src/cdogs/net_util.*` — networking
- `src/cdogs/proto/` — protobuf
- All joystick/mouse-specific code (keep keyboard)

**Keep `src/cdogs/` core engine** — this is the game logic, rendering, AI, maps, weapons.

### Step 0.2: Create app directory structure

```
apps/cdogs/
├── Makefile          # cross-compile (copy from apps/doom/Makefile)
├── linker.ld         # PIE linker script (copy from apps/doom/linker.ld)
├── cdogs_picos.c     # entry point (picos_main)
├── stubs.c           # newlib stubs (_sbrk, _open, etc.)
├── picos_sdl.h       # minimal SDL type shims
├── picos_grafx.c     # display HAL (ARGB8888 → RGB565 framebuffer)
├── picos_input.c     # input HAL (BTN_* → CMD_*)
├── picos_sound.c     # audio HAL (stub initially, wire up in Phase 3)
├── app.json          # {"id":"com.picos.cdogs", "name":"C-Dogs", ...}
├── src/              # stripped cdogs-sdl source tree
└── data/             # game assets (sprites, maps, sounds) — on SD card
```

### Step 0.3: Create `picos_sdl.h` (SDL type shim)

Minimal header providing SDL type definitions so game logic compiles without SDL:

```c
// picos_sdl.h — minimal SDL type shim for PicOS
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t Uint32;
typedef uint16_t Uint16;
typedef uint8_t  Uint8;
typedef int32_t  Sint32;
typedef int16_t  Sint16;

typedef struct { int x, y, w, h; } SDL_Rect;
typedef struct { uint8_t r, g, b, a; } SDL_Color;
typedef int SDL_BlendMode;
typedef void SDL_Texture;
typedef void SDL_Renderer;
typedef void SDL_Window;
typedef void SDL_Surface;

// Pixel format constants (values don't matter, just need to compile)
#define SDL_PIXELFORMAT_ARGB8888 0
#define SDL_BLENDMODE_BLEND 1
#define SDL_BLENDMODE_NONE 0

// Stub macros for SDL functions that become no-ops
#define SDL_GetTicks() picos_get_ticks()
#define SDL_Delay(ms) picos_delay(ms)

uint32_t picos_get_ticks(void);
void picos_delay(uint32_t ms);
```

### Step 0.4: Create `stubs.c`

Copy `apps/doom/stubs.c` verbatim, adjust heap size:

```c
#define HEAP_SIZE (3072 * 1024)  // 3MB (up from DOOM's 2.5MB)
```

### Step 0.5: Create `cdogs_picos.c` (entry point skeleton)

```c
#include "app_abi.h"
#include "os.h"
#include <setjmp.h>

const PicoCalcAPI *g_picos_api;
char g_app_dir[128];
jmp_buf g_exit_jmp;

void picos_main(const PicoCalcAPI *api, const char *app_dir,
                const char *app_id, const char *app_name)
{
    g_picos_api = api;
    strncpy(g_app_dir, app_dir, sizeof(g_app_dir) - 1);

    int exit_code = setjmp(g_exit_jmp);
    if (exit_code != 0) {
        api->sys->log("CDOGS: exit() called, returning to launcher");
        return;
    }

    api->display->clear(0x0000);
    api->display->flush();

    // TODO Phase 1: cdogs_init(), main loop
    api->sys->log("CDOGS: Scaffold loaded successfully");

    while (!api->sys->shouldExit()) {
        api->sys->poll();
        // TODO: cdogs_tick(); cdogs_draw();
    }
}
```

### Step 0.6: Create `Makefile`

Based on `apps/doom/Makefile`, targeting the stripped cdogs source tree. Key differences from DOOM:
- Source files: `cdogs_picos.c stubs.c picos_grafx.c picos_input.c picos_sound.c` + `src/cdogs/*.c`
- Include paths: `-Isrc/cdogs -Isrc/cdogs/include` (wherever cdogs headers live)
- Defines: `-DPICOS` (platform guard)
- Exclude patterns: SDL platform files, editor, networking

### Step 0.7: Gate

`arm-none-eabi-gcc` with `-fsyntax-only` parses all files without errors. Link errors are expected and acceptable at this stage — the goal is that all headers resolve and all types are defined.

---

## Part 3: Remaining Phases (summary, unchanged from feasibility)

### Phase 1: Boot to main menu (1-2 weeks)
- Stub all SDL rendering → get game logic compiling + linking
- Implement `picos_grafx.c` (ARGB8888 buffer → RGB565 → `flushRegion`)
- Implement `picos_input.c` (button polling)
- Wire file I/O stubs
- **Gate**: Main menu visible on screen, navigable

### Phase 2: Gameplay (2-3 weeks)
- Tile/sprite rendering, camera, map loading, AI, collision, HUD
- **Gate**: Play a mission end-to-end

### Phase 3: Audio (1 week)
- SFX: Software PCM mixer on Core 1 (DOOM pattern)
- Music: Wire up firmware MOD player via `g_api.modplayer`
- **Gate**: Gunshots + background music audible

### Phase 4: Polish (1-2 weeks)
- Menus, save/load, performance tuning, loading screen
- **Gate**: Full single-player campaign playable

---

## Key Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| SDL deeply coupled — shim approach breaks | Medium | High | Rewrite the ~15 core rendering files fully |
| Rendering too slow (<10fps) | Medium | High | Frame skip, reduce sprite count, native RGB565 |
| PSRAM OOM | Low-Medium | High | Pre-convert to RGB565, lower audio quality |
| Code size > 5MB ELF | Low | High | `-Os`, `--gc-sections`, strip features |
| Stack overflow (8KB PSP) | Medium | Medium | Profile call depth, heap-allocate large locals |
| pocketmod doesn't handle all MOD files | Low | Low | Upgrade to libxm if needed |

## Key Reference Files

- `apps/doom/dg_picos.c` — entry point, framebuffer pipeline, input mapping
- `apps/doom/stubs.c` — newlib stubs (_sbrk, FS, _exit longjmp)
- `apps/doom/i_picos_sound.c` — Core 1 audio mixer
- `apps/doom/Makefile` — cross-compile config
- `apps/doom/linker.ld` — PIE linker script
- `sdk/native/os.h` — PicoCalcAPI surface
- `src/drivers/mp3_player.c` — reference architecture for MOD player
- `src/os/lua_bridge_sound.c` — Lua binding pattern for audio
- `src/main.c:1257-1273` — where `g_api` sub-tables are wired
- `src/main.c` Core 1 entry — where `mod_player_update()` gets called

## Verification

### MOD player (firmware)
1. `cd build && make -j4` — firmware builds with MOD player included
2. Write a test Lua app: `local p = picocalc.modplayer.create(); p:load("test.mod"); p:play(true)`
3. Or test via native app using `g_api.modplayer->create()` / `load()` / `play()`
4. Verify audio output, no crashes, clean stop/destroy

### C-Dogs scaffold (Phase 0)
1. `cd apps/cdogs && make` — produces `main.elf`
2. `arm-none-eabi-size main.elf` — verify text+data+bss under 5MB
3. Copy to SD, boot PicOS → launcher shows "C-Dogs" → launch → "Scaffold loaded" in log
4. System menu exit → clean return to launcher
