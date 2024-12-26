/*
 * Compression utilities
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
#include "arith.h"
#include "libnc.h"

void __attribute__((noreturn, format(printf, 1, 2))) fatal_error(const char *fmt, ...);

int64_t get_time_ms(void);

static inline uint64_t get_be64(const uint8_t *d)
{
    return ((uint64_t)get_be32(d) << 32) | get_be32(d + 4);
}

void fput_u8(FILE *f, uint8_t v);
int fget_u8(FILE *f, uint8_t *pv);
void fput_be16(FILE *f, uint16_t v);
int fget_be16(FILE *f, uint16_t *pv);
void fput_be32(FILE *f, uint32_t v);
int fget_be32(FILE *f, uint32_t *pv);
void fput_le16(FILE *f, uint16_t v);
int fget_le16(FILE *f, uint16_t *pv);
void fput_le32(FILE *f, uint32_t v);
int fget_le32(FILE *f, uint32_t *pv);
void fput_be64(FILE *f, uint64_t v);
void fput_f32(FILE *f, float v);
int fget_f32(FILE *f, float *pv);
uint32_t mpegts_crc32(const uint8_t *data, size_t len, uint32_t crc);
void fput_sgd_opt(FILE *f, const SGDOptParams *p);
int fget_sgd_opt(FILE *f, SGDOptParams *p);
void dump_sgd_opt_params(FILE *f, const SGDOptParams *p);

void write_sym(PutBitState *pb, const float *prob_table, int n_symb, int sym);
int read_sym(GetBitState *gb, const float *prob_table, int n_symb);

void create_debug_dir(char *debug_dir, size_t debug_dir_size,
                      const char *debug_path, const char *prefix);

#define INTERP_MAX_STEPS 8

typedef struct {
    int n_steps;
    float val[INTERP_MAX_STEPS + 1];
    int64_t pos[INTERP_MAX_STEPS];
    float decay_power;
} InterpParams;

float get_interp_param(const InterpParams *p, int64_t pos);
void dump_interp_param(FILE *f, const InterpParams *p);
void parse_interp_param(InterpParams *p, const char *r);
void skip_c(const char **pp, int c);

void term_init(void);
int term_get_key(void);
int exec_cmd(char **argv);

uint8_t *load_file2(size_t *psize, FILE *f);
uint8_t *load_file(size_t *psize, const char *filename);
int get_random_seed(void);

typedef struct FilePutBitState FilePutBitState;
FilePutBitState *file_put_bit_init(FILE *f);
void file_put_bits_flush(FilePutBitState *s, BOOL pad);
void file_put_bits(FilePutBitState *s, int n, int bits);
void file_put_bits_end(FilePutBitState *s);

typedef struct FileGetBitState FileGetBitState;
FileGetBitState *file_get_bit_init(FILE *f);
void file_get_bit_refill(FileGetBitState *s);
int file_get_bits(FileGetBitState *s, int n);
void file_get_bits_end(FileGetBitState *s);


