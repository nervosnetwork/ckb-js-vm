#include <limits.h>
#include "my_math.h"
#include "my_setjmp.h"
#include "my_locale.h"
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

int ckb_exit(int8_t code);

#define CKB_SS (sizeof(size_t))
#define CKB_ALIGN (sizeof(size_t) - 1)
#define CKB_ONES ((size_t)-1 / UCHAR_MAX)
#define CKB_HIGHS (CKB_ONES * (UCHAR_MAX / 2 + 1))
#define CKB_HASZERO(x) (((x)-CKB_ONES) & ~(x)&CKB_HIGHS)

void *memchr(const void *src, int c, size_t n) {
    const unsigned char *s = src;
    c = (unsigned char)c;
#ifdef __GNUC__
    for (; ((uintptr_t)s & CKB_ALIGN) && n && *s != c; s++, n--)
        ;
    if (n && *s != c) {
        typedef size_t __attribute__((__may_alias__)) word;
        const word *w;
        size_t k = CKB_ONES * c;
        for (w = (const void *)s; n >= CKB_SS && !CKB_HASZERO(*w ^ k); w++, n -= CKB_SS)
            ;
        s = (const void *)w;
    }
#endif
    for (; n && *s != c; s++, n--)
        ;
    return n ? (void *)s : 0;
}

#define BITOP(a, b, op) ((a)[(size_t)(b) / (8 * sizeof *(a))] op(size_t) 1 << ((size_t)(b) % (8 * sizeof *(a))))

char *__strchrnul(const char *s, int c) {
    c = (unsigned char)c;
    if (!c) return (char *)s + strlen(s);

    for (; *s && *(unsigned char *)s != c; s++)
        ;
    return (char *)s;
}

char *strchr(const char *s, int c) {
    char *r = __strchrnul(s, c);
    return *(unsigned char *)r == (unsigned char)c ? r : 0;
}

int strncmp(const char *_l, const char *_r, size_t n) {
    const unsigned char *l = (void *)_l, *r = (void *)_r;
    if (!n--) return 0;
    for (; *l && *r && n && *l == *r; l++, r++, n--)
        ;
    return *l - *r;
}

size_t strspn(const char *s, const char *c) {
    const char *a = s;
    size_t byteset[32 / sizeof(size_t)] = {0};

    if (!c[0]) return 0;
    if (!c[1]) {
        for (; *s == *c; s++)
            ;
        return s - a;
    }

    for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++)
        ;
    for (; *s && BITOP(byteset, *(unsigned char *)s, &); s++)
        ;
    return s - a;
}

size_t strcspn(const char *s, const char *c) {
    const char *a = s;
    size_t byteset[32 / sizeof(size_t)];

    if (!c[0] || !c[1]) return __strchrnul(s, *c) - a;

    memset(byteset, 0, sizeof byteset);
    for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++)
        ;
    for (; *s && !BITOP(byteset, *(unsigned char *)s, &); s++)
        ;
    return s - a;
}

char *strpbrk(const char *s, const char *b) {
    s += strcspn(s, b);
    return *s ? (char *)s : 0;
}

static char *twobyte_strstr(const unsigned char *h, const unsigned char *n) {
    uint16_t nw = n[0] << 8 | n[1], hw = h[0] << 8 | h[1];
    for (h++; *h && hw != nw; hw = hw << 8 | *++h)
        ;
    return *h ? (char *)h - 1 : 0;
}

static char *threebyte_strstr(const unsigned char *h, const unsigned char *n) {
    uint32_t nw = (uint32_t)n[0] << 24 | n[1] << 16 | n[2] << 8;
    uint32_t hw = (uint32_t)h[0] << 24 | h[1] << 16 | h[2] << 8;
    for (h += 2; *h && hw != nw; hw = (hw | *++h) << 8)
        ;
    return *h ? (char *)h - 2 : 0;
}

