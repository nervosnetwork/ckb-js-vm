/*
 * C utilities
 * 
 * Copyright (c) 2018-2023 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef CUTILS_H
#define CUTILS_H

#include <stdlib.h>
#include <inttypes.h>

#define force_inline inline __attribute__((always_inline))
#define no_inline __attribute__((noinline))
#ifndef __unused
#define __unused __attribute__((unused))
#endif
#define __maybe_unused __attribute__((unused))
#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)
#ifndef offsetof
#define offsetof(type, field) ((size_t) &((type *)0)->field)
#endif
#define countof(x) (sizeof(x) / sizeof(x[0]))
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef int BOOL;

#ifndef FALSE
enum {
    FALSE = 0,
    TRUE = 1,
};
#endif

typedef uint8_t BOOL8;

typedef struct {
    uint16_t u16;
} nc_float16_t;

typedef struct {
    uint16_t u16;
} nc_bfloat16_t;

typedef struct {
    uint8_t u8;
} e5m2_t;

typedef struct {
    uint8_t u8;
} e4m3_t;

#if defined(__x86_64__)
static inline int64_t get_cycles(void)
{
    uint32_t low,high;
    int64_t val;
    asm volatile("rdtsc" : "=a" (low), "=d" (high));
    val = high;
    val <<= 32;
    val |= low;
    return val;
}
#elif defined(__i386__)
static inline int64_t get_cycles(void)
{
    int64_t val;
    asm volatile ("rdtsc" : "=A" (val));
    return val;
}
#else
#include <time.h>

/* in ns */
static inline int64_t get_cycles(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}
#endif

static inline int max_int(int a, int b)
{
    if (a > b)
        return a;
    else
        return b;
}

static inline int min_int(int a, int b)
{
    if (a < b)
        return a;
    else
        return b;
}

static inline size_t max_size_t(size_t a, size_t b)
{
    if (a > b)
        return a;
    else
        return b;
}

static inline size_t min_size_t(size_t a, size_t b)
{
    if (a < b)
        return a;
    else
        return b;
}

static inline ssize_t max_ssize_t(ssize_t a, ssize_t b)
{
    if (a > b)
        return a;
    else
        return b;
}

static inline ssize_t min_ssize_t(ssize_t a, ssize_t b)
{
    if (a < b)
        return a;
    else
        return b;
}

static inline float max_float(float a, float b)
{
    if (a > b)
        return a;
    else
        return b;
}

static inline float min_float(float a, float b)
{
    if (a < b)
        return a;
    else
        return b;
}

static inline int clamp_int(int val, int min_val, int max_val)
{
    if (val < min_val)
        return min_val;
    else if (val > max_val)
        return max_val;
    else
        return val;
}

static inline int ceil_udiv(int a, int b)
{
    return (a + b - 1) / b;
}

/* return floor(a / b) with b >= 1 */
static inline int floor_div(int a, int b)
{
    if (a >= 0) {
        return a / b;
    } else {
        return (a - b + 1) / b;
    }
}

/* return ceil(a / b) with b >= 1 */
static inline int ceil_div(int a, int b)
{
    if (a >= 0) {
        return (a + b - 1) / b;
    } else {
        return a / b;
    }
}

/* return r = a modulo b (0 <= r <= b - 1 */
static inline unsigned int mod_int(int a, unsigned int b)
{
    a = a % (int)b;
    if (a < 0)
        a += b;
    return a;
}

/* b must be a power of two */
static inline int align_int(int a, int b)
{
    return (a + b - 1) & ~(b - 1);
}

static inline size_t align_size_t(size_t a, size_t b)
{
    return (a + b - 1) & ~(b - 1);
}

static inline float clamp_float(float val, float min_val, float max_val)
{
    if (val < min_val)
        return min_val;
    else if (val > max_val)
        return max_val;
    else
        return val;
}

typedef union {
    uint32_t u32;
    float f;
} f32_union;

static inline uint32_t float_as_uint(float f)
{
    f32_union u;
    u.f = f;
    return u.u32;
}

static inline float uint_as_float(uint32_t v)
{
    f32_union u;
    u.u32 = v;
    return u.f;
}

