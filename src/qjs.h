#ifndef __QJS_H__
#define __QJS_H__

// Define meaningful enum names for exit codes
typedef enum {
    QJS_SUCCESS = 0,
    QJS_ERROR_GENERIC = -1,
    QJS_ERROR_MEMORY_ALLOCATION = -2,
    QJS_ERROR_FILE_TOO_LARGE = -3,
    QJS_ERROR_FILE_READ = -4,
    QJS_ERROR_INVALID_ARGUMENT = -5,
    QJS_ERROR_INTERNAL = -6,
    QJS_ERROR_EXCEPTION = -7,
    QJS_ERROR_EMPTY_FILE = -8,
    QJS_ERROR_INVALID_SCRIPT = -9,
    QJS_ERROR_INVALID_SYSCALL_ARGUMENT = -10,
    QJS_ERROR_MOUNT = -11,
    QJS_ERROR_INVALID_SCRIPT_ARGS = -12,
    QJS_ERROR_EVAL = -13,
    QJS_ERROR_FS = -14,
} QJSErrorCode;

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
        if (code != QJS_SUCCESS) {                                                   \
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

#endif  //__QJS_H__