/*
    PicOS SDL2 Type Shim for C-Dogs SDL
    Provides type definitions and stub functions so game code compiles
    without the real SDL2 library.
*/
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* ── Integer types ─────────────────────────────────────────────── */
typedef uint8_t  Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
typedef int8_t   Sint8;
typedef int16_t  Sint16;
typedef int32_t  Sint32;
typedef int64_t  Sint64;

typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;

/* ── Endian ────────────────────────────────────────────────────── */
#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321
#define SDL_BYTEORDER SDL_LIL_ENDIAN

static inline Uint16 SDL_Swap16(Uint16 x) {
    return (Uint16)((x << 8) | (x >> 8));
}
static inline Uint32 SDL_Swap32(Uint32 x) {
    return ((x << 24) | ((x << 8) & 0x00FF0000) |
            ((x >> 8) & 0x0000FF00) | (x >> 24));
}

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SDL_SwapLE16(x) (x)
#define SDL_SwapLE32(x) (x)
#define SDL_SwapBE16(x) SDL_Swap16(x)
#define SDL_SwapBE32(x) SDL_Swap32(x)
#else
#define SDL_SwapLE16(x) SDL_Swap16(x)
#define SDL_SwapLE32(x) SDL_Swap32(x)
#define SDL_SwapBE16(x) (x)
#define SDL_SwapBE32(x) (x)
#endif

/* ── Geometry ──────────────────────────────────────────────────── */
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;
typedef struct SDL_Point { int x, y; } SDL_Point;
typedef struct SDL_Color { Uint8 r, g, b, a; } SDL_Color;

/* ── Pixel formats ─────────────────────────────────────────────── */
#define SDL_PIXELFORMAT_UNKNOWN     0
#define SDL_PIXELFORMAT_INDEX8      0x13000001
#define SDL_PIXELFORMAT_RGB332      0x14110801
#define SDL_PIXELFORMAT_RGB444      0x15120C02
#define SDL_PIXELFORMAT_RGB555      0x15130F02
#define SDL_PIXELFORMAT_RGB565      0x15151002
#define SDL_PIXELFORMAT_RGB888      0x16161804
#define SDL_PIXELFORMAT_ARGB8888    0x16362004
#define SDL_PIXELFORMAT_RGBA8888    0x16462004
#define SDL_PIXELFORMAT_ABGR8888    0x16762004
#define SDL_PIXELFORMAT_BGRA8888    0x16862004
#define SDL_PIXELFORMAT_RGB24       0x17101803
#define SDL_PIXELFORMAT_BGR24       0x17401803
#define SDL_PIXELFORMAT_RGBA32      SDL_PIXELFORMAT_RGBA8888
#define SDL_PIXELFORMAT_BGRA32      SDL_PIXELFORMAT_BGRA8888
#define SDL_PREALLOC 0x00000001
#define SDL_RLEACCEL 0x00000002

typedef struct SDL_PixelFormat {
    Uint32 format;
    Uint8  BitsPerPixel;
    Uint8  BytesPerPixel;
    Uint8  padding[2];
    Uint32 Rmask, Gmask, Bmask, Amask;
    Uint8  Rloss, Gloss, Bloss, Aloss;
    Uint8  Rshift, Gshift, Bshift, Ashift;
} SDL_PixelFormat;

typedef struct SDL_Palette {
    int ncolors;
    SDL_Color *colors;
} SDL_Palette;

/* ── Surface ───────────────────────────────────────────────────── */
typedef struct SDL_Surface {
    Uint32 flags;
    SDL_PixelFormat *format;
    int w, h;
    int pitch;
    void *pixels;
    void *userdata;
    int locked;
    void *lock_data;
    SDL_Rect clip_rect;
    int refcount;
} SDL_Surface;

/* ── Opaque types ──────────────────────────────────────────────── */
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Cursor SDL_Cursor;
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_GameController SDL_GameController;
typedef struct SDL_Haptic SDL_Haptic;

typedef int SDL_JoystickID;
typedef int SDL_GLContext;
typedef int SDL_SpinLock;
typedef struct { int padding; } SDL_mutex;

/* ── Blend modes ───────────────────────────────────────────────── */
typedef enum {
    SDL_BLENDMODE_NONE  = 0x00000000,
    SDL_BLENDMODE_BLEND = 0x00000001,
    SDL_BLENDMODE_ADD   = 0x00000002,
    SDL_BLENDMODE_MOD   = 0x00000004
} SDL_BlendMode;

/* ── Renderer flip ─────────────────────────────────────────────── */
typedef enum {
    SDL_FLIP_NONE       = 0x00000000,
    SDL_FLIP_HORIZONTAL = 0x00000001,
    SDL_FLIP_VERTICAL   = 0x00000002
} SDL_RendererFlip;

/* ── Texture access ────────────────────────────────────────────── */
typedef enum {
    SDL_TEXTUREACCESS_STATIC,
    SDL_TEXTUREACCESS_STREAMING,
    SDL_TEXTUREACCESS_TARGET
} SDL_TextureAccess;

/* ── Renderer flags ────────────────────────────────────────────── */
#define SDL_RENDERER_SOFTWARE      0x00000001
#define SDL_RENDERER_ACCELERATED   0x00000002
#define SDL_RENDERER_PRESENTVSYNC  0x00000004
#define SDL_RENDERER_TARGETTEXTURE 0x00000008

/* ── Window flags ──────────────────────────────────────────────── */
#define SDL_WINDOW_FULLSCREEN          0x00000001
#define SDL_WINDOW_OPENGL              0x00000002
#define SDL_WINDOW_SHOWN               0x00000004
#define SDL_WINDOW_HIDDEN              0x00000008
#define SDL_WINDOW_BORDERLESS          0x00000010
#define SDL_WINDOW_RESIZABLE           0x00000020
#define SDL_WINDOW_MINIMIZED           0x00000040
#define SDL_WINDOW_MAXIMIZED           0x00000080
#define SDL_WINDOW_FULLSCREEN_DESKTOP  (SDL_WINDOW_FULLSCREEN | 0x00001000)
#define SDL_WINDOW_ALLOW_HIGHDPI       0x00002000
#define SDL_WINDOWPOS_UNDEFINED        0x1FFF0000
#define SDL_WINDOWPOS_CENTERED         0x2FFF0000