/* WARNING: undefined if a = 0 */
static inline int clz32(unsigned int a)
{
    return __builtin_clz(a);
}

/* WARNING: undefined if a = 0 */
static inline int clz64(uint64_t a)
{
    return __builtin_clzll(a);
}

/* WARNING: undefined if a = 0 */
static inline int ctz64(uint64_t a)
{
    return __builtin_ctzll(a);
}

/* WARNING: undefined if a = 0 */
static inline int ctz32(unsigned int a)
{
    return __builtin_ctz(a);
}

static inline int floor_log2(uint64_t a)
{
    return 63 - clz64(a);
}

static inline int ceil_log2(uint64_t a)
{
    if (a <= 1)
        return 0;
    else
        return 64 - clz64(a - 1);
}

struct __attribute__((packed)) packed_u64 {
    uint64_t v;
};

struct __attribute__((packed)) packed_u32 {
    uint32_t v;
};

struct __attribute__((packed)) packed_u16 {
    uint16_t v;
};

static inline uint64_t get_u64(const uint8_t *tab)
{
    return ((const struct packed_u64 *)tab)->v;
}

static inline int64_t get_i64(const uint8_t *tab)
{
    return (int64_t)((const struct packed_u64 *)tab)->v;
}

static inline void put_u64(uint8_t *tab, uint64_t val)
{
    ((struct packed_u64 *)tab)->v = val;
}

static inline uint32_t get_u32(const uint8_t *tab)
{
    return ((const struct packed_u32 *)tab)->v;
}

static inline int32_t get_i32(const uint8_t *tab)
{
    return (int32_t)((const struct packed_u32 *)tab)->v;
}

static inline void put_u32(uint8_t *tab, uint32_t val)
{
    ((struct packed_u32 *)tab)->v = val;
}

static inline uint32_t get_u16(const uint8_t *tab)
{
    return ((const struct packed_u16 *)tab)->v;
}

static inline int32_t get_i16(const uint8_t *tab)
{
    return (int16_t)((const struct packed_u16 *)tab)->v;
}

static inline void put_u16(uint8_t *tab, uint16_t val)
{
    ((struct packed_u16 *)tab)->v = val;
}

static inline uint32_t get_u8(const uint8_t *tab)
{
    return *tab;
}

static inline int32_t get_i8(const uint8_t *tab)
{
    return (int8_t)*tab;
}

static inline void put_u8(uint8_t *tab, uint8_t val)
{
    *tab = val;
}

static inline uint16_t bswap_16(uint16_t v)
{
    return __builtin_bswap16(v);
}

static inline uint32_t bswap_32(uint32_t v)
{
    return __builtin_bswap32(v);
}

static inline uint64_t bswap_64(uint64_t v)
{
    return __builtin_bswap64(v);
}

static inline uint32_t get_be32(const uint8_t *d)
{
    return bswap_32(get_u32(d));
}

static inline void put_be32(uint8_t *d, uint32_t v)
{
    put_u32(d, bswap_32(v));
}

#define GET_BITS_FAST_PADDING 3

/* Note: n must be such as 1 <= n <= 25. At most 3 bytes can be read
   after the last byte of the data */
static inline int get_bits_fast(const uint8_t *p, int *pindex, int n)
{
    int index;
    unsigned int val;

    index = *pindex;
    val = get_be32(p + (index >> 3));
    val = (val >> (32 - (index & 7) - n)) & ((1 << n) - 1);
    *pindex = index + n;
    return val;
}

#define PUT_BITS_FAST_PADDING 3

/* Note: the MSB bits of 'bits' must be zero. PUT_BITS_FAST_PADDING
   are read and written after the end of the exact size (in bytes). */
static inline void put_bits_fast(uint8_t *p, int *pindex, int n, int bits)
{
    int index;
    unsigned int val;
    index = *pindex;
    p += index >> 3;
    /* XXX: optimize */
    val = get_be32(p);
    val |= bits << (32 - (index & 7) - n);
    put_be32(p, val);
    index += n;
    *pindex = index;
}

static inline float squaref(float x)
{
    return x * x;
}

