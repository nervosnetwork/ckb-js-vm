#define CHECK2(cond, code)                                                           \
    do {                                                                             \
        if (!(cond)) {                                                               \
            err = code;                                                              \
            printf("checking failed on %s:%d, code = %d", __FILE__, __LINE__, code); \
            goto exit;                                                               \
        }                                                                            \
    } while (0)

#define CHECK(_code)                                                                 \
    do {                                                                             \
        int code = (_code);                                                          \
        if (code != 0) {                                                             \
            err = code;                                                              \
            printf("checking failed on %s:%d, code = %d", __FILE__, __LINE__, code); \
            goto exit;                                                               \
        }                                                                            \
    } while (0)

#ifdef CONFIG_BIGNUM
#define BC_BASE_VERSION 2
#else
#define BC_BASE_VERSION 1
#endif
#define BC_BE_VERSION 0x40
#ifdef WORDS_BIGENDIAN
#define BC_VERSION (BC_BASE_VERSION | BC_BE_VERSION)
#else
#define BC_VERSION BC_BASE_VERSION
#endif
