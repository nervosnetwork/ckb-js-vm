#include "base64.h"
#include <string.h>
#include <stdlib.h>

// modified from https://github.com/curl/curl/blob/daa0601614b6cfd44a3437a2e5b0c6ca868745f2/lib/base64.c

static const char base64encdec[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static const unsigned char decodetable[] = {62,  255, 255, 255, 63,  52,  53, 54, 55, 56, 57, 58, 59, 60, 61, 255,
                                            255, 255, 255, 255, 255, 255, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                                            10,  11,  12,  13,  14,  15,  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
                                            255, 255, 255, 255, 255, 255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
                                            36,  37,  38,  39,  40,  41,  42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
int qjs_base64_decode(const char *src, unsigned char **outptr, size_t *outlen) {
    size_t srclen = 0;
    size_t padding = 0;
    size_t i;
    size_t numQuantums;
    size_t fullQuantums;
    size_t rawlen = 0;
    unsigned char *pos;
    unsigned char *newstr;
    unsigned char lookup[256];

    *outptr = NULL;
    *outlen = 0;
    srclen = strlen(src);

    /* Check the length of the input string is valid */
    if (!srclen || srclen % 4) return -1;

    /* srclen is at least 4 here */
    while (src[srclen - 1 - padding] == '=') {
        /* count padding characters */
        padding++;
        /* A maximum of two = padding characters is allowed */
        if (padding > 2) return -1;
    }

    /* Calculate the number of quantums */
    numQuantums = srclen / 4;
    fullQuantums = numQuantums - (padding ? 1 : 0);

    /* Calculate the size of the decoded string */
    rawlen = (numQuantums * 3) - padding;

    /* Allocate our buffer including room for a null-terminator */
    newstr = malloc(rawlen + 1);
    if (!newstr) return -1;

    pos = newstr;

    memset(lookup, 0xff, sizeof(lookup));
    memcpy(&lookup['+'], decodetable, sizeof(decodetable));
    /* Decode the complete quantums first */
    for (i = 0; i < fullQuantums; i++) {
        unsigned char val;
        unsigned int x = 0;
        int j;

        for (j = 0; j < 4; j++) {
            val = lookup[(unsigned char)*src++];
            if (val == 0xff) /* bad symbol */
                goto bad;
            x = (x << 6) | val;
        }
        pos[2] = x & 0xff;
        pos[1] = (x >> 8) & 0xff;
        pos[0] = (x >> 16) & 0xff;
        pos += 3;
    }
    if (padding) {
        /* this means either 8 or 16 bits output */
        unsigned char val;
        unsigned int x = 0;
        int j;
        size_t padc = 0;
        for (j = 0; j < 4; j++) {
            if (*src == '=') {
                x <<= 6;
                src++;
                if (++padc > padding) /* this is a badly placed '=' symbol! */
                    goto bad;
            } else {
                val = lookup[(unsigned char)*src++];
                if (val == 0xff) /* bad symbol */
                    goto bad;
                x = (x << 6) | val;
            }
        }
        if (padding == 1) pos[1] = (x >> 8) & 0xff;
        pos[0] = (x >> 16) & 0xff;
        pos += 3 - padding;
    }

    /* Zero terminate */
    *pos = '\0';

    /* Return the decoded data */
    *outptr = newstr;
    *outlen = rawlen;

    return 0;
bad:
    free(newstr);
    return -1;
}

static int base64_encode(const char *table64, const char *inputbuff, size_t insize, char **outptr, size_t *outlen) {
    char *output;
    char *base64data;
    const unsigned char *in = (const unsigned char *)inputbuff;
    const char *padstr = &table64[64]; /* Point to padding string. */

    *outptr = NULL;
    *outlen = 0;

    if (!insize) insize = strlen(inputbuff);

    base64data = output = malloc((insize + 2) / 3 * 4 + 1);
    if (!output) return -1;

    while (insize >= 3) {
        *output++ = table64[in[0] >> 2];
        *output++ = table64[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *output++ = table64[((in[1] & 0x0F) << 2) | ((in[2] & 0xC0) >> 6)];
        *output++ = table64[in[2] & 0x3F];
        insize -= 3;
        in += 3;
    }
    if (insize) {
        /* this is only one or two bytes now */
        *output++ = table64[in[0] >> 2];
        if (insize == 1) {
            *output++ = table64[((in[0] & 0x03) << 4)];
            if (*padstr) {
                *output++ = *padstr;
                *output++ = *padstr;
            }
        } else {
            /* insize == 2 */
            *output++ = table64[((in[0] & 0x03) << 4) | ((in[1] & 0xF0) >> 4)];
            *output++ = table64[((in[1] & 0x0F) << 2)];
            if (*padstr) *output++ = *padstr;
        }
    }

    /* Zero terminate */
    *output = '\0';

    /* Return the pointer to the new data (allocated memory) */
    *outptr = base64data;

    /* Return the length of the new data */
    *outlen = (size_t)(output - base64data);

    return 0;
}

int qjs_base64_encode(const char *inputbuff, size_t insize, char **outptr, size_t *outlen) {
    return base64_encode(base64encdec, inputbuff, insize, outptr, outlen);
}
