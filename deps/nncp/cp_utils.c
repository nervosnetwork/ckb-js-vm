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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>
#include <getopt.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <termios.h>
#include <sys/wait.h>
#endif

#include "cutils.h"
#include "libnc.h"
#include "cp_utils.h"

void fatal_error(const char *fmt, ...)
{
    va_list ap;
    
    va_start(ap, fmt);
    fprintf(stderr, "Fatal error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

int64_t get_time_ms(void)
{
#ifdef _WIN32
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000U);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000U);
#endif
}

void fput_u8(FILE *f, uint8_t v)
{
    fputc(v, f);
}

int fget_u8(FILE *f, uint8_t *pv)
{
    int c;
    c = fgetc(f);
    if (c < 0)
        return -1;
    *pv = c;
    return 0;
}

void fput_be16(FILE *f, uint16_t v)
{
    fputc(v >> 8, f);
    fputc(v >> 0, f);
}

int fget_be16(FILE *f, uint16_t *pv)
{
    uint8_t buf[2];
    if (fread(buf, 1, sizeof(buf), f) != sizeof(buf))
        return -1;
    *pv = (buf[0] << 8) |
        (buf[1] << 0);
    return 0;
}

void fput_be32(FILE *f, uint32_t v)
{
    fputc(v >> 24, f);
    fputc(v >> 16, f);
    fputc(v >> 8, f);
    fputc(v >> 0, f);
}

void fput_be64(FILE *f, uint64_t v)
{
    fput_be32(f, v >> 32);
    fput_be32(f, v);
}

int fget_be32(FILE *f, uint32_t *pv)
{
    uint8_t buf[4];
    if (fread(buf, 1, sizeof(buf), f) != sizeof(buf))
        return -1;
    *pv = get_be32(buf);
    return 0;
}

void fput_le16(FILE *f, uint16_t v)
{
    fputc(v >> 0, f);
    fputc(v >> 8, f);
}

int fget_le16(FILE *f, uint16_t *pv)
{
    uint8_t buf[2];
    if (fread(buf, 1, sizeof(buf), f) != sizeof(buf))
        return -1;
    *pv = (buf[0] << 0) |
        (buf[1] << 8);
    return 0;
}

void fput_le32(FILE *f, uint32_t v)
{
    fputc(v >> 0, f);
    fputc(v >> 8, f);
    fputc(v >> 16, f);
    fputc(v >> 24, f);
}

int fget_le32(FILE *f, uint32_t *pv)
{
    uint8_t buf[4];
    if (fread(buf, 1, sizeof(buf), f) != sizeof(buf))
        return -1;
    *pv = get_u32(buf);
    return 0;
}

static const uint32_t crc_table[256] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
	0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
	0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
	0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
	0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
	0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
	0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
	0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
	0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
	0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
	0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
	0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
	0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
	0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
	0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
	0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
	0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

uint32_t mpegts_crc32(const uint8_t *data, size_t len, uint32_t crc)
{
    size_t i;
    
    for (i = 0; i < len; i++)
        crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *data++) & 0xff];
    return crc;
}

void fput_sgd_opt(FILE *f, const SGDOptParams *p)
{
    fput_u8(f, p->algo);
    switch(p->algo) {
    case SGD_OPT_BASIC:
        break;
    case SGD_OPT_ADAM:
        fput_f32(f, p->u.adam.beta1);
        fput_f32(f, p->u.adam.beta2);
        fput_f32(f, p->u.adam.eps);
        fput_f32(f, p->u.adam.gradient_clip);
        break;
    default:
        abort();
    }
}

int fget_sgd_opt(FILE *f, SGDOptParams *p)
{
    uint8_t v8;
    
    if (fget_u8(f, &v8))
        return -1;
    p->algo = v8;
    switch(p->algo) {
    case SGD_OPT_BASIC:
        break;
    case SGD_OPT_ADAM:
        if (fget_f32(f, &p->u.adam.beta1))
            return -1;
        if (fget_f32(f, &p->u.adam.beta2))
            return -1;
        if (fget_f32(f, &p->u.adam.eps))
            return -1;
        if (fget_f32(f, &p->u.adam.gradient_clip))
            return -1;
        break;
    default:
        return -1;
    }
    return 0;
}