static char *fourbyte_strstr(const unsigned char *h, const unsigned char *n) {
    uint32_t nw = (uint32_t)n[0] << 24 | n[1] << 16 | n[2] << 8 | n[3];
    uint32_t hw = (uint32_t)h[0] << 24 | h[1] << 16 | h[2] << 8 | h[3];
    for (h += 3; *h && hw != nw; hw = hw << 8 | *++h)
        ;
    return *h ? (char *)h - 3 : 0;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define BITOP(a, b, op) ((a)[(size_t)(b) / (8 * sizeof *(a))] op(size_t) 1 << ((size_t)(b) % (8 * sizeof *(a))))

static char *twoway_strstr(const unsigned char *h, const unsigned char *n) {
    const unsigned char *z;
    size_t l, ip, jp, k, p, ms, p0, mem, mem0;
    size_t byteset[32 / sizeof(size_t)] = {0};
    size_t shift[256];

    /* Computing length of needle and fill shift table */
    for (l = 0; n[l] && h[l]; l++) BITOP(byteset, n[l], |=), shift[n[l]] = l + 1;
    if (n[l]) return 0; /* hit the end of h */

    /* Compute maximal suffix */
    ip = -1;
    jp = 0;
    k = p = 1;
    while (jp + k < l) {
        if (n[ip + k] == n[jp + k]) {
            if (k == p) {
                jp += p;
                k = 1;
            } else
                k++;
        } else if (n[ip + k] > n[jp + k]) {
            jp += k;
            k = 1;
            p = jp - ip;
        } else {
            ip = jp++;
            k = p = 1;
        }
    }
    ms = ip;
    p0 = p;

    /* And with the opposite comparison */
    ip = -1;
    jp = 0;
    k = p = 1;
    while (jp + k < l) {
        if (n[ip + k] == n[jp + k]) {
            if (k == p) {
                jp += p;
                k = 1;
            } else
                k++;
        } else if (n[ip + k] < n[jp + k]) {
            jp += k;
            k = 1;
            p = jp - ip;
        } else {
            ip = jp++;
            k = p = 1;
        }
    }
    if (ip + 1 > ms + 1)
        ms = ip;
    else
        p = p0;

    /* Periodic needle? */
    if (memcmp(n, n + p, ms + 1)) {
        mem0 = 0;
        p = MAX(ms, l - ms - 1) + 1;
    } else
        mem0 = l - p;
    mem = 0;

    /* Initialize incremental end-of-haystack pointer */
    z = h;

    /* Search loop */
    for (;;) {
        /* Update incremental end-of-haystack pointer */
        if (z - h < l) {
            /* Fast estimate for MAX(l,63) */
            size_t grow = l | 63;
            const unsigned char *z2 = memchr(z, 0, grow);
            if (z2) {
                z = z2;
                if (z - h < l) return 0;
            } else
                z += grow;
        }

        /* Check last byte first; advance by shift on mismatch */
        if (BITOP(byteset, h[l - 1], &)) {
            k = l - shift[h[l - 1]];
            if (k) {
                if (k < mem) k = mem;
                h += k;
                mem = 0;
                continue;
            }
        } else {
            h += l;
            mem = 0;
            continue;
        }

        /* Compare right half */
        for (k = MAX(ms + 1, mem); n[k] && n[k] == h[k]; k++)
            ;
        if (n[k]) {
            h += k - ms;
            mem = 0;
            continue;
        }
        /* Compare left half */
        for (k = ms + 1; k > mem && n[k - 1] == h[k - 1]; k--)
            ;
        if (k <= mem) return (char *)h;
        h += p;
        mem = mem0;
    }
}

char *strstr(const char *h, const char *n) {
    /* Return immediately on empty needle */
    if (!n[0]) return (char *)h;

    /* Use faster algorithms for short needles */
    h = strchr(h, *n);
    if (!h || !n[1]) return (char *)h;
    if (!h[1]) return 0;
    if (!n[2]) return twobyte_strstr((void *)h, (void *)n);
    if (!h[2]) return 0;
    if (!n[3]) return threebyte_strstr((void *)h, (void *)n);
    if (!h[3]) return 0;
    if (!n[4]) return fourbyte_strstr((void *)h, (void *)n);

    return twoway_strstr((void *)h, (void *)n);
}

/* Copied from
 * https://github.com/bminor/musl/blob/46d1c7801bb509e1097e8fadbaf359367fa4ef0b/src/setjmp/riscv64/setjmp.S
 */
/* We need to use inline asm for easier compilation,
 * https://stackoverflow.com/a/42358235. */
/* We need __attribute__((naked)) to remove prologue and epilogue,
 * https://stackoverflow.com/a/42637729 */
__attribute__((naked)) int setjmp(jmp_buf b) {
    asm volatile(
        "sd s0,    0(a0)\n"
        "sd s1,    8(a0)\n"
        "sd s2,    16(a0)\n"
        "sd s3,    24(a0)\n"
        "sd s4,    32(a0)\n"
        "sd s5,    40(a0)\n"
        "sd s6,    48(a0)\n"
        "sd s7,    56(a0)\n"
        "sd s8,    64(a0)\n"
        "sd s9,    72(a0)\n"
        "sd s10,   80(a0)\n"
        "sd s11,   88(a0)\n"
        "sd sp,    96(a0)\n"
        "sd ra,    104(a0)\n"
        "li a0, 0\n"
        "ret\n");
}

__attribute__((naked)) void longjmp(jmp_buf b, int n) {
    asm volatile(
        "ld s0,    0(a0)\n"
        "ld s1,    8(a0)\n"
        "ld s2,    16(a0)\n"
        "ld s3,    24(a0)\n"
        "ld s4,    32(a0)\n"
        "ld s5,    40(a0)\n"
        "ld s6,    48(a0)\n"
        "ld s7,    56(a0)\n"
        "ld s8,    64(a0)\n"
        "ld s9,    72(a0)\n"
        "ld s10,   80(a0)\n"
        "ld s11,   88(a0)\n"
        "ld sp,    96(a0)\n"
        "ld ra,    104(a0)\n"
        "seqz a0, a1\n"
        "add a0, a0, a1\n"
        "ret\n");
}

__attribute__((naked)) void _longjmp(jmp_buf b, int n) {
    asm volatile(
        "ld s0,    0(a0)\n"
        "ld s1,    8(a0)\n"
        "ld s2,    16(a0)\n"
        "ld s3,    24(a0)\n"
        "ld s4,    32(a0)\n"
        "ld s5,    40(a0)\n"
        "ld s6,    48(a0)\n"
        "ld s7,    56(a0)\n"
        "ld s8,    64(a0)\n"
        "ld s9,    72(a0)\n"
        "ld s10,   80(a0)\n"
        "ld s11,   88(a0)\n"
        "ld sp,    96(a0)\n"
        "ld ra,    104(a0)\n"
        "seqz a0, a1\n"
        "add a0, a0, a1\n"
        "ret\n");
}

int strcoll(const char *l, const char *r) { return strcmp(l, r); }

int *__errno_location(void) {
    static int error = -1;
    return &error;
}

char *strerror(int e) {
    static char *errorstr = "There is an error";
    return errorstr;
}

int islower(int c) { return (unsigned)c - 'a' < 26; }

int isupper(int c) { return (unsigned)c - 'A' < 26; }

int tolower(int c) {
    if (isupper(c)) return c | 32;
    return c;
}

int toupper(int c) {
    if (islower(c)) return c & 0x5f;
    return c;
}

#define X(x) (((x) / 256 | (x)*256) % 65536)

const unsigned short **__ctype_b_loc(void) {
    static const unsigned short table[] = {
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        X(0x200), X(0x200), X(0x200), X(0x200),
        X(0x200), X(0x200), X(0x200), X(0x200), X(0x200), X(0x320), X(0x220), X(0x220), X(0x220), X(0x220), X(0x200),
        X(0x200), X(0x200), X(0x200), X(0x200), X(0x200), X(0x200), X(0x200), X(0x200), X(0x200), X(0x200), X(0x200),
        X(0x200), X(0x200), X(0x200), X(0x200), X(0x200), X(0x200), X(0x160), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0),
        X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0),
        X(0x8d8), X(0x8d8), X(0x8d8), X(0x8d8), X(0x8d8), X(0x8d8), X(0x8d8), X(0x8d8), X(0x8d8), X(0x8d8), X(0x4c0),
        X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x8d5), X(0x8d5), X(0x8d5), X(0x8d5), X(0x8d5),
        X(0x8d5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5),
        X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x8c5), X(0x4c0),
        X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x4c0), X(0x8d6), X(0x8d6), X(0x8d6), X(0x8d6), X(0x8d6), X(0x8d6),
        X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6),
        X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x8c6), X(0x4c0), X(0x4c0),
        X(0x4c0), X(0x4c0), X(0x200), 0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
        0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
    };

    static const unsigned short *const ptable = table + 128;
    return (void *)&ptable;
}

