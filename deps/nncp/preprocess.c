/*
 * NNCP preprocessor
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
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <getopt.h>

#include "cutils.h"
#include "cp_utils.h"

/****************************************************************/

typedef uint16_t DataSymbol;

typedef struct Word {
    uint32_t next; /* -1 = end */
    uint32_t freq;
    int64_t score;
    uint32_t len;
    DataSymbol buf[2];
} Word;

typedef struct {
    Word *words;
    size_t word_count;
    size_t word_size;
    uint32_t *hash_table;
    int hash_size;
    int hash_bits;
} WordList;

static uint32_t hash_calc(const DataSymbol *buf, int len, int n_bits)
{
    uint32_t h;
    int i;

    h = 1;
    for(i = 0; i < len; i++) {
        h = h * 314159 + buf[i];
    }
    return h & ((1 << n_bits) - 1);
}

static void hash_resize(WordList *s, int hash_bits)
{
    int i, h;
    Word *p;
    
    s->hash_bits = hash_bits;
    s->hash_size = 1 << hash_bits;
    free(s->hash_table);
    s->hash_table = malloc(sizeof(s->hash_table[0]) * s->hash_size);
    for(i = 0; i < s->hash_size; i++)
        s->hash_table[i] = -1;
    for(i = 0; i < s->word_count; i++) {
        p = &s->words[i];
        h = hash_calc(p->buf, p->len, s->hash_bits);
        p->next = s->hash_table[h];
        s->hash_table[h] = i;
    }
}

static WordList *word_list_init(void)
{
    WordList *s;
    
    s = mallocz(sizeof(WordList));
    s->word_count = 0;
    s->word_size = 0;
    hash_resize(s, 12);
    return s;
}

static void word_list_end(WordList *s)
{
    free(s->words);
    free(s->hash_table);
    free(s);
}

int64_t hash_lookup_count;
int64_t hash_it_count;

/* the hash size contains HASH_SIZE_FACTOR times more entries */
#define HASH_SIZE_FACTOR 2

static Word *word_find_add(WordList *s, const DataSymbol *buf, int len, int add)
{
    uint32_t h, idx;
    Word *p;
    int i;

    assert(len >= 1);
    h = hash_calc(buf, len, s->hash_bits);
    idx = s->hash_table[h];
    hash_lookup_count++;
    while (idx != -1) {
        hash_it_count++;
        p = &s->words[idx];
        if (p->len == len && !memcmp(p->buf, buf, len * sizeof(buf[0])))
            return p;
        idx = p->next;
    }

    if (!add)
        return NULL;

    if (s->word_count >= s->word_size) {
        size_t new_size = s->word_size + s->word_size / 2;
        if (new_size < 32)
            new_size = 32;
        if (s->word_count + 1 > new_size)
            new_size = s->word_count + 1;
        s->words = realloc(s->words, new_size * sizeof(s->words[0]));
        s->word_size = new_size;

    }
    /* resize the hash table when needed */
    if ((s->word_count * HASH_SIZE_FACTOR) > s->hash_size) {
        int hash_bits = s->hash_bits;
        while ((s->word_count * HASH_SIZE_FACTOR) > (1 << hash_bits))
            hash_bits++;
        hash_resize(s, hash_bits);
        
        /* recompute the hash with the new hash table size */
        h = hash_calc(buf, len, s->hash_bits);
    }

    idx = s->word_count++;
    p = &s->words[idx];
    p->freq = 0;
    p->len = len;
    for(i = 0; i < len; i++)
        p->buf[i] = buf[i];
    p->next = s->hash_table[h];
    s->hash_table[h] = idx;
    return p;
}

/****************************************************************/

typedef struct {
    uint32_t len;
    char data[0];
} StringEntry;

typedef struct {
    StringEntry **tab;
    size_t size;
    size_t count;
} StringTable;

StringTable *string_table_init(void)
{
    StringTable *s;
    s = mallocz(sizeof(*s));
    return s;
}

void string_table_add(StringTable *s, const char *data, uint32_t data_len)
{
    size_t new_size;
    StringEntry **new_tab, *se;

    if ((s->count + 1) > s->size) {
        new_size = s->size * 3 / 2;
        if (new_size < s->count + 1)
            new_size = s->count + 1;
        new_tab = realloc(s->tab, sizeof(s->tab[0]) * new_size);
        s->tab = new_tab;
        s->size = new_size;
    }
    se = malloc(sizeof(StringEntry) + data_len + 1);
    se->len = data_len;
    memcpy(se->data, data, data_len);
    se->data[data_len] = '\0';
    s->tab[s->count++] = se;
}

void string_table_end(StringTable *s)
{
    size_t i;
    for(i = 0; i < s->count; i++) {
        free(s->tab[i]);
    }
    free(s->tab);
    free(s);
}

/****************************************************************/

//#define USE_CUT /* disable multiple word combining */

#define FRAC_BITS 10
#define FRAC_ONE (1 << FRAC_BITS)

#define MAX_WORDS_PER_ITER 100
#define SUBST_COST (int)(7.0 * FRAC_ONE + 0.5) /* in bits * FRAC_OnE */
#define TOT_FREQ_RED_BITS ((int)(1.3 * FRAC_ONE + 0.5)) /* log2(old_tot_freq/new_tot_freq) */

#define CH_NO_SPACE     1
#define CH_TO_UPPER     2
#define CH_FIRST_UPPER  3
#define CH_ESCAPE       4

/* separate words */
#define CH_CUT          0xffff

/* number of reserved symbols */
#define NS 256

void dump_word(FILE *f, WordList *s, uint32_t code, BOOL text_output)
{
    Word *p;
    
    if (code < NS) {
        if (text_output) {
            switch(code) {
            case '\n':
                fprintf(f, "\\n");
                break;
            case '\\':
                fprintf(f, "\\%c", code);
                break;
            case CH_TO_UPPER:
                fprintf(f, "\\u");
                break;
            case CH_FIRST_UPPER:
                fprintf(f, "\\c");
                break;
            case CH_NO_SPACE:
                fprintf(f, "\\S");
                break;
            default:
                fprintf(f, "%c", code);
                break;
            }
        } else {
            switch(code) {
            case '\n':
                fprintf(f, "\\n");
                break;
            case '\\':
                fprintf(f, "\\%c", code);
                break;
            default:
                fprintf(f, "%c", code);
                break;
            }
        }
    } else {
        code -= NS;
        assert(code < s->word_count);
        p = &s->words[code];
        dump_word(f, s, p->buf[0], text_output);
        dump_word(f, s, p->buf[1], text_output);
    }
}

