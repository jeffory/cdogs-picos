/*
    C-Dogs SDL PicOS Port — Networking stubs
    Provides empty implementations for net functions referenced by game code.
    C-Dogs networking (ENet) is not available on PicOS.
*/
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* ENet types from our stub header */
#include <enet/enet.h>

/* Proto types (nanopb generated) */
#include "proto/msg.pb.h"
#include "c_array.h"

/* --- NetClient --- */
typedef struct {
    ENetHost *client;
    ENetPeer *peer;
    int ClientId;
    int FirstPlayerUID;
    bool Ready;
    ENetSocket scanner;
    uint16_t port;
    int ScanTicks;
    CArray ScannedAddrs;
    CArray scannedAddrBuf;
} NetClient;

extern NetClient gNetClient;
NetClient gNetClient;

void NetClientInit(NetClient *n) {
    if (n) memset(n, 0, sizeof(*n));
}
void NetClientTerminate(NetClient *n) { (void)n; }
void NetClientReset(NetClient *n) { (void)n; }
bool NetClientIsConnected(const NetClient *n) { (void)n; return false; }
void NetClientPoll(NetClient *n) { (void)n; }
void NetClientFlush(NetClient *n) { (void)n; }
void NetClientDisconnect(NetClient *n) { (void)n; }
void NetClientSendMsg(NetClient *n, int type, const void *data) {
    (void)n; (void)type; (void)data;
}
int NetClientOpen(NetClient *n) { (void)n; return 0; }
void NetClientConnect(NetClient *n, const ENetAddress addr) { (void)n; (void)addr; }
void NetClientScan(NetClient *n) { (void)n; }

/* --- NetServer --- */
typedef struct {
    int unused;
} NetPeerData;

typedef struct {
    ENetHost *server;
    ENetSocket listen;
    char hostname[12];
    int PrevCmd;
    int Cmd;
    int peerId;
} NetServer;

extern NetServer gNetServer;
NetServer gNetServer;

void NetServerInit(NetServer *n) {
    if (n) memset(n, 0, sizeof(*n));
}
void NetServerTerminate(NetServer *n) { (void)n; }
void NetServerReset(NetServer *n) { (void)n; }
void NetServerPoll(NetServer *n) { (void)n; }
void NetServerFlush(NetServer *n) { (void)n; }
void NetServerOpen(NetServer *n) { (void)n; }
void NetServerClose(NetServer *n) { (void)n; }
int NetServerSendMsg(NetServer *n, int peerId, int type, const void *data) {
    (void)n; (void)peerId; (void)type; (void)data;
    return 0;
}

/* --- Net utility stubs --- */
ENetPacket *NetEncode(const int e, const void *data) {
    (void)e; (void)data;
    return NULL;
}

bool NetDecode(ENetPacket *packet, void *dest, const pb_msgdesc_t *fields) {
    (void)packet; (void)dest; (void)fields;
    return false;
}

/* --- Net conversion stubs (from net_util.c) --- */
#include "cdogs/vector.h"
#include "cdogs/color.h"
#include "cdogs/draw/char_sprites.h"  /* for CharColors */
#include "cdogs/player.h"            /* for PlayerData */

/* Forward-declare nanopb message types used in net conversions */
#include "proto/msg.pb.h"

struct vec2 NetToVec2(const NVec2 v) {
    struct vec2 r = { v.x, v.y };
    return r;
}
NVec2 Vec2ToNet(const struct vec2 v) {
    NVec2 r = { v.x, v.y };
    return r;
}
struct vec2i Net2Vec2i(const NVec2i v) {
    struct vec2i r = { v.x, v.y };
    return r;
}
NVec2i Vec2i2Net(const struct vec2i v) {
    NVec2i r = { v.x, v.y };
    return r;
}

