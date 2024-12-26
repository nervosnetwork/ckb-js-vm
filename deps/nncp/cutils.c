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
#include <inttypes.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "cutils.h"

void pstrcpy(char *buf, int buf_size, const char *str)
{
    int c;
    char *q = buf;

    if (buf_size <= 0)
        return;

    for(;;) {
        c = *str++;
        if (c == 0 || q >= buf + buf_size - 1)
            break;
        *q++ = c;
    }
    *q = '\0';
}

/* strcat and truncate. */
char *pstrcat(char *buf, int buf_size, const char *s)
{
    int len;
    len = strlen(buf);
    if (len < buf_size)
        pstrcpy(buf + len, buf_size - len, s);
    return buf;
}

int strstart(const char *str, const char *val, const char **ptr)
{
    const char *p, *q;
    p = str;
    q = val;
    while (*q != '\0') {
        if (*p != *q)
            return 0;
        p++;
        q++;
    }
    if (ptr)
        *ptr = p;
    return 1;
}

BOOL has_suffix(const char *str, const char *suffix)
{
    size_t len = strlen(str);
    size_t slen = strlen(suffix);
    return (len >= slen && !memcmp(str + len - slen, suffix, slen));
}

#ifdef _WIN32
void *memmem(const void *hs1, size_t hs_len, const void *ne1, size_t ne_len)
{
    const uint8_t *hs = hs1;
    const uint8_t *ne = ne1;
    size_t i, len;
    
    if (ne_len == 0)
        return (void *)hs;
    if (ne_len == 1)
        return memchr(hs, ne[0], hs_len);
    if (ne_len > hs_len)
        return NULL;

    /* XXX: inefficient */
    len = hs_len - ne_len;
    for(i = 0; i <= len; i++) {
        if (memcmp(hs + i, ne, ne_len) == 0)
            return (void *)(hs + i);
    }
    return NULL;
}
#endif

void *mallocz(size_t size)
{
    return calloc(1, size);
}

void *memdup(const void *ptr, size_t size)
{
    void *ptr1;
    ptr1 = malloc(size);
    memcpy(ptr1, ptr, size);
    return ptr1;
}

char *strdup_len(const char *str, size_t len)
{
    char *r;

    r = malloc(len + 1);
    memcpy(r, str, len);
    r[len] = '\0';
    return r;
}

void dbuf_init(DynBuf *s)
{
    memset(s, 0, sizeof(*s));
}

int __dbuf_realloc(DynBuf *s, size_t new_size)
{
    new_size = max_int(new_size, s->allocated_size + s->allocated_size / 2);
    s->buf = realloc(s->buf, new_size);
    s->allocated_size = new_size;
    return 0;
}

void dbuf_write(DynBuf *s, size_t offset, const uint8_t *data, size_t len)
{
    size_t end;
    end = offset + len;
    dbuf_realloc(s, end);
    memcpy(s->buf + offset, data, len);
    if (end > s->size)
        s->size = end;
}

void dbuf_put(DynBuf *s, const void *data, size_t len)
{
    dbuf_write(s, s->size, data, len);
}

int dbuf_put_self(DynBuf *s, size_t offset, size_t len)
{
    if (unlikely((s->size + len) > s->allocated_size)) {
        if (dbuf_realloc(s, s->size + len))
            return -1;
    }
    memcpy(s->buf + s->size, s->buf + offset, len);
    s->size += len;
    return 0;
}

void dbuf_putc(DynBuf *s, uint8_t c)
{
    dbuf_write(s, s->size, &c, 1);
}

void dbuf_putstr(DynBuf *s, const char *str)
{
    dbuf_write(s, s->size, (const uint8_t *)str, strlen(str));
}

void dbuf_free(DynBuf *s)
{
    free(s->buf);
    memset(s, 0, sizeof(*s));
}

