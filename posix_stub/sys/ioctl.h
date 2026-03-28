/* PicOS sys/ioctl.h stub — no terminal ioctls on bare metal */
#pragma once

#define TIOCGWINSZ 0
#define FIONREAD   0x541B

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

static inline int ioctl(int fd, unsigned long req, ...) { (void)fd; (void)req; return -1; }