int get_word_bytes(uint8_t *buf, int buf_size, WordList *s, uint32_t code)
{
    Word *p;
    int n;
    
    if (code < NS) {
        if (buf_size >= 1) {
            buf[0] = code;
            n = 1;
        } else {
            n = 0; /* not enough space */
        }
    } else {
        code -= NS;
        assert(code < s->word_count);
        p = &s->words[code];
        n = get_word_bytes(buf, buf_size, s, p->buf[0]);
        n += get_word_bytes(buf + n, buf_size - n, s, p->buf[1]);
    }
    return n;
}


typedef struct {
    WordList *s;
    uint32_t *char_freq;
} SortState;

static int word_freq_cmp2(const void *a1, const void *a2, void *arg)
{
    SortState *ss = arg;
    uint32_t c1 = *(DataSymbol *)a1;
    uint32_t c2 = *(DataSymbol *)a2;
    uint32_t freq1, freq2;

    if (c1 < NS)
        freq1 = ss->char_freq[c1];
    else
        freq1 = ss->s->words[c1 - NS].freq;

    if (c2 < NS)
        freq2 = ss->char_freq[c2];
    else
        freq2 = ss->s->words[c2 - NS].freq;

    if (freq1 < freq2)
        return 1;
    else if (freq1 == freq2)
        return 0;
    else
        return -1;
}

/* XXX: could remove */
#define SORT_MAX_LEN 512

static int word_lex_cmp(const void *a1, const void *a2, void *arg)
{
    SortState *ss = arg;
    uint32_t c1 = *(DataSymbol *)a1;
    uint32_t c2 = *(DataSymbol *)a2;
    uint8_t buf1[SORT_MAX_LEN];
    uint8_t buf2[SORT_MAX_LEN];
    int res, n1, n2;
    
    n1 = get_word_bytes(buf1, sizeof(buf1), ss->s, c1);
    n2 = get_word_bytes(buf2, sizeof(buf2), ss->s, c2);
    res = memcmp(buf1, buf2, min_int(n1, n2));
    if (res != 0)
        return res;
    if (n1 < n2)
        return -1;
    else if (n1 == n2)
        return 0;
    else
        return 1;
}


#if defined(_WIN32) || defined(__ANDROID__)

static void *rqsort_arg;
static int (*rqsort_cmp)(const void *, const void *, void *);

static int rqsort_cmp2(const void *p1, const void *p2)
{
    return rqsort_cmp(p1, p2, rqsort_arg);
}

/* not reentrant, but not needed with emscripten */
void rqsort(void *base, size_t nmemb, size_t size,
            int (*cmp)(const void *, const void *, void *),
            void *arg)
{
    rqsort_arg = arg;
    rqsort_cmp = cmp;
    qsort(base, nmemb, size, rqsort_cmp2);
}

#else

void rqsort(void *base, size_t n, size_t elem_size,
            int (*cmp)(const void *, const void *, void *),
            void *arg)
{
    qsort_r(base, n, elem_size, cmp, arg);
}

#endif /* !_WIN32 */

int sort_words(WordList *s, uint32_t **ptab, uint32_t *char_freq,
               BOOL sort_by_freq)
{
    uint32_t *tab, n_words;
    int i, j;
    SortState ss_s, *ss = &ss_s;

    /* sort the words */
    n_words = NS + s->word_count;
    tab = malloc(sizeof(tab[0]) * n_words);
    j = 0;
    for(i = 0; i < n_words; i++) {
        if (i >= NS && s->words[i - NS].freq == 0)
            continue;
        tab[j++] = i;
    }
    ss->s = s;
    ss->char_freq = char_freq;
    if (sort_by_freq) {
        rqsort(tab, j, sizeof(tab[0]), word_freq_cmp2, ss);
    } else {
        /* lexicographic sort for the words indexes >= NS */
        rqsort(tab + NS, j - NS, sizeof(tab[0]), word_lex_cmp, ss);
    }
    *ptab = tab;
    return j;
}

void save_words_debug(WordList *s, const char *filename,
                      const uint32_t *char_freq, uint32_t tot_freq,
                      const uint32_t *tab, int word_count)
{
    FILE *f;
    int i;
    uint32_t c, sum, freq;
    Word *p;
    
    f = fopen(filename, "wb");
    if (!f) {
        perror(filename);
        exit(1);
    }

    fprintf(f, "%7s %5s %s\n",
            "FREQ", "CUM%", "WORD");
    sum = 0;
    for(i = 0; i < word_count; i++) {
        c = tab[i];
        if (c < NS) {
            freq = char_freq[c];
        } else {
            p = &s->words[c - NS];
            freq = p->freq;
        }
        sum += freq;
        fprintf(f, "%7u %5.1f '", freq, (double)sum / tot_freq * 100);
        dump_word(f, s, c, TRUE);
        fprintf(f, "'\n");
    }
    
    fclose(f);
}

void save_words(WordList *s, const char *filename,
                const uint32_t *tab, int word_count)
{
    FILE *f;
    int i;
    
    f = fopen(filename, "wb");
    if (!f) {
        perror(filename);
        exit(1);
    }

    for(i = 0; i < word_count; i++) {
        dump_word(f, s, tab[i], FALSE);
        fprintf(f, "\n");
    }
    
    fclose(f);
}

void dump_word_bin(FILE *f, WordList *s, uint32_t *convert_table,
                    uint32_t c)
{
    Word *p;
    
    if (c < NS) {
        fput_be16(f, convert_table[c]);
    } else {
        c -= NS;
        assert(c < s->word_count);
        p = &s->words[c];
        if (p->freq == 0) {
            dump_word_bin(f, s, convert_table, p->buf[0]);
            dump_word_bin(f, s, convert_table, p->buf[1]);
        } else {
            fput_be16(f, convert_table[c + NS]);
        }
    }
}

