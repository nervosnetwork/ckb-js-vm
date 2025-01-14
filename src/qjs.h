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
    QJS_ERROR_SYSCALL = -6,
    QJS_ERROR_INTERNAL = -7,
    QJS_ERROR_MODULE_LOAD = -8,
    QJS_ERROR_SCRIPT_EXECUTION = -9,
    QJS_ERROR_PROMISE_REJECTED = -10,
    QJS_ERROR_INVALID_SIGNATURE = -11,
    QJS_ERROR_INVALID_PUBLIC_KEY = -12,
    QJS_ERROR_INVALID_HEX_STRING = -13,
    QJS_ERROR_INVALID_BASE64_STRING = -14,
    QJS_ERROR_INVALID_JSON = -15,
    QJS_ERROR_INVALID_PROMISE_STATE = -16,
    QJS_ERROR_INVALID_RECOVERY_ID = -17,
    QJS_ERROR_INVALID_MESSAGE_LENGTH = -18,
    QJS_ERROR_INVALID_SIGNATURE_FORMAT = -19,
    QJS_ERROR_INVALID_PUBLIC_KEY_FORMAT = -20,
    QJS_ERROR_INVALID_MODULE_NAME = -21,
    QJS_ERROR_INVALID_SCRIPT = -22,
    QJS_ERROR_INVALID_SYSCALL_ARGUMENT = -23,
    QJS_ERROR_INVALID_SYSCALL_MEMORY = -24,
    QJS_ERROR_INVALID_SYSCALL_UNKNOWN = -25,
    QJS_ERROR_MOUNT = -26,
    QJS_ERROR_INVALID_SCRIPT_ARGS = -27,
    QJS_ERROR_EVAL = -28,
    QJS_ERROR_EXCEPTION = -29,
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