void dbuf_printf(DynBuf *s, const char *fmt, ...)
{
    va_list ap;
    size_t size;
    ssize_t ret;
    
    dbuf_realloc(s, s->size + 256); /* arbitrary initial size */
    size = s->allocated_size - s->size;
    va_start(ap, fmt);
    ret = vsnprintf((char *)(s->buf + s->size), size, fmt, ap);
    va_end(ap);
    if (ret >= size) {
        /* not enough space: do it again with the correct size */
        dbuf_realloc(s, s->size + ret + 1);
        size = s->allocated_size - s->size;
        va_start(ap, fmt);
        ret = vsnprintf((char *)(s->buf + s->size), size, fmt, ap);
        va_end(ap);
        assert(ret < size);
    }
    s->size += ret;
}

/* DynBuf32 */

void dbuf32_init(DynBuf32 *s)
{
    memset(s, 0, sizeof(*s));
}

void dbuf32_init_set(DynBuf32 *s, const uint32_t *buf, size_t len)
{
    s->buf = malloc(sizeof(buf[0]) * len);
    memcpy(s->buf, buf, sizeof(buf[0]) * len);
    s->len = len;
    s->allocated_len = len;
}

void __dbuf32_realloc(DynBuf32 *s, size_t new_len)
{
    new_len = max_int(new_len, s->allocated_len + s->allocated_len / 2);
    s->buf = realloc(s->buf, new_len * sizeof(uint32_t));
    s->allocated_len = new_len;
}

void dbuf32_put(DynBuf32 *s, const uint32_t *tab, size_t l)
{
    size_t len = s->len;
    dbuf32_realloc(s, len + l);
    memcpy(s->buf + len, tab, l * sizeof(tab[0]));
    s->len = len + l;
}

void dbuf32_free(DynBuf32 *s)
{
    free(s->buf);
    memset(s, 0, sizeof(*s));
}

/* Note: at most 31 bits are encoded. At most UTF8_CHAR_LEN_MAX bytes
   are output. */
int unicode_to_utf8(uint8_t *buf, unsigned int c)
{
    uint8_t *q = buf;

    if (c < 0x80) {
        *q++ = c;
    } else {
        if (c < 0x800) {
            *q++ = (c >> 6) | 0xc0;
        } else {
            if (c < 0x10000) {
                *q++ = (c >> 12) | 0xe0;
            } else {
                if (c < 0x00200000) {
                    *q++ = (c >> 18) | 0xf0;
                } else {
                    if (c < 0x04000000) {
                        *q++ = (c >> 24) | 0xf8;
                    } else if (c < 0x80000000) {
                        *q++ = (c >> 30) | 0xfc;
                        *q++ = ((c >> 24) & 0x3f) | 0x80;
                    } else {
                        return 0;
                    }
                    *q++ = ((c >> 18) & 0x3f) | 0x80;
                }
                *q++ = ((c >> 12) & 0x3f) | 0x80;
            }
            *q++ = ((c >> 6) & 0x3f) | 0x80;
        }
        *q++ = (c & 0x3f) | 0x80;
    }
    return q - buf;
}

static const unsigned int utf8_min_code[5] = {
    0x80, 0x800, 0x10000, 0x00200000, 0x04000000,
};

static const unsigned char utf8_first_code_mask[5] = {
    0x1f, 0xf, 0x7, 0x3, 0x1,
};

/* return -1 if error. *pp is not updated in this case. max_len must
   be >= 1. The maximum length for a UTF8 byte sequence is 6 bytes. */
int unicode_from_utf8(const uint8_t *p, int max_len, const uint8_t **pp)
{
    int l, c, b, i;

    c = *p++;
    if (c < 0x80) {
        *pp = p;
        return c;
    }
    switch(c) {
    case 0xc0 ... 0xdf:
        l = 1;
        break;
    case 0xe0 ... 0xef:
        l = 2;
        break;
    case 0xf0 ... 0xf7:
        l = 3;
        break;
    case 0xf8 ... 0xfb:
        l = 4;
        break;
    case 0xfc ... 0xfd:
        l = 5;
        break;
    default:
        return -1;
    }
    /* check that we have enough characters */
    if (l > (max_len - 1))
        return -1;
    c &= utf8_first_code_mask[l - 1];
    for(i = 0; i < l; i++) {
        b = *p++;
        if (b < 0x80 || b >= 0xc0)
            return -1;
        c = (c << 6) | (b & 0x3f);
    }
    if (c < utf8_min_code[l - 1])
        return -1;
    *pp = p;
    return c;
}