void save_output(const DataSymbol *buf, size_t buf_len, WordList *s,
                 const char *out_filename, const uint32_t *tab, int word_count)
{
    FILE *fo;
    uint32_t *convert_table;
    size_t i;
    
    fo = fopen(out_filename, "wb");
    if (!fo) {
        perror(out_filename);
        exit(1);
    }

    /* build the convertion table */
    convert_table = malloc(sizeof(convert_table[0]) * (s->word_count + NS));
    for(i = 0; i < s->word_count + NS; i++)
        convert_table[i] = -1;
    for(i = 0; i < word_count; i++) {
        convert_table[tab[i]] = i;
    }

    for(i = 0; i < buf_len; i++) {
        if (buf[i] != CH_CUT) {
            dump_word_bin(fo, s, convert_table, buf[i]);
        }
    }
    free(convert_table);
    
    fclose(fo);
}

static int word_score_cmp(const void *a1, const void *a2)
{
    const Word *p1 = a1;
    const Word *p2 = a2;

    if (p1->score > p2->score)
        return -1;
    else if (p1->score == p2->score)
        return 0;
    else
        return 1;
}

static const uint16_t log2_table[FRAC_ONE] = {
 0x000, 0x001, 0x003, 0x004, 0x006, 0x007, 0x009, 0x00a,
 0x00b, 0x00d, 0x00e, 0x010, 0x011, 0x013, 0x014, 0x015,
 0x017, 0x018, 0x01a, 0x01b, 0x01d, 0x01e, 0x01f, 0x021,
 0x022, 0x024, 0x025, 0x026, 0x028, 0x029, 0x02b, 0x02c,
 0x02d, 0x02f, 0x030, 0x032, 0x033, 0x034, 0x036, 0x037,
 0x039, 0x03a, 0x03b, 0x03d, 0x03e, 0x040, 0x041, 0x042,
 0x044, 0x045, 0x046, 0x048, 0x049, 0x04b, 0x04c, 0x04d,
 0x04f, 0x050, 0x051, 0x053, 0x054, 0x055, 0x057, 0x058,
 0x05a, 0x05b, 0x05c, 0x05e, 0x05f, 0x060, 0x062, 0x063,
 0x064, 0x066, 0x067, 0x068, 0x06a, 0x06b, 0x06c, 0x06e,
 0x06f, 0x070, 0x072, 0x073, 0x074, 0x076, 0x077, 0x078,
 0x07a, 0x07b, 0x07c, 0x07e, 0x07f, 0x080, 0x082, 0x083,
 0x084, 0x086, 0x087, 0x088, 0x08a, 0x08b, 0x08c, 0x08e,
 0x08f, 0x090, 0x092, 0x093, 0x094, 0x095, 0x097, 0x098,
 0x099, 0x09b, 0x09c, 0x09d, 0x09f, 0x0a0, 0x0a1, 0x0a2,
 0x0a4, 0x0a5, 0x0a6, 0x0a8, 0x0a9, 0x0aa, 0x0ab, 0x0ad,
 0x0ae, 0x0af, 0x0b1, 0x0b2, 0x0b3, 0x0b4, 0x0b6, 0x0b7,
 0x0b8, 0x0b9, 0x0bb, 0x0bc, 0x0bd, 0x0bf, 0x0c0, 0x0c1,
 0x0c2, 0x0c4, 0x0c5, 0x0c6, 0x0c7, 0x0c9, 0x0ca, 0x0cb,
 0x0cc, 0x0ce, 0x0cf, 0x0d0, 0x0d1, 0x0d3, 0x0d4, 0x0d5,
 0x0d6, 0x0d8, 0x0d9, 0x0da, 0x0db, 0x0dd, 0x0de, 0x0df,
 0x0e0, 0x0e2, 0x0e3, 0x0e4, 0x0e5, 0x0e7, 0x0e8, 0x0e9,
 0x0ea, 0x0ec, 0x0ed, 0x0ee, 0x0ef, 0x0f0, 0x0f2, 0x0f3,
 0x0f4, 0x0f5, 0x0f7, 0x0f8, 0x0f9, 0x0fa, 0x0fb, 0x0fd,
 0x0fe, 0x0ff, 0x100, 0x102, 0x103, 0x104, 0x105, 0x106,
 0x108, 0x109, 0x10a, 0x10b, 0x10c, 0x10e, 0x10f, 0x110,
 0x111, 0x112, 0x114, 0x115, 0x116, 0x117, 0x118, 0x11a,
 0x11b, 0x11c, 0x11d, 0x11e, 0x120, 0x121, 0x122, 0x123,
 0x124, 0x125, 0x127, 0x128, 0x129, 0x12a, 0x12b, 0x12d,
 0x12e, 0x12f, 0x130, 0x131, 0x132, 0x134, 0x135, 0x136,
 0x137, 0x138, 0x139, 0x13b, 0x13c, 0x13d, 0x13e, 0x13f,
 0x140, 0x142, 0x143, 0x144, 0x145, 0x146, 0x147, 0x148,
 0x14a, 0x14b, 0x14c, 0x14d, 0x14e, 0x14f, 0x151, 0x152,
 0x153, 0x154, 0x155, 0x156, 0x157, 0x159, 0x15a, 0x15b,
 0x15c, 0x15d, 0x15e, 0x15f, 0x161, 0x162, 0x163, 0x164,
 0x165, 0x166, 0x167, 0x168, 0x16a, 0x16b, 0x16c, 0x16d,
 0x16e, 0x16f, 0x170, 0x172, 0x173, 0x174, 0x175, 0x176,
 0x177, 0x178, 0x179, 0x17a, 0x17c, 0x17d, 0x17e, 0x17f,
 0x180, 0x181, 0x182, 0x183, 0x184, 0x186, 0x187, 0x188,
 0x189, 0x18a, 0x18b, 0x18c, 0x18d, 0x18e, 0x190, 0x191,
 0x192, 0x193, 0x194, 0x195, 0x196, 0x197, 0x198, 0x199,
 0x19b, 0x19c, 0x19d, 0x19e, 0x19f, 0x1a0, 0x1a1, 0x1a2,
 0x1a3, 0x1a4, 0x1a5, 0x1a6, 0x1a8, 0x1a9, 0x1aa, 0x1ab,
 0x1ac, 0x1ad, 0x1ae, 0x1af, 0x1b0, 0x1b1, 0x1b2, 0x1b3,
 0x1b4, 0x1b6, 0x1b7, 0x1b8, 0x1b9, 0x1ba, 0x1bb, 0x1bc,
 0x1bd, 0x1be, 0x1bf, 0x1c0, 0x1c1, 0x1c2, 0x1c3, 0x1c5,
 0x1c6, 0x1c7, 0x1c8, 0x1c9, 0x1ca, 0x1cb, 0x1cc, 0x1cd,
 0x1ce, 0x1cf, 0x1d0, 0x1d1, 0x1d2, 0x1d3, 0x1d4, 0x1d5,
 0x1d6, 0x1d8, 0x1d9, 0x1da, 0x1db, 0x1dc, 0x1dd, 0x1de,
 0x1df, 0x1e0, 0x1e1, 0x1e2, 0x1e3, 0x1e4, 0x1e5, 0x1e6,
 0x1e7, 0x1e8, 0x1e9, 0x1ea, 0x1eb, 0x1ec, 0x1ed, 0x1ee,
 0x1ef, 0x1f0, 0x1f1, 0x1f3, 0x1f4, 0x1f5, 0x1f6, 0x1f7,
 0x1f8, 0x1f9, 0x1fa, 0x1fb, 0x1fc, 0x1fd, 0x1fe, 0x1ff,
 0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207,
 0x208, 0x209, 0x20a, 0x20b, 0x20c, 0x20d, 0x20e, 0x20f,
 0x210, 0x211, 0x212, 0x213, 0x214, 0x215, 0x216, 0x217,
 0x218, 0x219, 0x21a, 0x21b, 0x21c, 0x21d, 0x21e, 0x21f,
 0x220, 0x221, 0x222, 0x223, 0x224, 0x225, 0x226, 0x227,
 0x228, 0x229, 0x22a, 0x22b, 0x22c, 0x22d, 0x22e, 0x22f,
 0x230, 0x231, 0x232, 0x233, 0x234, 0x235, 0x236, 0x237,
 0x238, 0x239, 0x23a, 0x23b, 0x23c, 0x23d, 0x23e, 0x23f,
 0x240, 0x241, 0x242, 0x243, 0x244, 0x245, 0x246, 0x247,
 0x248, 0x249, 0x249, 0x24a, 0x24b, 0x24c, 0x24d, 0x24e,
 0x24f, 0x250, 0x251, 0x252, 0x253, 0x254, 0x255, 0x256,
 0x257, 0x258, 0x259, 0x25a, 0x25b, 0x25c, 0x25d, 0x25e,
 0x25f, 0x260, 0x261, 0x262, 0x262, 0x263, 0x264, 0x265,
 0x266, 0x267, 0x268, 0x269, 0x26a, 0x26b, 0x26c, 0x26d,
 0x26e, 0x26f, 0x270, 0x271, 0x272, 0x273, 0x274, 0x275,
 0x275, 0x276, 0x277, 0x278, 0x279, 0x27a, 0x27b, 0x27c,
 0x27d, 0x27e, 0x27f, 0x280, 0x281, 0x282, 0x283, 0x284,
 0x284, 0x285, 0x286, 0x287, 0x288, 0x289, 0x28a, 0x28b,
 0x28c, 0x28d, 0x28e, 0x28f, 0x290, 0x291, 0x291, 0x292,
 0x293, 0x294, 0x295, 0x296, 0x297, 0x298, 0x299, 0x29a,
 0x29b, 0x29c, 0x29d, 0x29d, 0x29e, 0x29f, 0x2a0, 0x2a1,
 0x2a2, 0x2a3, 0x2a4, 0x2a5, 0x2a6, 0x2a7, 0x2a7, 0x2a8,
 0x2a9, 0x2aa, 0x2ab, 0x2ac, 0x2ad, 0x2ae, 0x2af, 0x2b0,
 0x2b1, 0x2b1, 0x2b2, 0x2b3, 0x2b4, 0x2b5, 0x2b6, 0x2b7,
 0x2b8, 0x2b9, 0x2ba, 0x2ba, 0x2bb, 0x2bc, 0x2bd, 0x2be,
 0x2bf, 0x2c0, 0x2c1, 0x2c2, 0x2c3, 0x2c3, 0x2c4, 0x2c5,
 0x2c6, 0x2c7, 0x2c8, 0x2c9, 0x2ca, 0x2cb, 0x2cb, 0x2cc,
 0x2cd, 0x2ce, 0x2cf, 0x2d0, 0x2d1, 0x2d2, 0x2d3, 0x2d3,
 0x2d4, 0x2d5, 0x2d6, 0x2d7, 0x2d8, 0x2d9, 0x2da, 0x2db,
 0x2db, 0x2dc, 0x2dd, 0x2de, 0x2df, 0x2e0, 0x2e1, 0x2e2,
 0x2e2, 0x2e3, 0x2e4, 0x2e5, 0x2e6, 0x2e7, 0x2e8, 0x2e9,
 0x2e9, 0x2ea, 0x2eb, 0x2ec, 0x2ed, 0x2ee, 0x2ef, 0x2ef,
 0x2f0, 0x2f1, 0x2f2, 0x2f3, 0x2f4, 0x2f5, 0x2f6, 0x2f6,
 0x2f7, 0x2f8, 0x2f9, 0x2fa, 0x2fb, 0x2fc, 0x2fc, 0x2fd,
 0x2fe, 0x2ff, 0x300, 0x301, 0x302, 0x302, 0x303, 0x304,
 0x305, 0x306, 0x307, 0x308, 0x308, 0x309, 0x30a, 0x30b,
 0x30c, 0x30d, 0x30e, 0x30e, 0x30f, 0x310, 0x311, 0x312,
 0x313, 0x313, 0x314, 0x315, 0x316, 0x317, 0x318, 0x319,
 0x319, 0x31a, 0x31b, 0x31c, 0x31d, 0x31e, 0x31e, 0x31f,
 0x320, 0x321, 0x322, 0x323, 0x323, 0x324, 0x325, 0x326,
 0x327, 0x328, 0x328, 0x329, 0x32a, 0x32b, 0x32c, 0x32d,
 0x32d, 0x32e, 0x32f, 0x330, 0x331, 0x332, 0x332, 0x333,
 0x334, 0x335, 0x336, 0x337, 0x337, 0x338, 0x339, 0x33a,
 0x33b, 0x33c, 0x33c, 0x33d, 0x33e, 0x33f, 0x340, 0x340,
 0x341, 0x342, 0x343, 0x344, 0x345, 0x345, 0x346, 0x347,
 0x348, 0x349, 0x349, 0x34a, 0x34b, 0x34c, 0x34d, 0x34e,
 0x34e, 0x34f, 0x350, 0x351, 0x352, 0x352, 0x353, 0x354,
 0x355, 0x356, 0x356, 0x357, 0x358, 0x359, 0x35a, 0x35b,
 0x35b, 0x35c, 0x35d, 0x35e, 0x35f, 0x35f, 0x360, 0x361,
 0x362, 0x363, 0x363, 0x364, 0x365, 0x366, 0x367, 0x367,
 0x368, 0x369, 0x36a, 0x36b, 0x36b, 0x36c, 0x36d, 0x36e,
 0x36f, 0x36f, 0x370, 0x371, 0x372, 0x373, 0x373, 0x374,
 0x375, 0x376, 0x377, 0x377, 0x378, 0x379, 0x37a, 0x37a,
 0x37b, 0x37c, 0x37d, 0x37e, 0x37e, 0x37f, 0x380, 0x381,
 0x382, 0x382, 0x383, 0x384, 0x385, 0x385, 0x386, 0x387,
 0x388, 0x389, 0x389, 0x38a, 0x38b, 0x38c, 0x38d, 0x38d,
 0x38e, 0x38f, 0x390, 0x390, 0x391, 0x392, 0x393, 0x394,
 0x394, 0x395, 0x396, 0x397, 0x397, 0x398, 0x399, 0x39a,
 0x39a, 0x39b, 0x39c, 0x39d, 0x39e, 0x39e, 0x39f, 0x3a0,
 0x3a1, 0x3a1, 0x3a2, 0x3a3, 0x3a4, 0x3a4, 0x3a5, 0x3a6,
 0x3a7, 0x3a8, 0x3a8, 0x3a9, 0x3aa, 0x3ab, 0x3ab, 0x3ac,
 0x3ad, 0x3ae, 0x3ae, 0x3af, 0x3b0, 0x3b1, 0x3b1, 0x3b2,
 0x3b3, 0x3b4, 0x3b5, 0x3b5, 0x3b6, 0x3b7, 0x3b8, 0x3b8,
 0x3b9, 0x3ba, 0x3bb, 0x3bb, 0x3bc, 0x3bd, 0x3be, 0x3be,
 0x3bf, 0x3c0, 0x3c1, 0x3c1, 0x3c2, 0x3c3, 0x3c4, 0x3c4,
 0x3c5, 0x3c6, 0x3c7, 0x3c7, 0x3c8, 0x3c9, 0x3ca, 0x3ca,
 0x3cb, 0x3cc, 0x3cd, 0x3cd, 0x3ce, 0x3cf, 0x3d0, 0x3d0,
 0x3d1, 0x3d2, 0x3d3, 0x3d3, 0x3d4, 0x3d5, 0x3d6, 0x3d6,
 0x3d7, 0x3d8, 0x3d9, 0x3d9, 0x3da, 0x3db, 0x3db, 0x3dc,
 0x3dd, 0x3de, 0x3de, 0x3df, 0x3e0, 0x3e1, 0x3e1, 0x3e2,
 0x3e3, 0x3e4, 0x3e4, 0x3e5, 0x3e6, 0x3e7, 0x3e7, 0x3e8,
 0x3e9, 0x3e9, 0x3ea, 0x3eb, 0x3ec, 0x3ec, 0x3ed, 0x3ee,
 0x3ef, 0x3ef, 0x3f0, 0x3f1, 0x3f2, 0x3f2, 0x3f3, 0x3f4,
 0x3f4, 0x3f5, 0x3f6, 0x3f7, 0x3f7, 0x3f8, 0x3f9, 0x3f9,
 0x3fa, 0x3fb, 0x3fc, 0x3fc, 0x3fd, 0x3fe, 0x3ff, 0x3ff,
};