/* ── Init subsystem flags ──────────────────────────────────────── */
#define SDL_INIT_TIMER          0x00000001
#define SDL_INIT_AUDIO          0x00000010
#define SDL_INIT_VIDEO          0x00000020
#define SDL_INIT_JOYSTICK       0x00000200
#define SDL_INIT_HAPTIC         0x00001000
#define SDL_INIT_GAMECONTROLLER 0x00002000
#define SDL_INIT_EVENTS         0x00004000
#define SDL_INIT_EVERYTHING     0x0000FFFF

/* ── Scancode / Keycode ────────────────────────────────────────── */
typedef enum {
    SDL_SCANCODE_UNKNOWN = 0,
    SDL_SCANCODE_A = 4, SDL_SCANCODE_B, SDL_SCANCODE_C, SDL_SCANCODE_D,
    SDL_SCANCODE_E, SDL_SCANCODE_F, SDL_SCANCODE_G, SDL_SCANCODE_H,
    SDL_SCANCODE_I, SDL_SCANCODE_J, SDL_SCANCODE_K, SDL_SCANCODE_L,
    SDL_SCANCODE_M, SDL_SCANCODE_N, SDL_SCANCODE_O, SDL_SCANCODE_P,
    SDL_SCANCODE_Q, SDL_SCANCODE_R, SDL_SCANCODE_S, SDL_SCANCODE_T,
    SDL_SCANCODE_U, SDL_SCANCODE_V, SDL_SCANCODE_W, SDL_SCANCODE_X,
    SDL_SCANCODE_Y, SDL_SCANCODE_Z,
    SDL_SCANCODE_1 = 30, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_5, SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8,
    SDL_SCANCODE_9, SDL_SCANCODE_0,
    SDL_SCANCODE_RETURN = 40,
    SDL_SCANCODE_ESCAPE = 41,
    SDL_SCANCODE_BACKSPACE = 42,
    SDL_SCANCODE_TAB = 43,
    SDL_SCANCODE_SPACE = 44,
    SDL_SCANCODE_MINUS = 45,
    SDL_SCANCODE_EQUALS = 46,
    SDL_SCANCODE_LEFTBRACKET = 47,
    SDL_SCANCODE_RIGHTBRACKET = 48,
    SDL_SCANCODE_BACKSLASH = 49,
    SDL_SCANCODE_SEMICOLON = 51,
    SDL_SCANCODE_APOSTROPHE = 52,
    SDL_SCANCODE_GRAVE = 53,
    SDL_SCANCODE_COMMA = 54,
    SDL_SCANCODE_PERIOD = 55,
    SDL_SCANCODE_SLASH = 56,
    SDL_SCANCODE_CAPSLOCK = 57,
    SDL_SCANCODE_PRINTSCREEN = 70,
    SDL_SCANCODE_SCROLLLOCK = 71,
    SDL_SCANCODE_PAUSE = 72,
    SDL_SCANCODE_F1 = 58, SDL_SCANCODE_F2, SDL_SCANCODE_F3, SDL_SCANCODE_F4,
    SDL_SCANCODE_F5, SDL_SCANCODE_F6, SDL_SCANCODE_F7, SDL_SCANCODE_F8,
    SDL_SCANCODE_F9, SDL_SCANCODE_F10, SDL_SCANCODE_F11, SDL_SCANCODE_F12,
    SDL_SCANCODE_INSERT = 73,
    SDL_SCANCODE_HOME = 74,
    SDL_SCANCODE_PAGEUP = 75,
    SDL_SCANCODE_DELETE = 76,
    SDL_SCANCODE_END = 77,
    SDL_SCANCODE_PAGEDOWN = 78,
    SDL_SCANCODE_RIGHT = 79,
    SDL_SCANCODE_LEFT = 80,
    SDL_SCANCODE_DOWN = 81,
    SDL_SCANCODE_UP = 82,
    SDL_SCANCODE_NUMLOCKCLEAR = 83,
    SDL_SCANCODE_KP_DIVIDE = 84,
    SDL_SCANCODE_KP_MULTIPLY = 85,
    SDL_SCANCODE_KP_MINUS = 86,
    SDL_SCANCODE_KP_PLUS = 87,
    SDL_SCANCODE_KP_ENTER = 88,
    SDL_SCANCODE_KP_1 = 89,
    SDL_SCANCODE_KP_2 = 90,
    SDL_SCANCODE_KP_3 = 91,
    SDL_SCANCODE_KP_4 = 92,
    SDL_SCANCODE_KP_5 = 93,
    SDL_SCANCODE_KP_6 = 94,
    SDL_SCANCODE_KP_7 = 95,
    SDL_SCANCODE_KP_8 = 96,
    SDL_SCANCODE_KP_9 = 97,
    SDL_SCANCODE_KP_0 = 98,
    SDL_SCANCODE_KP_PERIOD = 99,
    SDL_SCANCODE_LCTRL = 224,
    SDL_SCANCODE_LSHIFT = 225,
    SDL_SCANCODE_LALT = 226,
    SDL_SCANCODE_RCTRL = 228,
    SDL_SCANCODE_RSHIFT = 229,
    SDL_SCANCODE_RALT = 230,
    SDL_NUM_SCANCODES = 512
} SDL_Scancode;

typedef Sint32 SDL_Keycode;
#define SDLK_SCANCODE_MASK (1 << 30)
#define SDL_SCANCODE_TO_KEYCODE(X) ((X) | SDLK_SCANCODE_MASK)

