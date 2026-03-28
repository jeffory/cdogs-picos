/*
    Minimal nanopb stub for C-Dogs SDL on PicOS.
    Provides just enough type definitions for msg.pb.h to parse.
*/
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define PB_PROTO_HEADER_VERSION 40

/* Basic nanopb types */
typedef uint32_t pb_size_t;
typedef uint8_t pb_byte_t;

/* Message descriptor (opaque) */
typedef struct pb_msgdesc_s pb_msgdesc_t;
struct pb_msgdesc_s {
    const uint32_t *field_info;
    const pb_msgdesc_t * const *submsg_info;
    const void *default_value;
    bool (*field_callback)(void *, void *, void *);
    pb_size_t field_count;
    pb_size_t required_field_count;
    pb_size_t largest_tag;
};

/* Field iterator (for msg.pb.c) */
typedef struct {
    const pb_msgdesc_t *descriptor;
    void *message;
    pb_size_t field_index;
    pb_size_t required_field_index;
    pb_size_t submessage_index;
    pb_size_t tag;
    pb_size_t data_size;
    pb_size_t array_size;
    int pSize;
    void *pField;
    void *pData;
} pb_field_iter_t;

/* Byte array type */
#define PB_BYTES_ARRAY_T(n) struct { pb_size_t size; pb_byte_t bytes[n]; }
typedef PB_BYTES_ARRAY_T(1) pb_bytes_array_t;

/* Encoding/decoding types (stubs) */
typedef struct pb_istream_s pb_istream_t;
typedef struct pb_ostream_s pb_ostream_t;

struct pb_istream_s {
    bool (*callback)(pb_istream_t *stream, pb_byte_t *buf, size_t count);
    void *state;
    size_t bytes_left;
};

struct pb_ostream_s {
    bool (*callback)(pb_ostream_t *stream, const pb_byte_t *buf, size_t count);
    void *state;
    size_t max_size;
    size_t bytes_written;
};

/* Field info encoding macros (used by msg.pb.c) */
#define PB_FIELDINFO_ASSERT(x)
#define PB_ATYPE_STATIC    0
#define PB_ATYPE_POINTER   1
#define PB_ATYPE_CALLBACK  2
#define PB_HTYPE_REQUIRED  0
#define PB_HTYPE_OPTIONAL  1
#define PB_HTYPE_SINGULAR  2
#define PB_HTYPE_REPEATED  3
#define PB_HTYPE_FIXARRAY  4
#define PB_HTYPE_ONEOF     5
#define PB_LTYPE_BOOL      0
#define PB_LTYPE_VARINT    1
#define PB_LTYPE_UVARINT   2
#define PB_LTYPE_SVARINT   3
#define PB_LTYPE_FIXED32   4
#define PB_LTYPE_FIXED64   5
#define PB_LTYPE_BYTES     6
#define PB_LTYPE_STRING    7
#define PB_LTYPE_SUBMESSAGE 8
#define PB_LTYPE_EXTENSION 9
#define PB_LTYPE_FIXED_LENGTH_BYTES 10
#define PB_LTYPE_SUBMSG_W_CB 11

/* Size calculation macros */
#define PB_DATA_OFFSET(st, m) offsetof(st, m)
#define PB_DELTA(st, m1, m2) ((int)offsetof(st, m1) - (int)offsetof(st, m2))
#define PB_LAST_FIELD
#define PB_SIZE_MAX ((pb_size_t)-1)

/* PB_BIND macro — creates the message descriptor globals.
   In the real nanopb this sets up field info arrays.
   For our stub, just define the extern variable. */
#define AUTO 0
#define PB_BIND(msgtype, structtype, width) \
    const pb_msgdesc_t structtype ## _msg = {0};

/* Stub encode/decode functions */
static inline bool pb_encode(pb_ostream_t *s, const pb_msgdesc_t *f, const void *m) {
    (void)s; (void)f; (void)m; return false;
}
static inline bool pb_decode(pb_istream_t *s, const pb_msgdesc_t *f, void *m) {
    (void)s; (void)f; (void)m; return false;
}
static inline pb_ostream_t pb_ostream_from_buffer(pb_byte_t *buf, size_t sz) {
    pb_ostream_t s = {0}; (void)buf; (void)sz; return s;
}
static inline pb_istream_t pb_istream_from_buffer(const pb_byte_t *buf, size_t sz) {
    pb_istream_t s = {0}; (void)buf; (void)sz; return s;
}
