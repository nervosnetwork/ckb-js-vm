#ifndef CKB_C_STDLIB_SETJMP_H_
#define CKB_C_STDLIB_SETJMP_H_

#ifdef __cplusplus
extern "C" {
#endif

// #include <features.h>

typedef unsigned long __jmp_buf[26];

typedef struct __jmp_buf_tag {
    __jmp_buf __jb;
    unsigned long __fl;
    unsigned long __ss[128 / sizeof(long)];
} jmp_buf[1];

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
#define __setjmp_attr __attribute__((__returns_twice__))
#else
#define __setjmp_attr
#endif

#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || \
    defined(_BSD_SOURCE)
typedef jmp_buf sigjmp_buf;
int sigsetjmp(sigjmp_buf, int) __setjmp_attr;
_Noreturn void siglongjmp(sigjmp_buf, int);
#endif

int _setjmp(jmp_buf) __setjmp_attr;

_Noreturn void _longjmp(jmp_buf, int);

int setjmp(jmp_buf) __setjmp_attr;
_Noreturn void longjmp(jmp_buf, int);

#define setjmp setjmp

#undef __setjmp_attr

#ifdef __cplusplus
}
#endif

#endif