#define SDLK_UNKNOWN    0
#define SDLK_RETURN     '\r'
#define SDLK_ESCAPE     '\033'
#define SDLK_BACKSPACE  '\b'
#define SDLK_TAB        '\t'
#define SDLK_SPACE      ' '
#define SDLK_a          'a'
#define SDLK_b          'b'
#define SDLK_c          'c'
#define SDLK_d          'd'
#define SDLK_e          'e'
#define SDLK_f          'f'
#define SDLK_g          'g'
#define SDLK_h          'h'
#define SDLK_i          'i'
#define SDLK_j          'j'
#define SDLK_k          'k'
#define SDLK_l          'l'
#define SDLK_m          'm'
#define SDLK_n          'n'
#define SDLK_o          'o'
#define SDLK_p          'p'
#define SDLK_q          'q'
#define SDLK_r          'r'
#define SDLK_s          's'
#define SDLK_t          't'
#define SDLK_u          'u'
#define SDLK_v          'v'
#define SDLK_w          'w'
#define SDLK_x          'x'
#define SDLK_y          'y'
#define SDLK_z          'z'
#define SDLK_0          '0'
#define SDLK_1          '1'
#define SDLK_2          '2'
#define SDLK_3          '3'
#define SDLK_4          '4'
#define SDLK_5          '5'
#define SDLK_6          '6'
#define SDLK_7          '7'
#define SDLK_8          '8'
#define SDLK_9          '9'
#define SDLK_MINUS      '-'
#define SDLK_EQUALS     '='
#define SDLK_LEFTBRACKET '['
#define SDLK_RIGHTBRACKET ']'
#define SDLK_BACKSLASH  '\\'
#define SDLK_SEMICOLON  ';'
#define SDLK_QUOTE      '\''
#define SDLK_BACKQUOTE  '`'
#define SDLK_COMMA      ','
#define SDLK_PERIOD     '.'
#define SDLK_SLASH      '/'

#define SDLK_UP         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UP)
#define SDLK_DOWN       SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DOWN)
#define SDLK_LEFT       SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LEFT)
#define SDLK_RIGHT      SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RIGHT)
#define SDLK_INSERT     SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_INSERT)
#define SDLK_HOME       SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HOME)
#define SDLK_END        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_END)
#define SDLK_PAGEUP     SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEUP)
#define SDLK_PAGEDOWN   SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEDOWN)
#define SDLK_DELETE     SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DELETE)
#define SDLK_F1         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F1)
#define SDLK_F2         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F2)
#define SDLK_F3         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F3)
#define SDLK_F4         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F4)
#define SDLK_F5         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F5)
#define SDLK_F6         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F6)
#define SDLK_F7         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F7)
#define SDLK_F8         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F8)
#define SDLK_F9         SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F9)
#define SDLK_F10        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F10)
#define SDLK_F11        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F11)
#define SDLK_F12        SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F12)
#define SDLK_CAPSLOCK   SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CAPSLOCK)
#define SDLK_LCTRL      SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LCTRL)
#define SDLK_LSHIFT     SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LSHIFT)
#define SDLK_LALT       SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LALT)
#define SDLK_RCTRL      SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RCTRL)
#define SDLK_RSHIFT     SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RSHIFT)
#define SDLK_RALT       SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RALT)

/* ── Key modifier flags ────────────────────────────────────────── */
typedef enum {
    KMOD_NONE   = 0x0000,
    KMOD_LSHIFT = 0x0001,
    KMOD_RSHIFT = 0x0002,
    KMOD_LCTRL  = 0x0040,
    KMOD_RCTRL  = 0x0080,
    KMOD_LALT   = 0x0100,
    KMOD_RALT   = 0x0200,
    KMOD_SHIFT  = KMOD_LSHIFT | KMOD_RSHIFT,
    KMOD_CTRL   = KMOD_LCTRL | KMOD_RCTRL,
    KMOD_ALT    = KMOD_LALT | KMOD_RALT
} SDL_Keymod;

/* ── Events ────────────────────────────────────────────────────── */
typedef enum {
    SDL_FIRSTEVENT     = 0,
    SDL_QUIT           = 0x100,
    SDL_WINDOWEVENT    = 0x200,
    SDL_KEYDOWN        = 0x300,
    SDL_KEYUP          = 0x301,
    SDL_TEXTEDITING    = 0x302,
    SDL_TEXTINPUT      = 0x303,
    SDL_MOUSEMOTION    = 0x400,
    SDL_MOUSEBUTTONDOWN = 0x401,
    SDL_MOUSEBUTTONUP  = 0x402,
    SDL_MOUSEWHEEL     = 0x403,
    SDL_JOYAXISMOTION  = 0x600,
    SDL_JOYBUTTONDOWN  = 0x603,
    SDL_JOYBUTTONUP    = 0x604,
    SDL_JOYDEVICEADDED = 0x605,
    SDL_JOYDEVICEREMOVED = 0x606,
    SDL_CONTROLLERAXISMOTION = 0x650,
    SDL_CONTROLLERBUTTONDOWN = 0x651,
    SDL_CONTROLLERBUTTONUP   = 0x652,
    SDL_CONTROLLERDEVICEADDED = 0x653,
    SDL_CONTROLLERDEVICEREMOVED = 0x654,
    SDL_DROPFILE       = 0x1000,
    SDL_USEREVENT      = 0x8000,
    SDL_LASTEVENT      = 0xFFFF
} SDL_EventType;

typedef struct SDL_Keysym {
    SDL_Scancode scancode;
    SDL_Keycode sym;
    Uint16 mod;
    Uint32 unused;
} SDL_Keysym;

typedef struct SDL_KeyboardEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint8 state;
    Uint8 repeat;
    Uint8 padding2;
    Uint8 padding3;
    SDL_Keysym keysym;
} SDL_KeyboardEvent;