static int int_log2(uint32_t n)
{
    int l, r;
    if (n == 0)
        return 0; /* indefinite */
    l = clz32(n);
    n = (n << l) >> (31 - FRAC_BITS);
    r = log2_table[n - FRAC_ONE] + ((31 - l) << FRAC_BITS);
    return r;
}

#if 0
/* left for reference */
void build_log2_table(void)
{
    int i;
    for(i = 0; i < FRAC_ONE; i++) {
        printf(" 0x%03x,", (int)lrint(log2(1.0 + (double)i / FRAC_ONE) * FRAC_ONE));
        if ((i % 8) == 7)
            printf("\n");
    }
}

void log2_test(void)
{
    double r, ref, err, err_max;
    int i;
    err_max = 0;
    for(i = 1; i < 100000; i++) {
        ref = log2((double)i);
        r = (double)int_log2(i) / FRAC_ONE;
        err = fabs(r - ref);
        if (ref != 0)
            err /= ref;
        if (err > err_max)
            err_max = err;
    }
    printf("err_max=%0.2e\n", err_max);
}
#endif

/* return log2(n/d) * FRAC_ONE */
static int64_t int_log2_frac(uint32_t n, uint32_t d)
{
    return int_log2(n) - int_log2(d);
}

static int64_t get_n_bits(int c, WordList *s,
                          const uint32_t *char_freq, uint32_t tot_freq)
{
    Word *p;
    if (c < NS) {
        return -int_log2_frac(char_freq[c], tot_freq);
    } else {
        p = &s->words[c - NS];
        if (p->freq == 0) {
            /* deleted word */
            return get_n_bits(p->buf[0], s, char_freq, tot_freq) +
                get_n_bits(p->buf[1], s, char_freq, tot_freq);
        } else {
            return -int_log2_frac(p->freq, tot_freq);
        }
    }
}

