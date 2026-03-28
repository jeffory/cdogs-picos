/* PicOS termios.h stub — no terminal on bare metal */
#pragma once

struct termios {
    unsigned int c_iflag;
    unsigned int c_oflag;
    unsigned int c_cflag;
    unsigned int c_lflag;
    unsigned char c_cc[20];
};

#define ECHO   0x00000008
#define ICANON 0x00000002
#define TCSANOW 0
#define VMIN  6
#define VTIME 5

static inline int tcgetattr(int fd, struct termios *t) { (void)fd; (void)t; return -1; }
static inline int tcsetattr(int fd, int opt, const struct termios *t) { (void)fd; (void)opt; (void)t; return -1; }