#define DUP8(a) a, a, a, a, a, a, a, a

void *mallocz(size_t size);
void pstrcpy(char *buf, int buf_size, const char *str);
char *pstrcat(char *buf, int buf_size, const char *s);
int strstart(const char *str, const char *val, const char **ptr);
BOOL has_suffix(const char *str, const char *suffix);
#ifdef _WIN32
void *memmem(const void *hs1, size_t hs_len, const void *ne1, size_t ne_len);
#endif
void *memdup(const void *ptr, size_t size);
char *strdup_len(const char *str, size_t len);

static inline void __buf_expand(void **pbuf, int *psize, int count, int elem_size)
{
    int new_size;
    if (unlikely(count > *psize)) {
        new_size = max_int(*psize + *psize / 2, count);
        *pbuf = realloc(*pbuf, new_size * elem_size);
        *psize = new_size;
    }
}

#define BUF_EXPAND(pbuf, psize, count) __buf_expand((void **)(pbuf), psize, count, sizeof(**(pbuf)))

typedef struct {
    uint8_t *buf;
    size_t size;
    size_t allocated_size;
} DynBuf;

void dbuf_init(DynBuf *s);
int __dbuf_realloc(DynBuf *s, size_t new_size);
static inline int dbuf_realloc(DynBuf *s, size_t new_size)
{
    if (unlikely(new_size > s->allocated_size))
        return __dbuf_realloc(s, new_size);
    else
        return 0;
}
void dbuf_write(DynBuf *s, size_t offset, const uint8_t *data, size_t len);
void dbuf_put(DynBuf *s, const void *data, size_t len);
int dbuf_put_self(DynBuf *s, size_t offset, size_t len);
static inline void dbuf_put_u16(DynBuf *s, uint16_t val)
{
    dbuf_put(s, (uint8_t *)&val, 2);
}
static inline void dbuf_put_u32(DynBuf *s, uint32_t val)
{
    dbuf_put(s, (uint8_t *)&val, 4);
}
static inline void dbuf_put_u64(DynBuf *s, uint64_t val)
{
    dbuf_put(s, (uint8_t *)&val, 8);
}
void dbuf_putc(DynBuf *s, uint8_t c);
void dbuf_putstr(DynBuf *s, const char *str);
void __attribute__((format(printf, 2, 3))) dbuf_printf(DynBuf *s, const char *fmt, ...);
void dbuf_free(DynBuf *s);

static inline int dbuf_error(DynBuf *s)
{
    return 0;
}

typedef struct {
    uint32_t *buf;
    size_t len;
    size_t allocated_len;
} DynBuf32;

void dbuf32_init(DynBuf32 *s);
void dbuf32_init_set(DynBuf32 *s, const uint32_t *buf, size_t len);

void __dbuf32_realloc(DynBuf32 *s, size_t new_len);
static inline void dbuf32_realloc(DynBuf32 *s, size_t new_len)
{
    if (unlikely(new_len > s->allocated_len))
        __dbuf32_realloc(s, new_len);
}

static inline void dbuf32_putc(DynBuf32 *s, uint32_t c)
{
    size_t len = s->len;
    dbuf32_realloc(s, len + 1);
    s->buf[len++] = c;
    s->len = len;
}

void dbuf32_put(DynBuf32 *s, const uint32_t *tab, size_t len);

void dbuf32_free(DynBuf32 *s);

#define UTF8_CHAR_LEN_MAX 6

int unicode_to_utf8(uint8_t *buf, unsigned int c);
int unicode_from_utf8(const uint8_t *p, int max_len, const uint8_t **pp);

static inline int from_hex(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else
        return -1;
}

int utf8_to_utf32(uint32_t **pout_buf, const uint8_t *buf,
                  size_t buf_len);
size_t utf32_to_utf8(uint8_t **pout_buf, const uint32_t *buf,
                     size_t buf_len);
BOOL is_valid_utf8(const uint8_t *buf, size_t buf_len);

char *get_si_prefix(char *buf, int buf_size, uint64_t val);

void path_combine(char *dest, size_t dest_size,
                  const char *base_path, const char *filename);

#endif /* CUTILS_H */