static int64_t compute_score(const Word *p, WordList *cw,
                           const uint32_t *char_freq, uint32_t tot_freq)
{
    int64_t old_bits, new_bits;

    if (p->freq <= 1)
        return -1; /* not interesting if not repeating */
    if (1) {
        old_bits = (get_n_bits(p->buf[0], cw, char_freq, tot_freq) +
                    get_n_bits(p->buf[1], cw, char_freq, tot_freq)) * p->freq;
        
        new_bits = (-int_log2_frac(p->freq, tot_freq) + TOT_FREQ_RED_BITS) *
            p->freq + SUBST_COST;
        /* return the gain in bits per substitution */
        return old_bits - new_bits;
    } else {
        return p->freq;
    }
}

/* select at most 'n' non overlaping words in ws and add them in
   cw */
int select_best_words(WordList *s, int n, WordList *cw,
                      const uint32_t *char_freq, uint32_t tot_freq,
                      int min_word_freq)
{
    int i, j;
    Word *p;
    uint8_t *cw_bitmap_start, *cw_bitmap_end;
    const DataSymbol *buf;

    for(i = 0; i < s->word_count; i++) {
        p = &s->words[i];
        p->score = compute_score(p, cw, char_freq, tot_freq);
    }
    
    qsort(s->words, s->word_count, sizeof(s->words[0]), word_score_cmp);

#if 0
    {
        printf("%3s %7s %7s %s\n",
               "N", "FREQ", "SCORE", "WORD");
        for(i = 0; i < min_int(s->word_count, 50); i++) {
            p = &s->words[i];
            printf("%3u %7u %7.0f '", i, p->freq, (double)p->score / FRAC_ONE);
            dump_word(stdout, cw, p->buf[0], TRUE);
            dump_word(stdout, cw, p->buf[1], TRUE);
            printf("'\n");
        }
    }
#endif
    
    cw_bitmap_start = mallocz(NS + cw->word_count);
    cw_bitmap_end = mallocz(NS + cw->word_count);
    
    j = 0;
    for(i = 0; i < s->word_count; i++) {
        p = &s->words[i];
        if (p->score <= 0 || p->freq < min_word_freq)
            break;
        /* test if there is a potential overlap with an existing word */
        buf = p->buf;
        if (cw_bitmap_end[buf[0]] ||
            cw_bitmap_start[buf[1]])
            continue;
        cw_bitmap_start[buf[0]] = 1;
        cw_bitmap_end[buf[1]] = 1;
        
        /* add the word */
        word_find_add(cw, buf, 2, TRUE);
#if 0
        printf("%3u %7u '", j, p->freq);
        dump_word(stdout, cw, NS + cw->word_count - 1, TRUE);
        printf("'\n");
#endif
        if (++j >= n)
            break;
    }

    free(cw_bitmap_start);
    free(cw_bitmap_end);
    return j;
}