NColor Color2Net(const color_t c) {
    NColor r;
    r.RGBA = (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a;
    return r;
}
color_t Net2Color(const NColor c) {
    color_t r;
    r.r = (c.RGBA >> 24) & 0xFF;
    r.g = (c.RGBA >> 16) & 0xFF;
    r.b = (c.RGBA >> 8) & 0xFF;
    r.a = c.RGBA & 0xFF;
    return r;
}

NCharColors CharColors2Net(const CharColors c) {
    NCharColors r;
    memset(&r, 0, sizeof(r));
    r.has_Skin = true; r.Skin = Color2Net(c.Skin);
    r.has_Arms = true; r.Arms = Color2Net(c.Arms);
    r.has_Body = true; r.Body = Color2Net(c.Body);
    r.has_Legs = true; r.Legs = Color2Net(c.Legs);
    r.has_Hair = true; r.Hair = Color2Net(c.Hair);
    r.has_Feet = true; r.Feet = Color2Net(c.Feet);
    r.has_Facehair = true; r.Facehair = Color2Net(c.Facehair);
    r.has_Hat = true; r.Hat = Color2Net(c.Hat);
    r.has_Glasses = true; r.Glasses = Color2Net(c.Glasses);
    return r;
}
CharColors Net2CharColors(const NCharColors c) {
    CharColors r;
    memset(&r, 0, sizeof(r));
    if (c.has_Skin) r.Skin = Net2Color(c.Skin);
    if (c.has_Arms) r.Arms = Net2Color(c.Arms);
    if (c.has_Body) r.Body = Net2Color(c.Body);
    if (c.has_Legs) r.Legs = Net2Color(c.Legs);
    if (c.has_Hair) r.Hair = Net2Color(c.Hair);
    if (c.has_Feet) r.Feet = Net2Color(c.Feet);
    if (c.has_Facehair) r.Facehair = Net2Color(c.Facehair);
    if (c.has_Hat) r.Hat = Net2Color(c.Hat);
    if (c.has_Glasses) r.Glasses = Net2Color(c.Glasses);
    return r;
}

NPlayerData NMakePlayerData(const PlayerData *p) {
    NPlayerData r;
    memset(&r, 0, sizeof(r));
    if (p) {
        strncpy(r.Name, p->name, sizeof(r.Name) - 1);
        r.Lives = p->Lives;
        r.HP = p->HP;
        r.UID = p->UID;
    }
    return r;
}

/* --- Additional NetClient stubs --- */
bool NetClientTryConnect(NetClient *n, const ENetAddress addr) {
    (void)n; (void)addr; return false;
}
void NetClientFindLANServers(NetClient *n) { (void)n; }
bool NetClientTryScanAndConnect(NetClient *n, const uint32_t host) {
    (void)n; (void)host; return false;
}

/* --- Additional NetServer stubs --- */
void NetServerSendGameStartMessages(NetServer *n, const int peerId) {
    (void)n; (void)peerId;
}

/* --- Additional net conversion stubs --- */
void Ammo2Net(pb_size_t *ammoCount, NAmmo *ammo, const CArray *a) {
    (void)a;
    if (ammoCount) *ammoCount = 0;
    (void)ammo;
}

NMissionComplete NMakeMissionComplete(const void *mo) {
    (void)mo;
    NMissionComplete r = {0};
    return r;
}

/* --- Mouse stubs (mouse.c excluded) --- */
#include "cdogs/mouse.h"

void MouseInit(Mouse *m) { if (m) memset(m, 0, sizeof(*m)); }
void MouseReset(Mouse *m) { (void)m; }
void MousePrePoll(Mouse *m) { (void)m; }
void MousePostPoll(Mouse *m, const Uint32 ticks) { (void)m; (void)ticks; }
void MouseOnButtonDown(Mouse *m, Uint8 button) { (void)m; (void)button; }
void MouseOnButtonUp(Mouse *m, Uint8 button) { (void)m; (void)button; }
void MouseOnWheel(Mouse *m, const Sint32 x, const Sint32 y) { (void)m; (void)x; (void)y; }
bool MouseIsPressed(const Mouse *m, const int button) { (void)m; (void)button; return false; }
bool MouseHasMoved(const Mouse *m) { (void)m; return false; }
struct vec2i MouseWheel(const Mouse *m) {
    (void)m;
    struct vec2i r = { 0, 0 };
    return r;
}

/* --- Joystick stubs (joystick.c excluded) --- */
#include "cdogs/joystick.h"

void JoyInit(CArray *joys) { (void)joys; }
void JoyReset(CArray *joys) { (void)joys; }
void JoyLock(CArray *joys) { (void)joys; }
void JoyTerminate(CArray *joys) { (void)joys; }
bool JoyIsDown(const SDL_JoystickID id, const int cmd) { (void)id; (void)cmd; return false; }
bool JoyIsPressed(const SDL_JoystickID id, const int cmd) { (void)id; (void)cmd; return false; }
int JoyGetPressed(const SDL_JoystickID id) { (void)id; return 0; }
void JoyPrePoll(CArray *joys) { (void)joys; }
SDL_JoystickID JoyAdded(const Sint32 which) { (void)which; return -1; }
void JoyRemoved(const Sint32 which) { (void)which; }
void JoyOnButtonDown(const SDL_ControllerButtonEvent e) { (void)e; }
void JoyOnButtonUp(const SDL_ControllerButtonEvent e) { (void)e; }
void JoyOnAxis(const SDL_ControllerAxisEvent e) { (void)e; }
void JoyRumble(const SDL_JoystickID id, const float strength, const Uint32 length) {
    (void)id; (void)strength; (void)length;
}
void JoyImpact(const SDL_JoystickID id) { (void)id; }
const char *JoyName(const SDL_JoystickID id) { (void)id; return "None"; }
void JoyButtonNameColor(const SDL_JoystickID id, const int cmd, char *buf, color_t *color) {
    (void)id; (void)cmd;
    if (buf) buf[0] = '\0';
    if (color) memset(color, 0, sizeof(*color));
}

/* --- cwolfmap stubs (map_wolf.c excluded but referenced) --- */
void MapWolfInit(void) {}
void MapWolfTerminate(void) {}
int MapWolfScan(const char *path, char **title) {
    (void)path; (void)title; return -1;
}
int MapWolfLoad(void *map, const void *e, void *missions) {
    (void)map; (void)e; (void)missions; return -1;
}
void MapWolfLoadCampaignsFromSystem(void *campaigns) { (void)campaigns; }
void MapWolfN3DCheckAndLoadCustomQuiz(void *map) { (void)map; }
int CWGetType(const void *cw) { (void)cw; return 0; }

/* --- ENet library stubs --- */
int enet_initialize(void) { return 0; }
void enet_deinitialize(void) {}
