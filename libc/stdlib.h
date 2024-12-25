#ifndef CKB_C_STDLIB_STDLIB_H_
#define CKB_C_STDLIB_STDLIB_H_
#include <entry.h>
#include <internal/types.h>

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
typedef int (*cmpfun)(const void *, const void *);
void qsort(void *base, size_t nel, size_t width, cmpfun cmp);
int rand(void);
void *bsearch(const void *key, const void *base, size_t nel, size_t width,
              int (*cmp)(const void *, const void *));

float strtof(const char *__restrict, char **__restrict);
double strtod(const char *__restrict, char **__restrict);
long double strtold(const char *__restrict, char **__restrict);
int atoi(const char *);

int abs(int);
void exit(int);
void abort(void);
#define alloca __builtin_alloca

#endif /* CKB_C_STDLIB_STDLIB_H_ */