static void buf_realloc(DataSymbol **pbuf, size_t *pbuf_size, size_t new_size)
{
    if (new_size <= *pbuf_size)
        return;
    new_size = max_size_t(new_size, *pbuf_size + (*pbuf_size) / 8);
    *pbuf = realloc(*pbuf, new_size * sizeof(**pbuf));
    *pbuf_size = new_size;
}

static void out_word(DataSymbol **pbuf, size_t *pbuf_size,
                     size_t *pbuf_pos, WordList *s, uint32_t c)
{
    size_t pos;
    Word *p;
    
    if (c < NS) {
        goto out_char;
    } else {
        p = &s->words[c - NS];
        if (p->freq == 0) {
            out_word(pbuf, pbuf_size, pbuf_pos, s, p->buf[0]);
            out_word(pbuf, pbuf_size, pbuf_pos, s, p->buf[1]);
        } else {
        out_char:
            pos = *pbuf_pos;
            if (pos >= *pbuf_size)
                buf_realloc(pbuf, pbuf_size, pos + 1);
            (*pbuf)[pos++] = c;
            *pbuf_pos = pos;
        }
    }
}

static void compute_word_freq(WordList *s, uint32_t *char_freq,
                              const DataSymbol *buf, size_t buf_size)
{
    int i;
    uint32_t c;
    Word *p;
    
    /* compute the frequency of all the words */
    for(i = 0; i < s->word_count; i++) {
        p = &s->words[i];
        p->freq = 0;
    }
    for(i = 0; i < NS; i++) {
        char_freq[i] = 0;
    }
    for(i = 0; i < buf_size; i++) {
        c = buf[i];
        if (c != CH_CUT) {
            if (c >= NS) {
                p = &s->words[c - NS];
                p->freq++;
            } else {
                char_freq[c]++;
            }
        }
    }
}

/* compute the frequency of the words and remove the ones with a too
   low frequency. Return the word count */
static int update_word_freq(WordList *s, uint32_t *char_freq,
                            DataSymbol **pbuf, size_t *pbuf_size,
                            int min_word_freq)
{
    int i, word_count;
    Word *p;
    DataSymbol *obuf, *buf;
    size_t buf_size, obuf_size, buf_pos;
    
    buf_size = *pbuf_size;
    buf = *pbuf;

    compute_word_freq(s, char_freq, buf, buf_size);
    
    word_count = 0;
    for(i = 0; i < s->word_count; i++) {
        p = &s->words[i];
        if (p->freq >= min_word_freq) {
            word_count++;
        } else {
            p->freq = 0;
        }
    }
    if (word_count == s->word_count)
        return word_count;
    /* remove the words with a too low score from the buffer */
    obuf = malloc(sizeof(obuf[0]) * buf_size);
    obuf_size = buf_size;
    buf_pos = 0;
    for(i = 0; i < buf_size; i++) {
        out_word(&obuf, &obuf_size, &buf_pos, s, buf[i]);
    }
    free(buf);

    /* update the frequencies */
    compute_word_freq(s, char_freq, obuf, buf_pos);
    
    *pbuf = obuf;
    *pbuf_size = buf_pos;
    return word_count;
}

static double compute_entropy(WordList *s, uint32_t *char_freq,
                              size_t buf_size)
{
    int64_t n_bits;
    size_t i;
    Word *p;
    
    n_bits = 0;
    for(i = 0; i < NS; i++) {
        if (char_freq[i] != 0)
            n_bits += -int_log2_frac(char_freq[i], buf_size) * char_freq[i];
    }
    for(i = 0; i < s->word_count; i++) {
        p = &s->words[i];
        if (p->freq > 0) {
            n_bits += -int_log2_frac(p->freq, buf_size) * p->freq;
        }
    }
    return (double)n_bits / FRAC_ONE;
}

    
/* CH_WORD_START: add a space except if the last char is '[' or '(' */