void dump_sgd_opt_params(FILE *f, const SGDOptParams *p)
{
    switch(p->algo) {
    case SGD_OPT_BASIC:
        fprintf(f, " sgd_opt=%s", 
               "none");
        break;
    case SGD_OPT_ADAM:
        fprintf(f, " sgd_opt=%s beta1=%g beta2=%g eps=%g gclip=%g wdecay=%g",
               "adam",
                p->u.adam.beta1,
                p->u.adam.beta2,
                p->u.adam.eps,
                p->u.adam.gradient_clip,
                p->u.adam.weight_decay);
        break;
    default:
        abort();
    }
}

typedef union {
    float f;
    uint32_t u32;
} f32;

void fput_f32(FILE *f, float v)
{
    f32 u;
    u.f = v;
    fput_be32(f, u.u32);
}

int fget_f32(FILE *f, float *pv)
{
    f32 u;
    if (fget_be32(f, &u.u32))
        return -1;
    *pv = u.f;
    return 0;
}

void write_sym(PutBitState *pb, const float *prob_table, int n_symb, int sym)
{
    int start, range, prob0, bit, range0;
    float p, p0;
    
    start = 0;
    range = n_symb;
    p = 1.0; /* invariant: p=sum(prob_table[start...start + range]) */
    while (range > 1) {
        range0 = range >> 1;
        p0 = vec_sum_f32(prob_table + start, range0);
        prob0 = lrintf(p0 * PROB_UNIT / p);
        prob0 = clamp_int(prob0, 1, PROB_UNIT - 1);
        bit = sym >= (start + range0);
        put_bit(pb, prob0, bit);
        if (bit) {
            start += range0;
            range = range - range0;
            p = p - p0;
        } else {
            p = p0;
            range = range0;
        }
    }
}

int read_sym(GetBitState *gb, const float *prob_table, int n_symb)
{
    int start, range, prob0, bit, range0;
    float p, p0;
    
    start = 0;
    range = n_symb;
    p = 1.0; /* invariant: p=sum(prob_table[start...start + range]) */
    while (range > 1) {
        range0 = range >> 1;
        p0 = vec_sum_f32(prob_table + start, range0);
        prob0 = lrintf(p0 * PROB_UNIT / p);
        prob0 = clamp_int(prob0, 1, PROB_UNIT - 1);
        bit = get_bit(gb, prob0);
        if (bit) {
            start += range0;
            range = range - range0;
            p = p - p0;
        } else {
            p = p0;
            range = range0;
        }
    }
    return start;
}

void create_debug_dir(char *debug_dir, size_t debug_dir_size,
                      const char *debug_path, const char *prefix)
{
    char name1[1024];
    struct tm *tm;
    time_t ti;
    
    snprintf(name1, sizeof(name1), "%s/%s", debug_path, prefix);
#ifdef _WIN32
    _mkdir(name1);
#else
    mkdir(name1, 0777);
#endif
    
    ti = time(NULL);
    tm = localtime(&ti);
    snprintf(debug_dir, debug_dir_size, "%s/%04u%02u%02u-%02u%02u%02u",
             name1,
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec);
#ifdef _WIN32
    _mkdir(debug_dir);
#else
    mkdir(debug_dir, 0777);
#endif
}

/**************************************************************/
/* piece-wise linear interpolation + optional power decay */
float get_interp_param(const InterpParams *p, int64_t pos)
{
    int i;
    float lr, t;
    int64_t pos0;

    pos0 = 0;
    for(i = 0; i < p->n_steps; i++) {
        if (pos < p->pos[i]) {
            if (i == 0)
                pos0 = 0;
            else
                pos0 = p->pos[i - 1];
            t = (float)(pos - pos0) / (float)(p->pos[i] - pos0);
            lr = p->val[i] + t * (p->val[i + 1] - p->val[i]);
            return lr;
        }
    }
    if (p->decay_power == 0.0f || p->n_steps == 0) {
        return p->val[p->n_steps];
    } else if (p->decay_power == 0.5f) {
        /* special case to be independant of pow() rounding */
        return p->val[p->n_steps] /
            sqrtf((float)pos / (float)p->pos[p->n_steps - 1]);
    } else {
        /* XXX: use libc independant pow() implementation to avoid
           rounding differences */
        return p->val[p->n_steps] *
            pow((float)pos / (float)p->pos[p->n_steps - 1], -p->decay_power);
    }
}

