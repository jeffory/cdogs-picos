# PicOS C-Dogs SDL Native App Build

CC      = arm-none-eabi-gcc
CFLAGS  = -mcpu=cortex-m33 -mthumb -std=gnu11 \
          -fpie -fno-plt -ffunction-sections -fdata-sections \
          -Os -g \
          -Wall -Wno-unused-parameter -Wno-unused-function \
          -Wno-sign-compare -Wno-missing-field-initializers \
          -Wno-unused-variable -Wno-unused-but-set-variable \
          -Wno-old-style-declaration -Wno-maybe-uninitialized \
          -Wno-incompatible-pointer-types \
          -I. \
          -Iposix_stub \
          -Isdk/native \
          -Isdl_shim \
          -Ipb_stub \
          -Ienet_stub \
          -Isrc/src \
          -Isrc/src/cdogs \
          -Isrc/src/cdogs/yajl/api \
          -DPICOS
LDFLAGS = -T sdk/native/linker.ld \
          -Wl,--entry=picos_main \
          -Wl,-pie \
          -Wl,--gc-sections \
          -Wl,--no-warn-rwx-segments \
          -nostartfiles -nodefaultlibs -lc -lm -lgcc

# --- Source file discovery ---

# Files to exclude from wildcard-based collection
EXCLUDE_PATS = src/src/cdogs/enet/% \
               src/src/cdogs/cwolfmap/% \
               src/src/cdogs/SDL_JoystickButtonNames/% \
               src/src/cdogs/net_client.c \
               src/src/cdogs/net_server.c \
               src/src/cdogs/net_util.c \
               src/src/cdogs/joystick.c \
               src/src/cdogs/mouse.c \
               src/src/cdogs/map_wolf.c

# Core engine
CDOGS_SRCS     = $(filter-out $(EXCLUDE_PATS), $(wildcard src/src/cdogs/*.c))
DRAW_SRCS      = $(wildcard src/src/cdogs/draw/*.c)
HUD_SRCS       = $(wildcard src/src/cdogs/hud/*.c)
COLLISION_SRCS = $(wildcard src/src/cdogs/collision/*.c)
HASHMAP_SRCS   = $(wildcard src/src/cdogs/c_hashmap/*.c)
YAJL_SRCS      = $(wildcard src/src/cdogs/yajl/*.c)
MATHC_SRCS     = $(wildcard src/src/cdogs/mathc/mathc.c)
EASING_SRCS    = $(wildcard src/src/cdogs/aheasing/*.c)

# Game loop files (exclude cdogs.c — contains main())
GAME_SRCS = $(filter-out src/src/cdogs.c, $(wildcard src/src/*.c))

# Utility libraries
JSON_SRCS  = $(wildcard src/src/json/*.c)
BASE64_SRCS = $(wildcard src/src/base64/*.c)

# Proto (nanopb generated — needed for game event types)
PROTO_SRCS = src/src/proto/msg.pb.c

# PicOS platform
PICOS_SRCS = cdogs_picos.c stubs.c net_stubs.c picos_sdl_impl.c

SRCS = $(PICOS_SRCS) \
       $(CDOGS_SRCS) $(DRAW_SRCS) $(HUD_SRCS) $(COLLISION_SRCS) \
       $(HASHMAP_SRCS) $(YAJL_SRCS) $(MATHC_SRCS) $(EASING_SRCS) \
       $(GAME_SRCS) $(JSON_SRCS) $(BASE64_SRCS) $(PROTO_SRCS)

TARGET = main.elf

# Upstream generates src/src/cdogs/sys_config.h with CMake (and git-ignores it);
# the PicOS variant is the tracked sys_config.h at the repo root, staged here.
GEN_SYS_CONFIG = src/src/cdogs/sys_config.h

.PHONY: all clean syntax-check

all: $(TARGET)

$(GEN_SYS_CONFIG): sys_config.h
	cp $< $@

$(TARGET): $(SRCS) $(GEN_SYS_CONFIG) sdk/native/linker.ld sdk/native/os.h sdk/native/app_abi.h
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@
	cp $@ $@.debug
	arm-none-eabi-strip $@
	arm-none-eabi-size $@

# Phase 0 gate: all files parse without errors
syntax-check: $(GEN_SYS_CONFIG)
	$(CC) $(CFLAGS) -fsyntax-only $(SRCS)
	@echo "=== All files parse OK ==="

clean:
	rm -f $(TARGET) $(TARGET).debug $(GEN_SYS_CONFIG)
