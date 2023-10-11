
#ifndef _STDIO_H
#define _STDIO_H 1

#include <stddef.h>
#include <stdint.h>
#include <my_stdint.h>

#include "../../include/ckb_cell_fs.h"

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

int printf(const char *format, ...);

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

#endif /* <stdio.h> included.  */