static int is_word_char(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= 128);
}

static int is_upper(int c)
{
    return (c >= 'A' && c <= 'Z');
}

static int is_lower(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 128);
}

/*
  syntax:
  CH_NO_SPACE?[CH_TO_UPPER|CH_FIRST_UPPER]?SPACE word or not-word

  CH_ESCAPE is used in case one CH_x code is present in the input stream.
*/

DataSymbol *case_space_encoding(size_t *pobuf_len, DataSymbol *buf, size_t buf_len)
{
    size_t i, j, len, k, l, obuf_size;
    DataSymbol *obuf;
    int ch_type, c;
    BOOL has_space;
    
    obuf = malloc(sizeof(buf[0]) * buf_len);
    obuf_size = buf_len;
    k = 0;
    for(i = 0; i < buf_len;) {
        if (is_word_char(buf[i])) {
            j = i + 1;
            if (is_lower(buf[i])) {
                while (j < buf_len && is_lower(buf[j]))
                    j++;
                ch_type = 0;
            } else if (j < buf_len && is_upper(buf[j])) {
                while (j < buf_len && is_upper(buf[j]))
                    j++;
                ch_type = CH_TO_UPPER;
            } else {
                while (j < buf_len && is_lower(buf[j]))
                    j++;
                ch_type = CH_FIRST_UPPER;
            }
            len = j - i;

            if (k > 0 && obuf[k - 1] == ' ') {
                has_space = TRUE;
                k--;
            } else {
                has_space = FALSE;
            }
            buf_realloc(&obuf, &obuf_size, k + len + 3);
#ifdef USE_CUT
            obuf[k++] = CH_CUT;
#endif
            if (!has_space) {
                obuf[k++] = CH_NO_SPACE;
            }
            if (ch_type != 0)
                obuf[k++] = ch_type;
            obuf[k++] = ' ';
            for(l = 0; l < len; l++) {
                c = buf[i + l];
                if (c >= 'A' && c <= 'Z')
                    c = c - 'A' + 'a';
                obuf[k++] = c;
            }
            i += len;
        } else if (buf[i] == CH_NO_SPACE ||
                   buf[i] == CH_TO_UPPER ||
                   buf[i] == CH_FIRST_UPPER ||
                   buf[i] == CH_ESCAPE) {
            /* escape the reserved codes */
            buf_realloc(&obuf, &obuf_size, k + 2);
            obuf[k++] = CH_ESCAPE;
            obuf[k++] = buf[i++];
        } else {
            buf_realloc(&obuf, &obuf_size, k + 1);
            obuf[k++] = buf[i++];
        }
    }
    obuf = realloc(obuf, sizeof(obuf[0]) * k);
    *pobuf_len = k;
    return obuf;
}

typedef struct {
    BOOL has_space;
    int ch_type;
    int ch_type1;
    BOOL has_escape;
} CaseSpaceDecodeState;

static void case_space_decode_init(CaseSpaceDecodeState *s)
{
    s->ch_type = 0;
    s->has_space = TRUE;
    s->ch_type1 = 0;
    s->has_escape = FALSE;
}

/* return the character to output or -1 if none */
static int case_space_decode(CaseSpaceDecodeState *s, int c)
{
    if (s->has_escape) {
        s->has_escape = FALSE;
    } else if (c == CH_TO_UPPER || c == CH_FIRST_UPPER) {
        s->ch_type = c;
        c = -1;
    } else if (c == CH_NO_SPACE) {
        s->has_space = FALSE;
        c = -1;
    } else if (c == CH_ESCAPE) {
        s->has_escape = TRUE;
        c = -1;
    } else if (c == ' ') {
        s->ch_type1 = s->ch_type;
        s->ch_type = 0;
        if (!s->has_space) {
            c = -1;
        }
        s->has_space = TRUE;
    } else {
        if (s->ch_type1 == CH_TO_UPPER || s->ch_type1 == CH_FIRST_UPPER) {
            if (c >= 'a' && c <= 'z')
                c = c - 'a' + 'A';
            if (s->ch_type1 == CH_FIRST_UPPER)
                s->ch_type1 = 0;
        }
        s->has_space = TRUE;
    }
    return c;
}

/* return the total number of words */
int word_encode(const char *in_filename, const char *out_filename,
                const char *word_filename, int n_words, int min_word_freq,
                const char *debug_dict_filename, BOOL sort_by_freq,
                BOOL verbose)
{
    FILE *f;
    size_t buf_size, i, j, buf_size1;
    DataSymbol *buf, *buf1;
    uint32_t *char_freq, *tab_sort;
    WordList *ws, *s;
    Word *p;
    int n, word_count, word_count_prev;
    double n_bits;
    
    f = fopen(in_filename, "rb");
    if (!f) {
        perror(in_filename);
        exit(1);
    }
    
    fseek(f, 0, SEEK_END);
    buf_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(buf_size * sizeof(buf[0]));
    for(i = 0; i < buf_size; i++) {
        buf[i] = fgetc(f);
    }
    fclose(f);

    buf1 = case_space_encoding(&buf_size1, buf, buf_size);
    free(buf);
    buf = buf1;
    buf_size = buf_size1;
        
    if (verbose) {
        printf("Preprocessing: after case/space preprocessing: %d symbols\n",
               (int)buf_size);
    }
    
    s = word_list_init();

    char_freq = mallocz(sizeof(char_freq[0]) * NS);

    compute_word_freq(s, char_freq, buf, buf_size);

    n_words -= NS;

    if (verbose) {
        printf("%6s %9s %9s\n",
               "#WORDS", "SIZE", "ENTROPY");
    }

    for(word_count = 0; word_count < n_words;) {
        if (buf_size < 2)
            break;
        ws = word_list_init();
        hash_lookup_count = 0;
        hash_it_count = 0;
        for(i = 0; i < buf_size - 1; i++) {
            /* favorise words with space before */
            if (buf[i] != CH_CUT && buf[i + 1] != CH_CUT) {
                p = word_find_add(ws, buf + i, 2, TRUE);
                p->freq++;
            }
        }
#if 0
        printf("hash stats: %d %" PRId64 "d avg=%0.1f\n",
               (int)ws->word_count,
               hash_lookup_count,
               (double)hash_it_count / hash_lookup_count);
#endif
        n = select_best_words(ws, min_int(MAX_WORDS_PER_ITER,
                                          n_words - word_count),
                              s, char_freq, buf_size, min_word_freq);
        word_list_end(ws);
        if (n == 0)
            break;
        
        /* replace with the new words */
        j = 0;
        for(i = 0; i < buf_size;) {
            if ((i + 1) >= buf_size)
                goto no_subst;
            p = word_find_add(s, buf + i, 2, FALSE);
            if (p) {
                buf[j++] = NS + (p - s->words);
                i += 2;
            } else {
            no_subst:
                buf[j++] = buf[i++];
            }
        }
        buf_size = j;

        /* compute the frequency of all the words and remove the words
           with a too low frequency */
        word_count_prev = word_count;
        word_count = update_word_freq(s, char_freq, &buf, &buf_size,
                                      min_word_freq);

        /* mesure the entropy */
        n_bits = compute_entropy(s, char_freq, buf_size);
        
        if (verbose) {
            printf("%6u %9u %9.0f\r", word_count + NS, (int)buf_size, n_bits / 8);
            fflush(stdout);
        }

        if (word_count >= n_words ||
            word_count == word_count_prev)
            break;
    }

    if (verbose) {
        printf("%6u %9u\n", word_count + NS, (int)buf_size);
    }

    word_count = sort_words(s, &tab_sort, char_freq, sort_by_freq);
    
    save_words(s, word_filename, tab_sort, word_count);
    if (debug_dict_filename) {
        save_words_debug(s, debug_dict_filename, char_freq, buf_size,
                         tab_sort, word_count);
    }
    
    save_output(buf, buf_size, s, out_filename, tab_sort, word_count);
    free(tab_sort);
    free(buf);
    free(char_freq);
    
    word_list_end(s);
    return word_count;
}