const int32_t **__ctype_toupper_loc(void) {
    static const int32_t table[] = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   2,   3,
        4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,
        26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
        48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  'A', 'B', 'C', 'D', 'E',
        'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 91,
        92,  93,  94,  95,  96,  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
        'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 123, 124, 125, 126, 127, 0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    };

    static const int32_t *const ptable = table + 128;

    return (void *)&ptable;
}

const int32_t **__ctype_tolower_loc(void) {
    static const int32_t table[] = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   2,   3,
        4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,
        26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
        48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  'a', 'b', 'c', 'd', 'e',
        'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 91,
        92,  93,  94,  95,  96,  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
        'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 123, 124, 125, 126, 127, 0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    };

    static const int32_t *const ptable = table + 128;

    return (void *)&ptable;
}

char *getenv(const char *name) { return 0; }

int isspace(int c) { return c == ' ' || (unsigned)c - '\t' < 5; }

// Copied from dietlibc
float strtof(const char *s, char **endptr) {
    register const char *p = s;
    register float value = 0.;
    int sign = +1;
    float factor;
    unsigned int expo;

    while (isspace(*p)) p++;

    switch (*p) {
        case '-':
            sign = -1; /* fall through */
        case '+':
            p++;
        default:
            break;
    }

    while ((unsigned int)(*p - '0') < 10u) value = value * 10 + (*p++ - '0');

    if (*p == '.') {
        factor = 1.;

        p++;
        while ((unsigned int)(*p - '0') < 10u) {
            factor *= 0.1;
            value += (*p++ - '0') * factor;
        }
    }

    if ((*p | 32) == 'e') {
        expo = 0;
        factor = 10.L;

        switch (*++p) {  // ja hier weiß ich nicht, was mindestens nach einem
                         // 'E' folgenden MUSS.
            case '-':
                factor = 0.1; /* fall through */
            case '+':
                p++;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                break;
            default:
                value = 0.L;
                p = s;
                goto done;
        }

        while ((unsigned int)(*p - '0') < 10u) expo = 10 * expo + (*p++ - '0');

        while (1) {
            if (expo & 1) value *= factor;
            if ((expo >>= 1) == 0) break;
            factor *= factor;
        }
    }

done:
    if (endptr != NULL) *endptr = (char *)p;

    return value * sign;
}

