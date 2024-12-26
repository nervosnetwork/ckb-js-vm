/*
 * Arithmetic coder
 * 
 * Copyright (c) 2018-2019 Fabrice Bellard
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
#ifndef ARITH_H
#define ARITH_H

#define PROB_UNIT_BITS 15
#define PROB_UNIT (1 << PROB_UNIT_BITS)

typedef void PutBitWriteFunc(void *opaque, const uint8_t *buf, size_t buf_size);

typedef struct {
    uint32_t range;
    uint32_t low;
    uint8_t current_byte;
    uint32_t n_bytes;
    uint8_t *buf;
    size_t buf_size;
    size_t idx; /* current position in bytes */
    PutBitWriteFunc *write_func;
    void *opaque;
    uint64_t byte_count;
} PutBitState;

void put_bit_init(PutBitState *s, uint8_t *buf, int buf_size,
                  PutBitWriteFunc *write_func, void *opaque);
void put_bit(PutBitState *s, int prob0, int bit);
void put_bit_raw(PutBitState *s, int bit);
int64_t put_bit_flush(PutBitState *s);
int64_t put_bit_get_bit_count(PutBitState *s);

/* return the number of read bytes */
typedef ssize_t GetBitReadFunc(void *opaque, uint8_t *buf, size_t buf_size);

typedef struct {
    uint8_t *buf;
    int buf_len;
    int buf_size;
    int idx;
    uint32_t low;
    uint32_t range;
    BOOL eof_reached;
    GetBitReadFunc *read_func;
    void *opaque;
    uint64_t byte_count;
} GetBitState;

void get_bit_init(GetBitState *s, uint8_t *buf, size_t buf_size,
                  GetBitReadFunc *read_func, void *opaque);
int get_bit(GetBitState *s, int prob0);
int get_bit_raw(GetBitState *s);
int64_t get_bit_get_bit_count(GetBitState *s);

#endif /* ARITH_H */