typedef struct SDL_TextInputEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    char text[32];
} SDL_TextInputEvent;

typedef struct SDL_MouseMotionEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Uint32 state;
    Sint32 x, y;
    Sint32 xrel, yrel;
} SDL_MouseMotionEvent;

typedef struct SDL_MouseButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Uint8 button;
    Uint8 state;
    Uint8 clicks;
    Uint8 padding1;
    Sint32 x, y;
} SDL_MouseButtonEvent;

typedef struct SDL_MouseWheelEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Sint32 x, y;
    Uint32 direction;
} SDL_MouseWheelEvent;

typedef struct SDL_JoyAxisEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 axis;
    Sint16 value;
} SDL_JoyAxisEvent;

typedef struct SDL_JoyButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 button;
    Uint8 state;
} SDL_JoyButtonEvent;

typedef struct SDL_JoyDeviceEvent {
    Uint32 type;
    Uint32 timestamp;
    Sint32 which;
} SDL_JoyDeviceEvent;

typedef struct SDL_ControllerAxisEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 axis;
    Sint16 value;
} SDL_ControllerAxisEvent;

typedef struct SDL_ControllerButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 button;
    Uint8 state;
} SDL_ControllerButtonEvent;

typedef struct SDL_ControllerDeviceEvent {
    Uint32 type;
    Uint32 timestamp;
    Sint32 which;
} SDL_ControllerDeviceEvent;

typedef struct SDL_WindowEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint8 event;
    Sint32 data1, data2;
} SDL_WindowEvent;

typedef struct SDL_DropEvent {
    Uint32 type;
    Uint32 timestamp;
    char *file;
    Uint32 windowID;
} SDL_DropEvent;

#define SDL_AUDIODEVICEADDED   0x1100
#define SDL_AUDIODEVICEREMOVED 0x1101

typedef struct SDL_AudioDeviceEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 which;
    Uint8 iscapture;
} SDL_AudioDeviceEvent;

typedef union SDL_Event {
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_TextInputEvent text;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_MouseWheelEvent wheel;
    SDL_JoyAxisEvent jaxis;
    SDL_JoyButtonEvent jbutton;
    SDL_JoyDeviceEvent jdevice;
    SDL_ControllerAxisEvent caxis;
    SDL_ControllerButtonEvent cbutton;
    SDL_ControllerDeviceEvent cdevice;
    SDL_WindowEvent window;
    SDL_DropEvent drop;
    SDL_AudioDeviceEvent adevice;
    Uint8 padding[56];
} SDL_Event;

/* ── Window event IDs ──────────────────────────────────────────── */
typedef enum {
    SDL_WINDOWEVENT_NONE,
    SDL_WINDOWEVENT_SHOWN,
    SDL_WINDOWEVENT_HIDDEN,
    SDL_WINDOWEVENT_EXPOSED,
    SDL_WINDOWEVENT_MOVED,
    SDL_WINDOWEVENT_RESIZED,
    SDL_WINDOWEVENT_SIZE_CHANGED,
    SDL_WINDOWEVENT_MINIMIZED,
    SDL_WINDOWEVENT_MAXIMIZED,
    SDL_WINDOWEVENT_RESTORED,
    SDL_WINDOWEVENT_ENTER,
    SDL_WINDOWEVENT_LEAVE,
    SDL_WINDOWEVENT_FOCUS_GAINED,
    SDL_WINDOWEVENT_FOCUS_LOST,
    SDL_WINDOWEVENT_CLOSE
} SDL_WindowEventID;

/* ── Mouse button constants ────────────────────────────────────── */
#define SDL_BUTTON_LEFT   1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT  3

/* ── Key state ─────────────────────────────────────────────────── */
#define SDL_PRESSED  1
#define SDL_RELEASED 0

/* ── SDL_RWops ─────────────────────────────────────────────────── */
typedef struct SDL_RWops {
    Sint64 (*size)(struct SDL_RWops *);
    Sint64 (*seek)(struct SDL_RWops *, Sint64, int);
    size_t (*read)(struct SDL_RWops *, void *, size_t, size_t);
    size_t (*write)(struct SDL_RWops *, const void *, size_t, size_t);
    int (*close)(struct SDL_RWops *);
    Uint32 type;
} SDL_RWops;

/* ── Renderer info ─────────────────────────────────────────────── */
typedef struct SDL_RendererInfo {
    const char *name;
    Uint32 flags;
    Uint32 num_texture_formats;
    Uint32 texture_formats[16];
    int max_texture_width;
    int max_texture_height;
} SDL_RendererInfo;

/* ── Version ───────────────────────────────────────────────────── */
typedef struct SDL_version {
    Uint8 major;
    Uint8 minor;
    Uint8 patch;
} SDL_version;

#define SDL_MAJOR_VERSION 2
#define SDL_MINOR_VERSION 0
#define SDL_PATCHLEVEL    14

#define SDL_VERSIONNUM(X, Y, Z) ((X)*1000 + (Y)*100 + (Z))
#define SDL_COMPILEDVERSION SDL_VERSIONNUM(2, 0, 14)
#define SDL_VERSION_ATLEAST(X, Y, Z) (SDL_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))
#define SDL_VERSION(x) do { (x)->major = 2; (x)->minor = 0; (x)->patch = 14; } while(0)

/* ── Hint constants ────────────────────────────────────────────── */
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"
#define SDL_HINT_RENDER_VSYNC "SDL_RENDER_VSYNC"
#define SDL_HINT_VIDEO_ALLOW_SCREENSAVER "SDL_VIDEO_ALLOW_SCREENSAVER"

/* ── Joystick/gamecontroller constants ─────────────────────────── */
typedef enum {
    SDL_CONTROLLER_AXIS_INVALID = -1,
    SDL_CONTROLLER_AXIS_LEFTX,
    SDL_CONTROLLER_AXIS_LEFTY,
    SDL_CONTROLLER_AXIS_RIGHTX,
    SDL_CONTROLLER_AXIS_RIGHTY,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
    SDL_CONTROLLER_AXIS_MAX
} SDL_GameControllerAxis;

