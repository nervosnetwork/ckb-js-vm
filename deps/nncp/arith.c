/*
 * Arithmetic coder
 * 
 * Copyright (c) 2018-2021 Fabrice Bellard
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>
#include <getopt.h>

#include "cutils.h"
#include "arith.h"

#define RANGE_MIN_BITS 16
#define RANGE_MIN ((0xff << (RANGE_MIN_BITS - 8)) + 1)
#define RANGE_MAX (0xff << RANGE_MIN_BITS)

//#define DUMP_PUT_BIT
//#define DUMP_GET_BIT

void put_bit_init(PutBitState *s, uint8_t *buf, int buf_size,
                  PutBitWriteFunc *write_func, void *opaque)
{
    s->low = 0;
    s->range = RANGE_MAX;
    s->current_byte = 0xff;
    s->n_bytes = 0;
    s->buf = buf;
    s->buf_size = buf_size;
    s->idx = 0;
    s->write_func = write_func;
    s->opaque = opaque;
    s->byte_count = 0;
    assert(PROB_UNIT <= RANGE_MIN);
}

static void put_byte(PutBitState *s, int v)
{
    s->buf[s->idx++] = v;
    if (unlikely(s->idx == s->buf_size)) {
        s->byte_count += s->idx;
        s->write_func(s->opaque, s->buf, s->idx);
        s->idx = 0;
    }
}

/* 0 <= v <= 0x1fe. The current output stream contains n_bytes with:
   current_byte, then (n_bytes - 1) x 0xff. The first byte cannot be
   0xff.
 */
static void put_val(PutBitState *s, int v)
{
    uint32_t carry, b;

#ifdef DUMP_PUT_BIT
    printf("  out=%d\n", v);
#endif
    if (v == 0xff) {
        s->n_bytes++;
    } else {
        if (s->n_bytes > 0) {
            carry = v >> 8;
            put_byte(s, s->current_byte + carry);
            b = (0xff + carry) & 0xff;
            while (s->n_bytes > 1) {
                put_byte(s, b);
                s->n_bytes--;
            }
        }
        s->n_bytes = 1;
        s->current_byte = v;
    }
}

static void put_val_flush(PutBitState *s)
{
    if (s->n_bytes > 0) {
        put_val(s, 0);
    }
}

static void put_bit_renorm(PutBitState *s)
{
    uint32_t v;
    /* after renormalisation:
       0 <= low <= RANGE_MAX
       RANGE_MIN <= range <= RANGE_MAX
       In the worst case before normalisation:
       low_max = 2 * RANGE_MAX hence v <= 0x1fe
    */
    while (s->range < RANGE_MIN) {
        v = s->low >> RANGE_MIN_BITS;
        put_val(s, v);
        s->low = (s->low & ((1 << RANGE_MIN_BITS) - 1)) << 8;
        s->range <<= 8;
    }
}

/* 0 < prob0 < PROB_UNIT */
void put_bit(PutBitState *s, int prob0, int bit)
{
    int range0;

    assert(s->range >= RANGE_MIN);
    range0 = ((uint64_t)s->range * prob0) >> PROB_UNIT_BITS;
    assert(range0 > 0);
    assert(range0 < s->range);
#if defined(DUMP_PUT_BIT)
    {
        static int count;
        printf("%d: range=%d b=%d range0=%d low=%d\n",
               count++, s->range, bit, range0, s->low);
    }
#endif
    if (!bit) {
        s->range = range0;
    } else {
        s->low += range0;
        s->range -= range0;
    }
    
    put_bit_renorm(s);
}

void put_bit_raw(PutBitState *s, int bit)
{
    int range0;
    
    assert(s->range >= RANGE_MIN);
    range0 = s->range >> 1;
    if (!bit) {
        s->range = range0;
    } else {
        s->low += range0;
        s->range -= range0;
    }
    
    put_bit_renorm(s);
}

