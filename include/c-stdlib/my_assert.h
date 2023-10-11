#ifndef C_STDLIB_ASSERT_H_
#define C_STDLIB_ASSERT_H_

int ckb_exit(signed char);

#define assert(s)                                                      \
    do {                                                               \
        if (!(s)) {                                                    \
            printf("Failed at %s:%d: %s\n", __FILE__, __LINE__, (#s)); \
            ckb_exit(-1);                                              \
        }                                                              \
    } while (0)

#endif  // C_STDLIB_ASSERT_H_