typedef enum {
    SDL_CONTROLLER_BUTTON_INVALID = -1,
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK,
    SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_MAX
} SDL_GameControllerButton;

#define SDL_HAT_CENTERED  0x00
#define SDL_HAT_UP        0x01
#define SDL_HAT_RIGHT     0x02
#define SDL_HAT_DOWN      0x04
#define SDL_HAT_LEFT      0x08

typedef Sint16 SDL_JoystickPowerLevel;
typedef struct { int unused; } SDL_JoystickGUID;

/* ── Functions implemented in picos_sdl_impl.c ────────────────── */

/* Init / Quit — still stubs */
static inline int SDL_Init(Uint32 flags) { (void)flags; return 0; }
static inline int SDL_InitSubSystem(Uint32 flags) { (void)flags; return 0; }
static inline void SDL_QuitSubSystem(Uint32 flags) { (void)flags; }
static inline void SDL_Quit(void) {}
static inline Uint32 SDL_WasInit(Uint32 flags) { (void)flags; return 0; }

/* Error — still stubs */
static inline const char *SDL_GetError(void) { return "PicOS SDL shim"; }
static inline int SDL_SetError(const char *fmt, ...) { (void)fmt; return -1; }
static inline void SDL_ClearError(void) {}

/* Log — still stubs */
static inline void SDL_Log(const char *fmt, ...) { (void)fmt; }
static inline void SDL_LogError(int cat, const char *fmt, ...) { (void)cat; (void)fmt; }
static inline void SDL_LogWarn(int cat, const char *fmt, ...) { (void)cat; (void)fmt; }

/* Timer — implemented */
extern Uint32 SDL_GetTicks(void);
extern Uint64 SDL_GetPerformanceCounter(void);
extern Uint64 SDL_GetPerformanceFrequency(void);
extern void SDL_Delay(Uint32 ms);

/* Window — implemented */
extern SDL_Window *SDL_CreateWindow(const char *t, int x, int y, int w, int h, Uint32 f);
extern void SDL_DestroyWindow(SDL_Window *w);
static inline void SDL_SetWindowTitle(SDL_Window *w, const char *t) { (void)w; (void)t; }
static inline void SDL_SetWindowIcon(SDL_Window *w, SDL_Surface *i) { (void)w; (void)i; }
static inline void SDL_SetWindowSize(SDL_Window *w, int x, int y) { (void)w; (void)x; (void)y; }
extern void SDL_GetWindowSize(SDL_Window *w, int *x, int *y);
static inline int SDL_SetWindowFullscreen(SDL_Window *w, Uint32 f) { (void)w; (void)f; return 0; }
static inline void SDL_SetWindowMinimumSize(SDL_Window *w, int x, int y) { (void)w; (void)x; (void)y; }
static inline Uint32 SDL_GetWindowFlags(SDL_Window *w) { (void)w; return 0; }
static inline void SDL_ShowWindow(SDL_Window *w) { (void)w; }
static inline int SDL_GetWindowDisplayIndex(SDL_Window *w) { (void)w; return 0; }
static inline void SDL_SetWindowPosition(SDL_Window *w, int x, int y) { (void)w; (void)x; (void)y; }

/* Renderer — implemented */
extern SDL_Renderer *SDL_CreateRenderer(SDL_Window *w, int i, Uint32 f);
extern void SDL_DestroyRenderer(SDL_Renderer *r);
extern int SDL_SetRenderDrawColor(SDL_Renderer *r, Uint8 red, Uint8 g, Uint8 b, Uint8 a);
extern int SDL_RenderClear(SDL_Renderer *r);
extern void SDL_RenderPresent(SDL_Renderer *r);
extern int SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *s, const SDL_Rect *d);
extern int SDL_RenderCopyEx(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *s, const SDL_Rect *d, double a, const SDL_Point *c, SDL_RendererFlip f);
extern int SDL_SetRenderTarget(SDL_Renderer *r, SDL_Texture *t);
extern SDL_Texture *SDL_GetRenderTarget(SDL_Renderer *r);
extern int SDL_RenderSetLogicalSize(SDL_Renderer *r, int w, int h);
extern void SDL_RenderGetLogicalSize(SDL_Renderer *r, int *w, int *h);
extern int SDL_RenderFillRect(SDL_Renderer *r, const SDL_Rect *rc);
extern int SDL_RenderDrawRect(SDL_Renderer *r, const SDL_Rect *rc);
extern int SDL_RenderDrawLine(SDL_Renderer *r, int x1, int y1, int x2, int y2);
extern int SDL_RenderDrawPoint(SDL_Renderer *r, int x, int y);
extern int SDL_SetRenderDrawBlendMode(SDL_Renderer *r, SDL_BlendMode m);
extern int SDL_GetRendererInfo(SDL_Renderer *r, SDL_RendererInfo *i);
extern int SDL_GetRendererOutputSize(SDL_Renderer *r, int *w, int *h);
static inline int SDL_RenderSetClipRect(SDL_Renderer *r, const SDL_Rect *rc) { (void)r; (void)rc; return 0; }
static inline void SDL_RenderGetClipRect(SDL_Renderer *r, SDL_Rect *rc) { (void)r; if(rc) memset(rc,0,sizeof(*rc)); }
static inline SDL_bool SDL_RenderIsClipEnabled(SDL_Renderer *r) { (void)r; return SDL_FALSE; }