/* return the minimum number of bits to be able to correctly decode */
int64_t put_bit_flush(PutBitState *s)
{
    int n, val, mask;

    /* force larger range */
    if (s->range < (1 << RANGE_MIN_BITS)) {
        put_val(s, s->low >> RANGE_MIN_BITS);
        s->low = (s->low & ((1 << RANGE_MIN_BITS) - 1)) << 8;
        s->range <<= 8;
    }

    /* largest n such as 2^n <= range */
    n = 0;
    while ((1 << (n + 1)) <= s->range)
        n++;
    assert(n >= RANGE_MIN_BITS && n <= (RANGE_MIN_BITS + 7));

    val = s->low;
    mask = (1 << n) - 1;
    if ((val & mask) != 0)
        val = (val + (1 << n)) & ~mask;
    assert(val >= s->low && val < s->low + s->range);

    put_val(s, val >> RANGE_MIN_BITS);
    put_val_flush(s);
    if (s->idx > 0) {
        s->byte_count += s->idx;
        s->write_func(s->opaque, s->buf, s->idx);
        s->idx = 0;
    }
    return (s->byte_count - 1) * 8 + (RANGE_MIN_BITS + 8 - n);
}

/* return the approximate number of written bits */
int64_t put_bit_get_bit_count(PutBitState *s)
{
    int n;
    n = 0;
    while ((1 << (n + 1)) <= s->range)
        n++;
    return (s->byte_count + s->idx + s->n_bytes) * 8 + (RANGE_MIN_BITS + 7 - n);
}

/****************************************/

static void refill(GetBitState *s)
{
    s->range <<= 8;
    s->low <<= 8;
    if (s->idx >= s->buf_len) {
        if (!s->read_func || s->eof_reached)
            goto zero_pad;
        s->buf_len = s->read_func(s->opaque, s->buf, s->buf_size);
        s->idx = 0;
        if (s->buf_len == 0) {
            s->eof_reached = TRUE;
        zero_pad:
            s->byte_count++;
            return; /* pad with zeros */
        } else {
            s->byte_count += s->buf_len;
        }
    }
#ifdef DUMP_GET_BIT
    printf("  in=%d\n", s->buf[s->idx]);
#endif
    s->low += s->buf[s->idx++];
}

void get_bit_init(GetBitState *s, uint8_t *buf, size_t buf_size,
                  GetBitReadFunc *read_func, void *opaque)
{
    int i;
    s->buf_size = buf_size;
    s->buf = buf;
    s->read_func = read_func;
    s->opaque = opaque;
    if (read_func) {
        s->buf_len = 0;
    } else {
        /* prefilled buffer */
        s->buf_len = s->buf_size;
    }
    s->byte_count = s->buf_len;
    s->eof_reached = FALSE;
    s->range = 0;
    s->low = 0;
    s->idx = 0;
    for(i = 0; i <= RANGE_MIN_BITS; i += 8) {
        refill(s);
    }
    s->range = RANGE_MAX;
}

/* 0 < prob0 < PROB_UNIT */
int get_bit(GetBitState *s, int prob0)
{
    int b, range0;

    assert(s->range >= RANGE_MIN);
    range0 = ((uint64_t)s->range * prob0) >> PROB_UNIT_BITS;
    assert(range0 > 0);
    assert(range0 < s->range);
    b = s->low >= range0;
#ifdef DUMP_GET_BIT
    {
        static int count;
        printf("%d: range=%d b=%d range0=%d low=%d\n", count++, s->range, b, range0, s->low);
    }
#endif
    if (b) {
        s->low -= range0;
        s->range -= range0;
    } else {
        s->range = range0;
    }
    while (s->range < RANGE_MIN)
        refill(s);
    return b;
}

/* no context */
int get_bit_raw(GetBitState *s)
{
    int b, range0;
    range0 = s->range >> 1;
    b = s->low >= range0;
    if (b) {
        s->low -= range0;
        s->range -= range0;
    } else {
        s->range = range0;
    }
    if (s->range < RANGE_MIN)
        refill(s);
    return b;
}

/* return the approximate number of read bits */
int64_t get_bit_get_bit_count(GetBitState *s)
{
    int n;
    n = 0;
    while ((1 << (n + 1)) <= s->range)
        n++;
    return (s->byte_count - s->buf_len + s->idx) * 8 - n - 1;
}