void word_load(StringTable *s, const char *filename)
{
    FILE *f;
    uint8_t buf[4096];
    int len, c;
    
    f = fopen(filename, "rb");
    if (!f) {
        perror(filename);
        exit(1);
    }
    len = 0;
    for(;;) {
        c = fgetc(f);
        if (c < 0)
            break;
        if (c == '\n') {
            if (len > 0) {
                string_table_add(s, (const char *)buf, len);
            }
            len = 0;
        } else {
            if (c == '\\') {
                c = fgetc(f);
                if (c < 0)
                    break;
                if (c == 'n') {
                    c = '\n';
                } else if (c != '\\') {
                    fprintf(stderr, "Invalid escape\n");
                    exit(1);
                }
            }
            if (len >= sizeof(buf)) {
                fprintf(stderr, "Word too long\n");
                exit(1);
            }
            buf[len++] = c;
        }
    }
    fclose(f);
}

void word_decode(const char *in_filename, const char *out_filename,
                 const char *word_filename)
{
    StringTable *s;
    FILE *f, *fo;
    uint16_t c;
    int i, len, b;
    StringEntry *p;
    const uint8_t *buf;
    CaseSpaceDecodeState cs;
    
    s = string_table_init();
    word_load(s, word_filename);

    //    printf("%d words\n", (int)s->count);
    
    f = fopen(in_filename, "rb");
    if (!f) {
        perror(in_filename);
        exit(1);
    }
    
    fo = fopen(out_filename, "wb");
    if (!fo) {
        perror(out_filename);
        exit(1);
    }

    case_space_decode_init(&cs);
    for(;;) {
        if (fget_be16(f, &c))
            break;
        if (c >= s->count) {
            fprintf(stderr, "Invalid symbol %d\n", c);
            exit(1);
        }
        p = s->tab[c];
        buf = (uint8_t *)p->data;
        len = p->len;
        for(i = 0; i < len; i++) {
            b = case_space_decode(&cs, buf[i]);
            if (b >= 0)
                fputc(b, fo);
        }
    }

    fclose(fo);

    fclose(f);
    
    string_table_end(s);
}

#ifdef CONFIG_STANDALONE

void help(void)
{
    printf("Preprocess version " CONFIG_VERSION", Copyright (c) 2018-2021 Fabrice Bellard\n"
           "Dictionary based preprocessor\n"
           "usage: preprocess [options] c dictfile infile outfile n_words min_freq\n"
           "       preprocess [options] d dictfile infile outfile\n"
           "\n"
           "'c' command: build the dictionary 'dictfile' from 'infile' and output the preprocessed data to 'outfile'. 'n_words' is the approximative maximum number of words of the dictionary. 'min_freq' is the minimum frequency of the selected words.\n"
           "'d' command: rebuild the original file from the dictionary and the preprocessed data.\n"
           "\n"
           "Options:\n"
           "-h             this help\n"
           "-D filename    output debug information associated with the dictionary\n"
           "-s             sort the words by decreasing frequency\n"

);
    exit(1);
}

int main(int argc, char **argv)
{
    const char *in_filename, *out_filename, *mode, *dict_filename;
    const char *debug_dict_filename;
    BOOL sort_by_freq;
    int c;

    debug_dict_filename = NULL;
    sort_by_freq = FALSE;
    for(;;) {
        c = getopt(argc, argv, "hD:s");
        if (c == -1)
            break;
        switch(c) {
        case 'h':
            help();
            break;
        case 'D':
            debug_dict_filename = optarg;
            break;
        case 's':
            sort_by_freq = TRUE;
            break;
        default:
            exit(1);
        }
    }

    if ((argc - optind) < 1)
        help();

    mode = argv[optind++];

    if (mode[0] == 'c') {
        int n_words, min_word_freq;
        if ((argc - optind) != 5)
            help();
        dict_filename = argv[optind++];
        in_filename = argv[optind++];
        out_filename = argv[optind++];
        n_words = atoi(argv[optind++]);
        min_word_freq = atoi(argv[optind++]);
        word_encode(in_filename, out_filename, dict_filename, n_words,
                    min_word_freq, debug_dict_filename, sort_by_freq, TRUE);
    } else if (mode[0] == 'd') {
        if ((argc - optind) != 3)
            help();
        dict_filename = argv[optind++];
        in_filename = argv[optind++];
        out_filename = argv[optind++];
        word_decode(in_filename, out_filename, dict_filename);
    } else {
        help();
    }
    return 0;
}

#endif
