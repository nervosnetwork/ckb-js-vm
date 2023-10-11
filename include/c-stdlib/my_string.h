#ifndef C_STDLIB_STRING_H_
#define C_STDLIB_STRING_H_

#include <stddef.h>

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

#endif /* C_STDLIB_STRING_H_ */