/* Texture — implemented */
extern SDL_Texture *SDL_CreateTexture(SDL_Renderer *r, Uint32 f, int a, int w, int h);
/* PicOS extension: texture over caller-owned pixels (no copy). */
extern SDL_Texture *PicosTextureBorrow(uint32_t *pixels, int w, int h);
extern SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *r, SDL_Surface *s);
extern void SDL_DestroyTexture(SDL_Texture *t);
extern int SDL_SetTextureBlendMode(SDL_Texture *t, SDL_BlendMode m);
extern int SDL_SetTextureAlphaMod(SDL_Texture *t, Uint8 a);
extern int SDL_SetTextureColorMod(SDL_Texture *t, Uint8 r, Uint8 g, Uint8 b);
extern int SDL_QueryTexture(SDL_Texture *t, Uint32 *f, int *a, int *w, int *h);
extern int SDL_LockTexture(SDL_Texture *t, const SDL_Rect *r, void **p, int *pitch);
extern void SDL_UnlockTexture(SDL_Texture *t);
extern int SDL_UpdateTexture(SDL_Texture *t, const SDL_Rect *r, const void *p, int pitch);

/* Surface — implemented */
extern SDL_Surface *SDL_CreateRGBSurface(Uint32 f, int w, int h, int d,
    Uint32 rm, Uint32 gm, Uint32 bm, Uint32 am);
extern SDL_Surface *SDL_CreateRGBSurfaceFrom(void *p, int w, int h, int d, int pitch,
    Uint32 rm, Uint32 gm, Uint32 bm, Uint32 am);
extern SDL_Surface *SDL_CreateRGBSurfaceWithFormat(Uint32 f, int w, int h, int d, Uint32 fmt);
extern void SDL_FreeSurface(SDL_Surface *s);
static inline int SDL_LockSurface(SDL_Surface *s) { (void)s; return 0; }
static inline void SDL_UnlockSurface(SDL_Surface *s) { (void)s; }
static inline int SDL_SetSurfaceBlendMode(SDL_Surface *s, SDL_BlendMode m) { (void)s; (void)m; return 0; }
static inline int SDL_SetSurfaceAlphaMod(SDL_Surface *s, Uint8 a) { (void)s; (void)a; return 0; }
static inline int SDL_SetColorKey(SDL_Surface *s, int f, Uint32 k) { (void)s; (void)f; (void)k; return 0; }
extern SDL_Surface *SDL_ConvertSurface(SDL_Surface *s, const SDL_PixelFormat *f, Uint32 flags);
static inline int SDL_BlitSurface(SDL_Surface *s, const SDL_Rect *sr, SDL_Surface *d, SDL_Rect *dr) {
    (void)s; (void)sr; (void)d; (void)dr; return 0;
}
static inline int SDL_UpperBlit(SDL_Surface *s, const SDL_Rect *sr, SDL_Surface *d, SDL_Rect *dr) {
    (void)s; (void)sr; (void)d; (void)dr; return 0;
}
extern int SDL_FillRect(SDL_Surface *s, const SDL_Rect *r, Uint32 c);
static inline int SDL_SetSurfacePalette(SDL_Surface *s, SDL_Palette *p) { (void)s; (void)p; return 0; }
extern SDL_Surface *SDL_ConvertSurfaceFormat(SDL_Surface *s, Uint32 f, Uint32 flags);
extern SDL_Surface *SDL_LoadBMP_RW(SDL_RWops *src, int freesrc);
#define SDL_LoadBMP(file) SDL_LoadBMP_RW(SDL_RWFromFile(file, "rb"), 1)
extern int SDL_RWclose(SDL_RWops *c); /* declared again below with RWops API */
static inline int SDL_SaveBMP_RW(SDL_Surface *s, SDL_RWops *d, int f) {
    (void)s;
    if (d && f) SDL_RWclose(d);
    return -1;
}
#define SDL_SaveBMP(s, file) SDL_SaveBMP_RW(s, SDL_RWFromFile(file, "wb"), 1)

/* Pixel manipulation — implemented */
extern Uint32 SDL_MapRGB(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b);
extern Uint32 SDL_MapRGBA(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
extern void SDL_GetRGBA(Uint32 pixel, const SDL_PixelFormat *f,
    Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a);
extern SDL_PixelFormat *SDL_AllocFormat(Uint32 f);
extern void SDL_FreeFormat(SDL_PixelFormat *f);
static inline SDL_Palette *SDL_AllocPalette(int n) { (void)n; return NULL; }
static inline void SDL_FreePalette(SDL_Palette *p) { (void)p; }

/* Events — implemented */
extern int SDL_PollEvent(SDL_Event *e);
extern void SDL_PumpEvents(void);
extern const Uint8 *SDL_GetKeyboardState(int *n);
extern Uint32 SDL_GetMouseState(int *x, int *y);
extern Uint8 SDL_EventState(Uint32 type, int state);
static inline SDL_Keymod SDL_GetModState(void) { return KMOD_NONE; }
static inline void SDL_StartTextInput(void) {}
static inline void SDL_StopTextInput(void) {}
static inline SDL_bool SDL_IsTextInputActive(void) { return SDL_FALSE; }

#define SDL_ENABLE  1
#define SDL_DISABLE 0
#define SDL_QUERY   (-1)

/* RWops — implemented */
extern SDL_RWops *SDL_RWFromFile(const char *f, const char *m);
extern SDL_RWops *SDL_RWFromMem(void *m, int sz);
extern int SDL_RWclose(SDL_RWops *c);
extern size_t SDL_RWread(SDL_RWops *c, void *p, size_t s, size_t n);
extern size_t SDL_RWwrite(SDL_RWops *c, const void *p, size_t s, size_t n);
extern Sint64 SDL_RWseek(SDL_RWops *c, Sint64 o, int w);
extern Sint64 SDL_RWtell(SDL_RWops *c);
extern Sint64 SDL_RWsize(SDL_RWops *c);

/* Clipboard */
static inline char *SDL_GetClipboardText(void) { return NULL; }
static inline int SDL_SetClipboardText(const char *t) { (void)t; return 0; }
static inline SDL_bool SDL_HasClipboardText(void) { return SDL_FALSE; }

/* Joystick */
static inline int SDL_NumJoysticks(void) { return 0; }
static inline SDL_bool SDL_IsGameController(int i) { (void)i; return SDL_FALSE; }
static inline SDL_Joystick *SDL_JoystickOpen(int i) { (void)i; return NULL; }
static inline void SDL_JoystickClose(SDL_Joystick *j) { (void)j; }
static inline const char *SDL_JoystickName(SDL_Joystick *j) { (void)j; return ""; }
static inline SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick *j) { (void)j; return 0; }
static inline int SDL_JoystickNumButtons(SDL_Joystick *j) { (void)j; return 0; }
static inline int SDL_JoystickNumAxes(SDL_Joystick *j) { (void)j; return 0; }
static inline int SDL_JoystickNumHats(SDL_Joystick *j) { (void)j; return 0; }
static inline Uint8 SDL_JoystickGetButton(SDL_Joystick *j, int b) { (void)j; (void)b; return 0; }
static inline Sint16 SDL_JoystickGetAxis(SDL_Joystick *j, int a) { (void)j; (void)a; return 0; }
static inline Uint8 SDL_JoystickGetHat(SDL_Joystick *j, int h) { (void)j; (void)h; return 0; }
static inline SDL_JoystickGUID SDL_JoystickGetGUID(SDL_Joystick *j) { (void)j; SDL_JoystickGUID g = {0}; return g; }

