#ifndef __BASE64_H__
#define __BASE64_H__

#include <stddef.h>
int qjs_base64_encode(const char *inputbuff, size_t insize, char **outptr, size_t *outlen);
int qjs_base64_decode(const char *src, unsigned char **outptr, size_t *outlen);

#endif
