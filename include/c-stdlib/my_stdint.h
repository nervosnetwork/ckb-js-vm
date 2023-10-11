#ifndef _STDINT_H
#define _STDINT_H 1

#define INTPTR_MIN (-9223372036854775807L - 1)
#define INTPTR_MAX (9223372036854775807L)
#define UINTPTR_MAX (18446744073709551615UL)

#define INT8_C(c) c
#define INT16_C(c) c
#define INT32_C(c) c

#define UINT8_C(c) c
#define UINT16_C(c) c
#define UINT32_C(c) c##U

#if UINTPTR_MAX == UINT64_MAX
#define INT64_C(c) c##L
#define UINT64_C(c) c##UL
#define INTMAX_C(c) c##L
#define UINTMAX_C(c) c##UL
#else
#define INT64_C(c) c##LL
#define UINT64_C(c) c##ULL
#define INTMAX_C(c) c##LL
#define UINTMAX_C(c) c##ULL
#endif

#endif