/* Game controller */
static inline SDL_GameController *SDL_GameControllerOpen(int i) { (void)i; return NULL; }
static inline void SDL_GameControllerClose(SDL_GameController *g) { (void)g; }
static inline const char *SDL_GameControllerName(SDL_GameController *g) { (void)g; return ""; }
static inline SDL_Joystick *SDL_GameControllerGetJoystick(SDL_GameController *g) { (void)g; return NULL; }
static inline Sint16 SDL_GameControllerGetAxis(SDL_GameController *g, SDL_GameControllerAxis a) { (void)g; (void)a; return 0; }
static inline Uint8 SDL_GameControllerGetButton(SDL_GameController *g, SDL_GameControllerButton b) { (void)g; (void)b; return 0; }

/* Haptic */
static inline SDL_Haptic *SDL_HapticOpen(int i) { (void)i; return NULL; }
static inline SDL_Haptic *SDL_HapticOpenFromJoystick(SDL_Joystick *j) { (void)j; return NULL; }
static inline void SDL_HapticClose(SDL_Haptic *h) { (void)h; }
static inline int SDL_HapticRumbleInit(SDL_Haptic *h) { (void)h; return -1; }
static inline int SDL_HapticRumblePlay(SDL_Haptic *h, float s, Uint32 l) { (void)h; (void)s; (void)l; return -1; }
static inline int SDL_HapticRumbleStop(SDL_Haptic *h) { (void)h; return -1; }
static inline int SDL_JoystickIsHaptic(SDL_Joystick *j) { (void)j; return 0; }

/* Cursor */
static inline SDL_Cursor *SDL_CreateSystemCursor(int id) { (void)id; return NULL; }
static inline void SDL_SetCursor(SDL_Cursor *c) { (void)c; }
static inline void SDL_FreeCursor(SDL_Cursor *c) { (void)c; }
static inline int SDL_ShowCursor(int t) { (void)t; return 0; }

/* Display */
typedef struct SDL_DisplayMode {
    Uint32 format;
    int w, h, refresh_rate;
    void *driverdata;
} SDL_DisplayMode;
static inline int SDL_GetNumVideoDisplays(void) { return 1; }
static inline int SDL_GetDisplayBounds(int d, SDL_Rect *r) {
    (void)d; if(r) { r->x=0; r->y=0; r->w=320; r->h=320; } return 0;
}
static inline int SDL_GetCurrentDisplayMode(int d, SDL_DisplayMode *m) {
    (void)d; if(m) { m->w=320; m->h=320; m->refresh_rate=30; } return 0;
}

/* Misc */
static inline const char *SDL_GetPlatform(void) { return "PicOS"; }
static inline int SDL_SetHint(const char *n, const char *v) { (void)n; (void)v; return 1; }
static inline char *SDL_GetBasePath(void) { return NULL; }
static inline char *SDL_GetPrefPath(const char *o, const char *a) { (void)o; (void)a; return NULL; }
static inline void SDL_free(void *p) { free(p); }
static inline void *SDL_malloc(size_t s) { return malloc(s); }
static inline void *SDL_calloc(size_t n, size_t s) { return calloc(n, s); }
static inline void *SDL_realloc(void *p, size_t s) { return realloc(p, s); }
static inline void SDL_GetVersion(SDL_version *v) { if(v) { v->major=2; v->minor=0; v->patch=14; } }

/* Threading stubs */
static inline SDL_mutex *SDL_CreateMutex(void) { return NULL; }
static inline void SDL_DestroyMutex(SDL_mutex *m) { (void)m; }
static inline int SDL_LockMutex(SDL_mutex *m) { (void)m; return 0; }
static inline int SDL_UnlockMutex(SDL_mutex *m) { (void)m; return 0; }

/* Rect helpers */
static inline SDL_bool SDL_HasIntersection(const SDL_Rect *a, const SDL_Rect *b) {
    (void)a; (void)b; return SDL_FALSE;
}
static inline SDL_bool SDL_IntersectRect(const SDL_Rect *a, const SDL_Rect *b, SDL_Rect *r) {
    (void)a; (void)b; (void)r; return SDL_FALSE;
}
static inline void SDL_UnionRect(const SDL_Rect *a, const SDL_Rect *b, SDL_Rect *r) {
    (void)a; (void)b; (void)r;
}
static inline SDL_bool SDL_PointInRect(const SDL_Point *p, const SDL_Rect *r) {
    if (!p || !r) return SDL_FALSE;
    return (p->x >= r->x && p->x < r->x + r->w && p->y >= r->y && p->y < r->y + r->h) ? SDL_TRUE : SDL_FALSE;
}
static inline SDL_bool SDL_RectEmpty(const SDL_Rect *r) {
    return (!r || r->w <= 0 || r->h <= 0) ? SDL_TRUE : SDL_FALSE;
}