/* return the string length */
int utf8_to_utf32(uint32_t **pout_buf, const uint8_t *buf,
                  size_t buf_len)
{
    const uint8_t *buf_end;
    DynBuf32 out_buf;
    uint32_t c;
    
    dbuf32_init(&out_buf);

    buf_end = buf + buf_len;
    while (buf < buf_end) {
        c = unicode_from_utf8(buf, buf_end - buf, &buf);
        if (c == -1)
            break;
        dbuf32_putc(&out_buf, c);
    }
    *pout_buf = out_buf.buf;
    return out_buf.len;
}

size_t utf32_to_utf8(uint8_t **pout_buf, const uint32_t *buf,
                     size_t buf_len)
{
    DynBuf dbuf;
    size_t i;
    uint32_t c;
    uint8_t buf1[6];
    int l;
    
    dbuf_init(&dbuf);
    for(i = 0; i < buf_len; i++) {
        c = buf[i];
        l = unicode_to_utf8(buf1, c);
        dbuf_put(&dbuf, buf1, l);
    }
    /* add a trailing '\0' */
    dbuf_putc(&dbuf, '\0');
    *pout_buf = dbuf.buf;
    return dbuf.size - 1;
}

BOOL is_valid_utf8(const uint8_t *buf, size_t buf_len)
{
    const uint8_t *buf_end = buf + buf_len;
    int c;
    
    while (buf < buf_end) {
        if (*buf < 0x80) {
            buf++;
        } else {
            c = unicode_from_utf8(buf, buf_end - buf, &buf);
            if (c == -1 ||
                c > 0x10ffff ||
                (c >= 0xd800 && c <= 0xdfff)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

/* we print at least 3 significant digits with at most 5 chars, except
   if larger than 9999T. The value is rounded to zero. */
char *get_si_prefix(char *buf, int buf_size, uint64_t val)
{
    static const char suffixes[4] = "kMGT";
    uint64_t base;
    int i;

    if (val <= 999) {
        snprintf(buf, buf_size, "%" PRId64, val);
    } else {
        base = 1000;
        for(i=0;i<4;i++) {
            /* Note: we round to 0 */
            if (val < base * 10) {
                snprintf(buf, buf_size, "%0.2f%c", 
                         floor((val * 100.0) / base) / 100.0,
                         suffixes[i]);
                break;
            } else if (val < base * 100) {
                snprintf(buf, buf_size, "%0.1f%c", 
                         floor((val * 10.0) / base) / 10.0,
                         suffixes[i]);
                break;
            } else if (val < base * 1000 || (i == 3)) {
                snprintf(buf, buf_size,
                         "%" PRId64 "%c", 
                         val / base,
                         suffixes[i]);
                break;
            }
            base = base * 1000;
        }
    }
    return buf;
}

#ifdef _WIN32
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

static int path_is_absolute(const char *path)
{
    const char *p;
    p = strchr(path, ':');
    if (p)
        p++;
    else
        p = path;
    return (*p == PATH_SEP);
}

/* if filename is absolute, just copy it to dest. Otherwise, build a
   path to it by considering it is relative to base_path. URL are
   supported. */
void path_combine(char *dest, size_t dest_size,
                  const char *base_path, const char *filename)
{
    const char *p, *p1;
    size_t len;

    if (dest_size <= 0)
        return;
    if (path_is_absolute(filename)) {
        pstrcpy(dest, dest_size, filename);
    } else {
        p = strchr(base_path, ':');
        if (p)
            p++;
        else
            p = base_path;
        p1 = strrchr(base_path, PATH_SEP);
        if (p1)
            p1++;
        else
            p1 = base_path;
        if (p1 > p)
            p = p1;
        len = p - base_path;
        if (len > dest_size - 1)
            len = dest_size - 1;
        memcpy(dest, base_path, len);
        dest[len] = '\0';
        pstrcat(dest, dest_size, filename);
    }
}
