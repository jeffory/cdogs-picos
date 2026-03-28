/*
    PicOS SDL_mixer Type Shim for C-Dogs SDL
*/
#pragma once

#include "picos_sdl.h"

#define SDL_MIXER_MAJOR_VERSION 2
#define SDL_MIXER_MINOR_VERSION 0
#define SDL_MIXER_PATCHLEVEL    4

#define MIX_MAX_VOLUME 128
#define MIX_DEFAULT_FREQUENCY 22050
#define MIX_DEFAULT_CHANNELS 2
#define MIX_CHANNELS 8

#define AUDIO_U8     0x0008
#define AUDIO_S8     0x8008
#define AUDIO_U16LSB 0x0010
#define AUDIO_S16LSB 0x8010
#define AUDIO_U16MSB 0x1010
#define AUDIO_S16MSB 0x9010
#define AUDIO_S16SYS AUDIO_S16LSB
#define MIX_DEFAULT_FORMAT AUDIO_S16SYS

typedef struct Mix_Chunk {
    int allocated;
    Uint8 *abuf;
    Uint32 alen;
    Uint8 volume;
} Mix_Chunk;

typedef struct Mix_Music Mix_Music;

typedef enum {
    MUS_NONE,
    MUS_CMD,
    MUS_WAV,
    MUS_MOD,
    MUS_MID,
    MUS_OGG,
    MUS_MP3,
    MUS_FLAC
} Mix_MusicType;

typedef enum {
    MIX_INIT_FLAC = 0x00000001,
    MIX_INIT_MOD  = 0x00000002,
    MIX_INIT_MP3  = 0x00000008,
    MIX_INIT_OGG  = 0x00000010
} MIX_InitFlags;

static inline int Mix_Init(int flags) { (void)flags; return 0; }
static inline void Mix_Quit(void) {}
static inline int Mix_OpenAudio(int freq, Uint16 fmt, int ch, int sz) {
    (void)freq; (void)fmt; (void)ch; (void)sz; return 0;
}
static inline void Mix_CloseAudio(void) {}
static inline int Mix_AllocateChannels(int n) { (void)n; return n; }
static inline int Mix_Volume(int ch, int vol) { (void)ch; (void)vol; return 0; }
static inline int Mix_VolumeMusic(int vol) { (void)vol; return 0; }

static inline Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc) { (void)src; (void)freesrc; return NULL; }
#define Mix_LoadWAV(file) Mix_LoadWAV_RW(SDL_RWFromFile(file, "rb"), 1)
static inline Mix_Chunk *Mix_QuickLoad_RAW(Uint8 *mem, Uint32 len) { (void)mem; (void)len; return NULL; }
static inline void Mix_FreeChunk(Mix_Chunk *chunk) { (void)chunk; }

static inline int Mix_PlayChannelTimed(int ch, Mix_Chunk *chunk, int loops, int ticks) {
    (void)ch; (void)chunk; (void)loops; (void)ticks; return -1;
}
#define Mix_PlayChannel(ch, chunk, loops) Mix_PlayChannelTimed(ch, chunk, loops, -1)
static inline int Mix_HaltChannel(int ch) { (void)ch; return 0; }
static inline int Mix_Playing(int ch) { (void)ch; return 0; }
static inline int Mix_Pause(int ch) { (void)ch; return 0; }
static inline int Mix_Resume(int ch) { (void)ch; return 0; }
static inline int Mix_VolumeChunk(Mix_Chunk *chunk, int vol) { (void)chunk; (void)vol; return 0; }
static inline int Mix_SetDistance(int ch, Uint8 dist) { (void)ch; (void)dist; return 0; }
static inline int Mix_SetPosition(int ch, Sint16 angle, Uint8 dist) { (void)ch; (void)angle; (void)dist; return 0; }

static inline Mix_Music *Mix_LoadMUS(const char *file) { (void)file; return NULL; }
static inline Mix_Music *Mix_LoadMUS_RW(SDL_RWops *src, int freesrc) { (void)src; (void)freesrc; return NULL; }
static inline Mix_Music *Mix_LoadMUSType_RW(SDL_RWops *src, Mix_MusicType type, int freesrc) {
    (void)src; (void)type; (void)freesrc; return NULL;
}
static inline void Mix_FreeMusic(Mix_Music *music) { (void)music; }
static inline int Mix_PlayMusic(Mix_Music *music, int loops) { (void)music; (void)loops; return -1; }
static inline int Mix_HaltMusic(void) { return 0; }
static inline int Mix_PauseMusic(void) { return 0; }
static inline int Mix_ResumeMusic(void) { return 0; }
static inline int Mix_PlayingMusic(void) { return 0; }
static inline int Mix_PausedMusic(void) { return 0; }
static inline Mix_MusicType Mix_GetMusicType(const Mix_Music *m) { (void)m; return MUS_NONE; }
static inline int Mix_SetMusicPosition(double pos) { (void)pos; return 0; }
static inline int Mix_FadeInMusic(Mix_Music *m, int loops, int ms) { (void)m; (void)loops; (void)ms; return -1; }
static inline int Mix_FadeOutMusic(int ms) { (void)ms; return 0; }

static inline const char *Mix_GetError(void) { return "PicOS mixer stub"; }
static inline int Mix_QuerySpec(int *freq, Uint16 *fmt, int *ch) {
    if(freq)*freq=22050; if(fmt)*fmt=AUDIO_S16SYS; if(ch)*ch=2; return 1;
}
static inline int Mix_OpenAudioDevice(int freq, Uint16 fmt, int ch, int sz,
    const char *dev, int allowed_changes) {
    (void)freq; (void)fmt; (void)ch; (void)sz; (void)dev; (void)allowed_changes; return 0;
}

typedef void (*Mix_EffectFunc_t)(int chan, void *stream, int len, void *udata);
typedef void (*Mix_EffectDone_t)(int chan, void *udata);

static inline int Mix_RegisterEffect(int chan, Mix_EffectFunc_t f, Mix_EffectDone_t d, void *arg) {
    (void)chan; (void)f; (void)d; (void)arg; return 0;
}
static inline int Mix_UnregisterEffect(int chan, Mix_EffectFunc_t f) {
    (void)chan; (void)f; return 0;
}
static inline int Mix_UnregisterAllEffects(int chan) { (void)chan; return 0; }
static inline int Mix_SetPanning(int chan, Uint8 left, Uint8 right) {
    (void)chan; (void)left; (void)right; return 0;
}
