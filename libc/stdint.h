#ifndef CKB_C_STDLIB_STDINT_H_
#define CKB_C_STDLIB_STDINT_H_

#include <entry.h>

#include "internal/types.h"

#define INT8_MIN (-1 - 0x7f)
#define INT16_MIN (-1 - 0x7fff)
#define INT32_MIN (-1 - 0x7fffffff)
#define INT64_MIN (-1 - 0x7fffffffffffffff)

#define INT8_MAX (0x7f)
#define INT16_MAX (0x7fff)
#define INT32_MAX (0x7fffffff)
#define INT64_MAX (0x7fffffffffffffff)

#define UINT8_MAX (0xff)
#define UINT16_MAX (0xffff)
#define UINT32_MAX (0xffffffffu)
#define UINT64_MAX (0xffffffffffffffffu)

#define SIZE_MAX UINT64_MAX

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

#endif /* CKB_C_STDLIB_STDINT_H_ */