void dump_interp_param(FILE *f, const InterpParams *p)
{
    int i;
    fprintf(f, "%g", p->val[0]);
    for(i = 0; i < p->n_steps; i++) {
        fprintf(f, ",%" PRIu64 ",%g",
                p->pos[i], p->val[i + 1]);
    }
    if (p->decay_power != 0) {
        fprintf(f, ",p%g", p->decay_power);
    }
}

void skip_c(const char **pp, int c)
{
    const char *p;
    p = *pp;
    if (*p != c) {
        fprintf(stderr, "expecting '%c'\n", c);
        exit(1);
    }
    p++;
    *pp = p;
}

void parse_interp_param(InterpParams *p, const char *r)
{
    float lr;
                    
    lr = strtod(r, (char **)&r);
    p->n_steps = 0;
    p->val[0] = lr;
    p->decay_power = 0.0;
    if (*r != '\0') {
        skip_c(&r, ',');
        for(;;) {
            if (*r == 'p') {
                r++;
                p->decay_power = strtod(r, (char **)&r);
                if (*r != '\0')
                    fatal_error("extranous characters");
                break;
            } else {
                if (p->n_steps >= INTERP_MAX_STEPS)
                    fatal_error("too many steps");
                p->pos[p->n_steps] =
                    lrint(strtod(r, (char **)&r));
                
                skip_c(&r, ',');
                p->val[++p->n_steps] =
                    strtod(r, (char **)&r);
                if (*r == '\0')
                    break;
                skip_c(&r, ',');
            }
        }
    }
}

#ifdef _WIN32
void term_init(void)
{
}
int term_get_key(void)
{
    return 0;
}
#else
static struct termios oldtty;
static int old_fd0_flags;

static void term_exit(void)
{
    tcsetattr (0, TCSANOW, &oldtty);
    fcntl(0, F_SETFL, old_fd0_flags);
}

void term_init(void)
{
    struct termios tty;

    memset(&tty, 0, sizeof(tty));
    tcgetattr (0, &tty);
    oldtty = tty;
    old_fd0_flags = fcntl(0, F_GETFL);

    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                          |INLCR|IGNCR|ICRNL|IXON);
    tty.c_oflag |= OPOST;
    tty.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
    tty.c_cflag &= ~(CSIZE|PARENB);
    tty.c_cflag |= CS8;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    tcsetattr (0, TCSANOW, &tty);
    fcntl(0, F_SETFL, O_NONBLOCK);

    atexit(term_exit);
}

/* return 0 if no key */
int term_get_key(void)
{
    uint8_t ch;
    if (read(0, &ch, 1) == 1)
        return ch;
    else
        return 0;
}
#endif /* !_WIN32 */

#if defined(_WIN32)
/* better than nothing: use system() */
int exec_cmd(char **argv)
{
    DynBuf dbuf;    
    int ret;
    
    dbuf_init(&dbuf);
    while (*argv != NULL) {
        if (dbuf.size != 0)
            dbuf_putc(&dbuf, ' ');
        dbuf_putstr(&dbuf, *argv++);
    }
    dbuf_putc(&dbuf, '\0');
    ret = system((char *)dbuf.buf);
    dbuf_free(&dbuf);
    return ret;
}

#else
int exec_cmd(char **argv)
{
    int pid, status, ret, fd_count, i;

    pid = fork();
    if (pid == 0) {
        fd_count = sysconf (_SC_OPEN_MAX);
        for (i = 3; i < fd_count; i++)
            close(i);
        
        execvp(argv[0], argv);
        exit(1);
    } else if (pid < 0) {
        return 254;
    }

    for(;;) {
        ret = waitpid(pid, &status, 0);
        if (ret == pid && (WIFEXITED(status) || WIFSIGNALED(status)))
            break;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return 255; /* signaled */
    }
}
#endif

uint8_t *load_file2(size_t *psize, FILE *f)
{
    size_t size, pos;
    uint8_t *buf;
    
    pos = ftell(f);
    fseek(f, 0, SEEK_END);
    size = ftell(f) - pos;
    fseek(f, pos, SEEK_SET);
    buf = malloc(size + 1);
    if (fread(buf, 1, size, f) != size) {
        free(buf);
        buf = NULL;
        size = 0;
    } else {
        buf[size] = '\0';
    }
    if (psize)
        *psize = size;
    return buf;
}