// Convert char to an int in base `base`,
// `base` must be 10 or 16, return -1 on error.
int char2int(char ch, unsigned int base) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (base == 16) {
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    }
    return -1;
}

#define ldbltype long double
double strtod(const char *s, char **endptr) {
    register const char *p = s;
    register ldbltype value = 0.;
    int sign = +1;
    unsigned int base = 10;
    ldbltype base_inverse = (ldbltype)1 / (ldbltype)base;
    ldbltype factor;
    unsigned int expo;
    unsigned int has_digits = 0;

    while (isspace(*p)) p++;

    switch (*p) {
        case '-':
            sign = -1; /* fall through */
        case '+':
            p++;
        case '0':
            p++;
            if ((*p | 32) == 'x') {
                base = 16;
                base_inverse = (ldbltype)1 / (ldbltype)base;
                p++;
            } else {
                p--;
            }
        default:
            break;
    }

    unsigned int current_value;
    while ((current_value = char2int(*p, base)) != -1) {
        p++;
        value = value * base + current_value;
        has_digits = 1;
    }

    if (*p == '.') {
        factor = 1.;

        p++;
        while ((current_value = char2int(*p, base)) != -1) {
            p++;
            factor *= base_inverse;
            value += current_value * factor;
            has_digits = 1;
        }
    }

    if ((*p | 32) == 'e' && base == 10) {
        expo = 0;
        factor = 10.;

        switch (*++p) {  // ja hier weiß ich nicht, was mindestens nach einem
                         // 'E' folgenden MUSS.
            case '-':
                factor = 0.1; /* fall through */
            case '+':
                p++;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                break;
            default:
                value = 0.;
                p = s;
                goto done;
        }

        while ((unsigned int)(*p - '0') < 10u) expo = 10 * expo + (*p++ - '0');

        while (1) {
            if (expo & 1) value *= factor;
            if ((expo >>= 1) == 0) break;
            factor *= factor;
        }
    }

    if ((*p | 32) == 'p' && base == 16) {
        // TODO: add specifier p support
        // https://cplusplus.com/reference/cstdlib/strtod/
        //  - A 0x or 0X prefix, then a sequence of hexadecimal digits (as in
        //  isxdigit) optionally containing a period which separates the whole
        //  and fractional number parts. Optionally followed by a power of 2
        //  exponent (a p or P character followed by an optional sign and a
        //  sequence of hexadecimal digits).
    }
done:
    if (endptr != NULL) {
        if (has_digits) {
            *endptr = (char *)p;
        } else {
            *endptr = (char *)s;
        }
    }

    return value * sign;
}

