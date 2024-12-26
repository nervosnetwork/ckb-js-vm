///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2014-2019, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief Tiny printf, sprintf and (v)snprintf implementation, optimized for
// speed on
//        embedded systems with a very limited resources. These routines are
//        thread safe and reentrant! Use this instead of the bloated
//        standard/newlib printf cause these use malloc for printf (and may not
//        be thread safe).
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CKB_C_STDLIB_STDIO_H_
#define CKB_C_STDLIB_STDIO_H_
#include <entry.h>

/*
 * This function uses `ckb_debug` syscall to output formatted messages.
 *
 * Pass `-D CKB_C_STDLIB_PRINTF` flag to GCC to enable printf;
 * If the flag is undefined the printf will be compiled as an empty function.
 *
 * Some versions of GCC raise errors on compiling since those versions have a
 * built-in printf function; pass `-fno-builtin-printf` flag to GCC to fix the
 * compiling.
 */
int printf(const char *format, ...);
/*
 * This function uses `ckb_debug` syscall to output formatted messages.
 *
 * Pass `-D CKB_C_STDLIB_PRINTF` flag to GCC to enable ckb_printf;
 * If the flag is undefined the ckb_printf will be compiled as an empty
 * function.
 */
int ckb_printf(const char *format, ...);
int ckb_debug(const char *s);

#include <stddef.h>
#include <stdint.h>
#include "ckb_cell_fs.h"

#define BUFSIZ 512
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

typedef struct FILE {
    FSFile *file;
    uint32_t offset;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int remove(const char *__filename);

int rename(const char *__old, const char *__new);

FILE *tmpfile(void);

char *tmpnam(char *__s);

char *tempnam(const char *__dir, const char *__pfx);

int fclose(FILE *__stream);
int fflush(FILE *__stream);

FILE *fopen(const char *__filename, const char *__modes);
FILE *freopen(const char *__filename, const char *__modes, FILE *__stream);

void setbuf(FILE *__stream, char *__buf);
int setvbuf(FILE *__stream, char *__buf, int __modes, size_t __n);

int fprintf(FILE *__stream, const char *__format, ...);

int sprintf(char *__s, const char *__format, ...);

int vfprintf(FILE *__s, const char *__format, ...);

int vsprintf(char *__s, const char *__format, ...);
int vsnprintf(char *__s, size_t __maxlen, const char *__format, ...);
int snprintf(char *__s, size_t __maxlen, const char *__format, ...);
int snprintf_(char *__s, size_t __maxlen, const char *__format, ...);

int fscanf(FILE *__stream, const char *__format, ...);

int scanf(const char *__format, ...);

int sscanf(const char *__s, const char *__format, ...);

int fgetc(FILE *__stream);

int getc(FILE *__stream);

int getchar(void);

int fputc(int __c, FILE *__stream);

int putc(int __c, FILE *__stream);

int putchar(int __c);

char *fgets(char *__s, int __n, FILE *__stream);
char *gets(char *__s);

int getline(char **__lineptr, size_t *__n, FILE *__stream);

int fputs(const char *__s, FILE *__stream);

int puts(const char *__s);

int ungetc(int __c, FILE *__stream);

size_t fread(void *__ptr, size_t __size, size_t __n, FILE *__stream);

size_t fwrite(const void *__ptr, size_t __size, size_t __n, FILE *__s);

int fseek(FILE *__stream, long int __off, int __whence);

long int ftell(FILE *__stream);

void rewind(FILE *__stream);

void clearerr(FILE *__stream);

int feof(FILE *__stream);

int ferror(FILE *__stream);

void perror(const char *__s);

int fileno(FILE *__stream);

FILE *popen(const char *__command, const char *__modes);

int pclose(FILE *__stream);

void enable_local_access(int);

#endif /* CKB_C_STDLIB_STDIO_H_ */