uint8_t *load_file(size_t *psize, const char *filename)
{
    FILE *f;
    size_t size;
    uint8_t *buf;
    
    f = fopen(filename, "rb");
    if (!f) {
        perror(filename);
        exit(1);
    }
    buf = load_file2(&size, f);
    if (!buf) {
        fprintf(stderr, "%s: I/O error\n", filename);
        exit(1);
    }
    fclose(f);
    if (psize)
        *psize = size;
    return buf;
}

int get_random_seed(void)
{
    struct timeval tv;
    int seed;

    gettimeofday(&tv, NULL);
    seed = tv.tv_sec + tv.tv_usec;
    return seed;
}

#define FILE_PUT_BIT_BUF_SIZE 1024

struct FilePutBitState {
    FILE *f;
    uint8_t *buf;
    int alloc_buf_len;
    int buf_pos; /* in bits */
    int buf_len; /* in bits */
};

FilePutBitState *file_put_bit_init(FILE *f)
{
    FilePutBitState *s;
    s = mallocz(sizeof(*s));
    s->f = f;
    s->alloc_buf_len = FILE_PUT_BIT_BUF_SIZE + PUT_BITS_FAST_PADDING;
    s->buf = mallocz(s->alloc_buf_len);
    s->buf_pos = 0;
    s->buf_len = FILE_PUT_BIT_BUF_SIZE * 8 - 32;
    return s;
}

void file_put_bits_flush(FilePutBitState *s, BOOL pad)
{
    int len;

    if (pad) {
        len = (s->buf_pos + 7) / 8;
        if (len > 0) {
            fwrite(s->buf, 1, len, s->f);
            memset(s->buf, 0, s->alloc_buf_len);
            s->buf_pos = 0;
        }
    } else {
        len = s->buf_pos / 8;
        if (len > 0) {
            uint8_t b;
            fwrite(s->buf, 1, len, s->f);
            b = s->buf[len];
            memset(s->buf, 0, s->alloc_buf_len);
            s->buf_pos = s->buf_pos & 7;
            s->buf[0] = b;
        }
    }
}

void file_put_bits(FilePutBitState *s, int n, int bits)
{
    put_bits_fast(s->buf, &s->buf_pos, n, bits);
    if (s->buf_pos >= s->buf_len)
        file_put_bits_flush(s, FALSE);
}

void file_put_bits_end(FilePutBitState *s)
{
    free(s->buf);
    free(s);
}

/* XXX: untested */
#if 0
#define FILE_GET_BIT_BUF_SIZE 1024

typedef struct FileGetBitState FileGetBitState;

struct FileGetBitState {
    FILE *f;
    uint8_t *buf;
    int alloc_buf_len;
    int buf_pos; /* in bits */
    int buf_len; /* in bits */
    int read_buf_len; /* in bytes */
};

FileGetBitState *file_get_bit_init(FILE *f)
{
    FileGetBitState *s;
    s = mallocz(sizeof(*s));
    s->f = f;
    s->alloc_buf_len = FILE_GET_BIT_BUF_SIZE;
    s->buf = mallocz(s->alloc_buf_len);
    s->buf_pos = 0;
    s->buf_len = 0;
    s->read_buf_len = 0;
    return s;
}

void file_get_bit_refill(FileGetBitState *s)
{
    int len, pos;

    /* first free as much space as possible */
    if (s->buf_pos >= 8) {
        pos = s->buf_pos / 8;
        memmove(s->buf, s->buf + pos, s->read_buf_len - pos);
        s->read_buf_len -= pos;
        s->buf_pos -= pos * 8;
        s->buf_len -= pos * 8;
    }
    len = s->alloc_buf_len - s->read_buf_len;
    if (len > 0) {
        len = fread(s->buf + s->read_buf_len, 1, len, s->f);
        s->read_buf_len += len;
        /* pad with zeros in case of EOF */
        len = s->alloc_buf_len - s->read_buf_len;
        if (len > 0) {
            memset(s->buf + s->read_buf_len, 0, len);
            s->read_buf_len += len;
        }
    }
    /* set the length so that we never try to read bits outside the
       buffer */
    s->buf_len = (s->read_buf_len - GET_BITS_FAST_PADDING - 4) * 8;
}

int file_get_bits(FileGetBitState *s, int n)
{
    if (s->buf_pos >= s->buf_len)
        file_get_bit_refill(s);
    return get_bits_fast(s->buf, &s->buf_pos, n);
}

void file_get_bits_end(FileGetBitState *s)
{
    free(s->buf);
    free(s);
}
#endif