long double strtold(const char *s, char **endptr) {
    register const char *p = s;
    register long double value = 0.L;
    int sign = +1;
    long double factor;
    unsigned int expo;

    while (isspace(*p)) p++;

    switch (*p) {
        case '-':
            sign = -1; /* fall through */
        case '+':
            p++;
        default:
            break;
    }

    while ((unsigned int)(*p - '0') < 10u) value = value * 10 + (*p++ - '0');

    if (*p == '.') {
        factor = 1.;

        p++;
        while ((unsigned int)(*p - '0') < 10u) {
            factor *= 0.1;
            value += (*p++ - '0') * factor;
        }
    }

    if ((*p | 32) == 'e') {
        expo = 0;
        factor = 10.L;

        switch (*++p) {  // ja hier weiß ich nicht, was mindestens nach einem
                         // 'E' folgenden MUSS.
            case '-':
                factor = 0.1; /* fall through */
            case '+':
                p++;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                break;
            default:
                value = 0.L;
                p = s;
                goto done;
        }

        while ((unsigned int)(*p - '0') < 10u) expo = 10 * expo + (*p++ - '0');

        while (1) {
            if (expo & 1) value *= factor;
            if ((expo >>= 1) == 0) break;
            factor *= factor;
        }
    }

done:
    if (endptr != NULL) *endptr = (char *)p;

    return value * sign;
}

void exit(int status) { ckb_exit(status); }
void abort(void) { ckb_exit(-1); }

static void *__memrchr(const void *m, int c, size_t n) {
    const unsigned char *s = m;
    c = (unsigned char)c;
    while (n--)
        if (s[n] == c) return (void *)(s + n);
    return 0;
}

char *strrchr(char *s, int c) { return __memrchr(s, c, strlen(s) + 1); }

char *strcat(char *dest, const char *src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}
