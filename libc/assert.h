#ifndef CKB_C_STDLIB_ASSERT_H_
#define CKB_C_STDLIB_ASSERT_H_

#include "ckb_syscall_apis.h"

#define assert(s)                                                      \
    do {                                                               \
        if (!(s)) {                                                    \
            printf("Failed at %s:%d: %s\n", __FILE__, __LINE__, (#s)); \
            ckb_exit(-1);                                              \
        }                                                              \
    } while (0)

#endif  // CKB_C_STDLIB_ASSERT_H_