/* System cursor IDs */
typedef enum {
    SDL_SYSTEM_CURSOR_ARROW,
    SDL_SYSTEM_CURSOR_IBEAM,
    SDL_SYSTEM_CURSOR_WAIT,
    SDL_SYSTEM_CURSOR_CROSSHAIR,
    SDL_SYSTEM_CURSOR_WAITARROW,
    SDL_SYSTEM_CURSOR_SIZENWSE,
    SDL_SYSTEM_CURSOR_SIZENESW,
    SDL_SYSTEM_CURSOR_SIZEWE,
    SDL_SYSTEM_CURSOR_SIZENS,
    SDL_SYSTEM_CURSOR_SIZEALL,
    SDL_SYSTEM_CURSOR_NO,
    SDL_SYSTEM_CURSOR_HAND,
    SDL_NUM_SYSTEM_CURSORS
} SDL_SystemCursor;

/* (SDL_AudioDeviceEvent moved to before SDL_Event) */

/* ── RWops seek constants ──────────────────────────────────────── */
#define RW_SEEK_SET 0
#define RW_SEEK_CUR 1
#define RW_SEEK_END 2

/* ── Additional functions ──────────────────────────────────────── */
extern SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *p, int w, int h, int d, int pitch, Uint32 fmt);
static inline const char *SDL_GetScancodeName(SDL_Scancode sc) { (void)sc; return ""; }
static inline void SDL_GetWindowPosition(SDL_Window *w, int *x, int *y) { (void)w; if(x)*x=0; if(y)*y=0; }
static inline void SDL_VideoQuit(void) {}
static inline void SDL_SetWindowGrab(SDL_Window *w, SDL_bool g) { (void)w; (void)g; }
static inline SDL_bool SDL_GetWindowGrab(SDL_Window *w) { (void)w; return SDL_FALSE; }
static inline int SDL_GetNumRenderDrivers(void) { return 0; }
static inline int SDL_GetRenderDriverInfo(int i, SDL_RendererInfo *info) { (void)i; (void)info; return -1; }
static inline int SDL_GL_SetAttribute(int attr, int value) { (void)attr; (void)value; return 0; }
static inline int SDL_GL_GetAttribute(int attr, int *value) { (void)attr; (void)value; return -1; }
static inline SDL_Scancode SDL_GetScancodeFromName(const char *name) { (void)name; return SDL_SCANCODE_UNKNOWN; }
static inline SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode sc) { (void)sc; return SDLK_UNKNOWN; }
static inline SDL_Scancode SDL_GetScancodeFromKey(SDL_Keycode key) { (void)key; return SDL_SCANCODE_UNKNOWN; }
static inline const char *SDL_GetKeyName(SDL_Keycode key) { (void)key; return ""; }
static inline int SDL_SetRelativeMouseMode(SDL_bool enabled) { (void)enabled; return 0; }
static inline SDL_bool SDL_GetRelativeMouseMode(void) { return SDL_FALSE; }
static inline void SDL_WarpMouseInWindow(SDL_Window *w, int x, int y) { (void)w; (void)x; (void)y; }
static inline Uint32 SDL_GetGlobalMouseState(int *x, int *y) { if(x)*x=0; if(y)*y=0; return 0; }

/* SDL_audio.h stub */
typedef Uint16 SDL_AudioFormat;
typedef void (*SDL_AudioCallback)(void *userdata, Uint8 *stream, int len);
typedef Uint32 SDL_AudioDeviceID;

typedef struct SDL_AudioSpec {
    int freq;
    SDL_AudioFormat format;
    Uint8 channels;
    Uint8 silence;
    Uint16 samples;
    Uint32 size;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;

static inline int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
    (void)desired; (void)obtained; return -1;
}
static inline void SDL_CloseAudio(void) {}
static inline void SDL_PauseAudio(int pause_on) { (void)pause_on; }
static inline void SDL_LockAudio(void) {}
static inline void SDL_UnlockAudio(void) {}
static inline SDL_AudioDeviceID SDL_OpenAudioDevice(const char *dev, int iscapture,
    const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int allowed_changes) {
    (void)dev; (void)iscapture; (void)desired; (void)obtained; (void)allowed_changes; return 0;
}
static inline void SDL_CloseAudioDevice(SDL_AudioDeviceID dev) { (void)dev; }
static inline void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on) { (void)dev; (void)pause_on; }

/* ── SDL_AudioCVT ──────────────────────────────────────────────── */
typedef struct SDL_AudioCVT {
    int needed;
    SDL_AudioFormat src_format;
    SDL_AudioFormat dst_format;
    double rate_incr;
    Uint8 *buf;
    int len;
    int len_cvt;
    int len_mult;
    double len_ratio;
} SDL_AudioCVT;

static inline int SDL_BuildAudioCVT(SDL_AudioCVT *cvt,
    SDL_AudioFormat src_format, Uint8 src_channels, int src_rate,
    SDL_AudioFormat dst_format, Uint8 dst_channels, int dst_rate) {
    (void)cvt; (void)src_format; (void)src_channels; (void)src_rate;
    (void)dst_format; (void)dst_channels; (void)dst_rate;
    if (cvt) { cvt->needed = 0; cvt->len_mult = 1; }
    return 0;
}
static inline int SDL_ConvertAudio(SDL_AudioCVT *cvt) { (void)cvt; return 0; }

/* Add adevice member to SDL_Event union — patch into the union via cast */
/* (cdogs uses ev.adevice) — redefine the union would break things,
   so we just add the field to the union. Actually we can't change it.
   Instead, let's ensure it compiles. */

/* end of picos_sdl.h */
