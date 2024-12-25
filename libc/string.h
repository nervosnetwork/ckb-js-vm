#ifndef CKB_C_STDLIB_STRING_H_
#define CKB_C_STDLIB_STRING_H_

#include <entry.h>
#include <internal/types.h>

void *memset(void *dest, int c, size_t n);
void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *vl, const void *vr, size_t n);
char *strcpy(char *restrict d, const char *restrict s);
size_t strlen(const char *s);
int strcmp(const char *l, const char *r);
char *strstr(const char *, const char *);

char *strchr(const char *, int);
int strncmp(const char *_l, const char *_r, size_t n);
char *strpbrk(const char *, const char *);
size_t strcspn(const char *, const char *);
size_t strspn(const char *, const char *);
void *memchr(const void *, int, size_t);
char *strrchr(const char *str, int character);
char *strcat(char *destination, const char *source);
int strcoll(const char *, const char *);
char *strerror(int);

#endif /* CKB_C_STDLIB_STRING_H_ */
