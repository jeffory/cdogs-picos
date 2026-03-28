/*
    Minimal ENet stub for C-Dogs SDL on PicOS.
    Just provides type definitions so headers compile.
*/
#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct _ENetAddress { uint32_t host; uint16_t port; } ENetAddress;
typedef struct _ENetPacket { size_t dataLength; uint8_t *data; uint32_t flags; } ENetPacket;
typedef struct _ENetPeer { ENetAddress address; void *data; } ENetPeer;
typedef struct _ENetHost ENetHost;
typedef int ENetSocket;

typedef uint32_t enet_uint32;
typedef uint16_t enet_uint16;
typedef uint8_t  enet_uint8;

#define ENET_SOCKET_NULL -1

int enet_initialize(void);
void enet_deinitialize(void);
static inline int enet_address_set_host(ENetAddress *addr, const char *name) {
    (void)addr; (void)name; return -1;
}
static inline int enet_address_get_host_ip(const ENetAddress *addr, char *name, size_t nameLength) {
    (void)addr; if(name && nameLength > 0) name[0] = '\0'; return 0;
}
static inline int enet_address_get_host(const ENetAddress *addr, char *name, size_t nameLength) {
    (void)addr; if(name && nameLength > 0) name[0] = '\0'; return 0;
}
