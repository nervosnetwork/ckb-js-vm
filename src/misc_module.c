#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cutils.h"
#include "quickjs.h"
#include "misc_module.h"
#define BLAKE2_IMPL_H
#define BLAKE2_REF_C
#include "blake2b.h"
#include "ckb_smt.h"
#include "qjs.h"
#include "utils.h"
#include "base64.h"

static JSClassID js_smt_class_id;
static JSClassID js_text_decoder_class_id;
static JSClassID js_text_encoder_class_id;

typedef struct {
    uint8_t key[SMT_KEY_BYTES];
    uint8_t value[SMT_VALUE_BYTES];
} KeyValuePair;

typedef struct {
    KeyValuePair *pairs;
    size_t len;
    size_t capacity;
} SMTWrapper;

// Constructor for Smt class
static JSValue js_smt_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
    JSValue obj = JS_NewObjectClass(ctx, js_smt_class_id);
    if (JS_IsException(obj)) return obj;

    // Initialize with empty dynamic array
    SMTWrapper *wrapper = js_mallocz(ctx, sizeof(SMTWrapper));
    if (!wrapper) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
    }

    wrapper->pairs = NULL;
    wrapper->len = 0;
    wrapper->capacity = 0;

    JS_SetOpaque(obj, wrapper);
    return obj;
}

// Insert method for Smt class
static JSValue js_smt_insert(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    SMTWrapper *w = JS_GetOpaque2(ctx, this_val, js_smt_class_id);
    if (!w) return JS_EXCEPTION;

    size_t key_len, value_len;
    uint8_t *key = JS_GetArrayBuffer(ctx, &key_len, argv[0]);
    uint8_t *value = JS_GetArrayBuffer(ctx, &value_len, argv[1]);

    if (!key || !value || key_len != SMT_KEY_BYTES || value_len != SMT_VALUE_BYTES)
        return JS_ThrowTypeError(ctx, "Invalid key or value format");

    // Grow array if needed
    if (w->len >= w->capacity) {
        size_t new_capacity = w->capacity == 0 ? 16 : w->capacity * 2;
        KeyValuePair *new_pairs = js_realloc(ctx, w->pairs, new_capacity * sizeof(KeyValuePair));
        if (!new_pairs) return JS_ThrowOutOfMemory(ctx);

        w->pairs = new_pairs;
        w->capacity = new_capacity;
    }

    // Store new pair
    _smt_fast_memcpy(w->pairs[w->len].key, key, SMT_KEY_BYTES);
    _smt_fast_memcpy(w->pairs[w->len].value, value, SMT_VALUE_BYTES);
    w->len++;

    return JS_UNDEFINED;
}

// Verify method for Smt class
static JSValue js_smt_verify(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    SMTWrapper *w = JS_GetOpaque2(ctx, this_val, js_smt_class_id);
    if (!w) return JS_EXCEPTION;

    size_t root_len, proof_len;
    uint8_t *root = JS_GetArrayBuffer(ctx, &root_len, argv[0]);
    uint8_t *proof = JS_GetArrayBuffer(ctx, &proof_len, argv[1]);

    if (!root || root_len != SMT_VALUE_BYTES || !proof) return JS_ThrowTypeError(ctx, "Invalid root or proof format");

    // Create temporary smt_state with collected pairs
    smt_pair_t *temp_pairs = js_mallocz(ctx, w->len * sizeof(smt_pair_t));
    if (!temp_pairs) return JS_ThrowOutOfMemory(ctx);

    smt_state_t state;
    smt_state_init(&state, temp_pairs, w->len);

    // Insert all collected pairs
    for (size_t i = 0; i < w->len; i++) {
        int ret = smt_state_insert(&state, w->pairs[i].key, w->pairs[i].value);
        if (ret != 0) {
            js_free(ctx, temp_pairs);
            return JS_ThrowRangeError(ctx, "SMT insertion failed");
        }
    }
    smt_state_normalize(&state);
    // Verify the proof
    int ret = smt_verify(root, &state, proof, proof_len);

    js_free(ctx, temp_pairs);
    return JS_NewBool(ctx, ret == 0);
}

// Finalizer for Smt class
static void js_smt_finalizer(JSRuntime *rt, JSValue val) {
    SMTWrapper *w = JS_GetOpaque(val, js_smt_class_id);
    if (w) {
        if (w->pairs) js_free_rt(rt, w->pairs);
        js_free_rt(rt, w);
    }
}

// Convert ArrayBuffer to hex string
static JSValue js_encode_hex(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t data_len;
    uint8_t *data = JS_GetArrayBuffer(ctx, &data_len, argv[0]);
    if (!data) return JS_ThrowTypeError(ctx, "Expected ArrayBuffer");

    // Each byte becomes 2 hex characters
    char *hex = js_malloc(ctx, data_len * 2 + 1);
    if (!hex) return JS_ThrowOutOfMemory(ctx);

    // Lookup table for hex conversion
    static const char hex_chars[] = "0123456789abcdef";

    for (size_t i = 0; i < data_len; i++) {
        hex[i * 2] = hex_chars[data[i] >> 4];
        hex[i * 2 + 1] = hex_chars[data[i] & 0x0F];
    }
    hex[data_len * 2] = '\0';

    JSValue result = JS_NewString(ctx, hex);
    js_free(ctx, hex);
    return result;
}

static uint8_t hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0xFF;  // Invalid hex character marker
}

// Convert hex string to ArrayBuffer
static JSValue js_decode_hex(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *hex = JS_ToCString(ctx, argv[0]);
    if (!hex) return JS_ThrowTypeError(ctx, "Expected string");

    size_t hex_len = strlen(hex);
    if (hex_len % 2 != 0) {
        JS_FreeCString(ctx, hex);
        return JS_ThrowTypeError(ctx, "Invalid hex string length");
    }

    size_t data_len = hex_len / 2;
    uint8_t *data = js_malloc(ctx, data_len);
    if (!data) {
        JS_FreeCString(ctx, hex);
        return JS_ThrowOutOfMemory(ctx);
    }

    for (size_t i = 0; i < data_len; i++) {
        char high = hex[i * 2];
        char low = hex[i * 2 + 1];
        uint8_t high_val = hex_char_to_int(high);
        uint8_t low_val = hex_char_to_int(low);

        if (high_val == 0xFF || low_val == 0xFF) {
            js_free(ctx, data);
            JS_FreeCString(ctx, hex);
            return JS_ThrowTypeError(ctx, "Invalid hex character");
        }

        data[i] = (high_val << 4) | low_val;
    }

    JS_FreeCString(ctx, hex);
    return JS_NewArrayBufferCopy(ctx, data, data_len);
}

// Convert ArrayBuffer to base64 string
static JSValue js_encode_base64(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t data_len;
    uint8_t *data = JS_GetArrayBuffer(ctx, &data_len, argv[0]);
    if (!data) return JS_ThrowTypeError(ctx, "Expected ArrayBuffer");

    char *base64 = NULL;
    size_t base64_len = 0;
    int err = qjs_base64_encode((const char *)data, data_len, &base64, &base64_len);
    if (err) {
        return JS_ThrowTypeError(ctx, "base64 encode error");
    }
    JSValue result = JS_NewString(ctx, base64);
    js_free(ctx, base64);
    return result;
}

// Convert base64 string to ArrayBuffer
static JSValue js_decode_base64(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *base64 = JS_ToCString(ctx, argv[0]);
    if (!base64) return JS_ThrowTypeError(ctx, "Expected string");
    unsigned char *data = NULL;
    size_t data_len = 0;

    int err = qjs_base64_decode(base64, &data, &data_len);
    if (err) {
        JS_FreeCString(ctx, base64);
        return JS_ThrowTypeError(ctx, "base64 decode error");
    }

    JS_FreeCString(ctx, base64);
    return JS_NewArrayBufferCopy(ctx, data, data_len);
}

// Update the function list to include hex and base64 functions under separate objects
static const JSCFunctionListEntry js_hex_funcs[] = {
    JS_CFUNC_DEF("encode", 1, js_encode_hex),
    JS_CFUNC_DEF("decode", 1, js_decode_hex),
};

static const JSCFunctionListEntry js_base64_funcs[] = {
    JS_CFUNC_DEF("encode", 1, js_encode_base64),
    JS_CFUNC_DEF("decode", 1, js_decode_base64),
};

// SMT class definition
static const JSCFunctionListEntry js_smt_proto_funcs[] = {
    JS_CFUNC_DEF("insert", 2, js_smt_insert),
    JS_CFUNC_DEF("verify", 2, js_smt_verify),
};

static const JSClassDef js_smt_class = {
    "Smt",
    .finalizer = js_smt_finalizer,
};

// Add these new function definitions
static JSValue js_throw_exception(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *message = "Test exception";
    if (argc > 0) {
        message = JS_ToCString(ctx, argv[0]);
        if (!message) return JS_EXCEPTION;
    }
    JSValue error = JS_ThrowInternalError(ctx, "This is a test exception, %s", message);
    if (argc > 0) JS_FreeCString(ctx, message);
    return error;
}

static bool my_isdigit(int c) { return (c >= '0' && c <= '9'); }

static JSValue js_printf_internal(JSContext *ctx, int argc, JSValueConst *argv, bool to_console) {
    char fmtbuf[32];
    uint8_t cbuf[UTF8_CHAR_LEN_MAX + 1];
    JSValue res;
    DynBuf dbuf;
    const char *fmt_str;
    const uint8_t *fmt, *fmt_end;
    const uint8_t *p;
    char *q;
    int i, c, len, mod;
    size_t fmt_len;
    int32_t int32_arg;
    int64_t int64_arg;
    double double_arg;
    const char *string_arg;
    /* Use indirect call to dbuf_printf to prevent gcc warning */
    int (*dbuf_printf_fun)(DynBuf *s, const char *fmt, ...) = (void *)dbuf_printf;

    qjs_dbuf_init(ctx, &dbuf);

    if (argc > 0) {
        fmt_str = JS_ToCStringLen(ctx, &fmt_len, argv[0]);
        if (!fmt_str) goto fail;

        i = 1;
        fmt = (const uint8_t *)fmt_str;
        fmt_end = fmt + fmt_len;
        while (fmt < fmt_end) {
            for (p = fmt; fmt < fmt_end && *fmt != '%'; fmt++) continue;
            dbuf_put(&dbuf, p, fmt - p);
            if (fmt >= fmt_end) break;
            q = fmtbuf;
            *q++ = *fmt++; /* copy '%' */

            /* flags */
            for (;;) {
                c = *fmt;
                if (c == '0' || c == '#' || c == '+' || c == '-' || c == ' ' || c == '\'') {
                    if (q >= fmtbuf + sizeof(fmtbuf) - 1) goto invalid;
                    *q++ = c;
                    fmt++;
                } else {
                    break;
                }
            }
            /* width */
            if (*fmt == '*') {
                if (i >= argc) goto missing;
                if (JS_ToInt32(ctx, &int32_arg, argv[i++])) goto fail;
                q += snprintf(q, fmtbuf + sizeof(fmtbuf) - q, "%d", int32_arg);
                fmt++;
            } else {
                while (my_isdigit(*fmt)) {
                    if (q >= fmtbuf + sizeof(fmtbuf) - 1) goto invalid;
                    *q++ = *fmt++;
                }
            }
            if (*fmt == '.') {
                if (q >= fmtbuf + sizeof(fmtbuf) - 1) goto invalid;
                *q++ = *fmt++;
                if (*fmt == '*') {
                    if (i >= argc) goto missing;
                    if (JS_ToInt32(ctx, &int32_arg, argv[i++])) goto fail;
                    q += snprintf(q, fmtbuf + sizeof(fmtbuf) - q, "%d", int32_arg);
                    fmt++;
                } else {
                    while (my_isdigit(*fmt)) {
                        if (q >= fmtbuf + sizeof(fmtbuf) - 1) goto invalid;
                        *q++ = *fmt++;
                    }
                }
            }

            /* we only support the "l" modifier for 64 bit numbers */
            mod = ' ';
            if (*fmt == 'l') {
                mod = *fmt++;
            }

            /* type */
            c = *fmt++;
            if (q >= fmtbuf + sizeof(fmtbuf) - 1) goto invalid;
            *q++ = c;
            *q = '\0';

            switch (c) {
                case 'c':
                    if (i >= argc) goto missing;
                    if (JS_IsString(argv[i])) {
                        string_arg = JS_ToCString(ctx, argv[i++]);
                        if (!string_arg) goto fail;
                        int32_arg = unicode_from_utf8((uint8_t *)string_arg, UTF8_CHAR_LEN_MAX, &p);
                        JS_FreeCString(ctx, string_arg);
                    } else {
                        if (JS_ToInt32(ctx, &int32_arg, argv[i++])) goto fail;
                    }
                    /* handle utf-8 encoding explicitly */
                    if ((unsigned)int32_arg > 0x10FFFF) int32_arg = 0xFFFD;
                    /* ignore conversion flags, width and precision */
                    len = unicode_to_utf8(cbuf, int32_arg);
                    dbuf_put(&dbuf, cbuf, len);
                    break;

                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'x':
                case 'X':
                    if (i >= argc) goto missing;
                    if (JS_ToInt64Ext(ctx, &int64_arg, argv[i++])) goto fail;
                    if (mod == 'l') {
                        /* 64 bit number */
                        if (q >= fmtbuf + sizeof(fmtbuf) - 2) goto invalid;
                        q[1] = q[-1];
                        q[-1] = q[0] = 'l';
                        q[2] = '\0';
                        dbuf_printf_fun(&dbuf, fmtbuf, (long long)int64_arg);
                    } else {
                        dbuf_printf_fun(&dbuf, fmtbuf, (int)int64_arg);
                    }
                    break;

                case 's':
                    if (i >= argc) goto missing;
                    /* XXX: handle strings containing null characters */
                    string_arg = JS_ToCString(ctx, argv[i++]);
                    if (!string_arg) goto fail;
                    dbuf_printf_fun(&dbuf, fmtbuf, string_arg);
                    JS_FreeCString(ctx, string_arg);
                    break;

                case 'e':
                case 'f':
                case 'g':
                case 'a':
                case 'E':
                case 'F':
                case 'G':
                case 'A':
                    if (i >= argc) goto missing;
                    if (JS_ToFloat64(ctx, &double_arg, argv[i++])) goto fail;
                    dbuf_printf_fun(&dbuf, fmtbuf, double_arg);
                    break;

                case '%':
                    dbuf_putc(&dbuf, '%');
                    break;

                default:
                    /* XXX: should support an extension mechanism */
                invalid:
                    JS_ThrowTypeError(ctx, "invalid conversion specifier in format string");
                    goto fail;
                missing:
                    JS_ThrowReferenceError(ctx, "missing argument for conversion specifier");
                    goto fail;
            }
        }
        JS_FreeCString(ctx, fmt_str);
    }
    if (dbuf.error) {
        res = JS_ThrowOutOfMemory(ctx);
    } else {
        if (to_console) {
            dbuf_putc(&dbuf, '\0');
            ckb_debug((char *)dbuf.buf);
            res = JS_NewInt32(ctx, len);
        } else {
            res = JS_NewStringLen(ctx, (char *)dbuf.buf, dbuf.size);
        }
    }
    dbuf_free(&dbuf);
    return res;

fail:
    dbuf_free(&dbuf);
    return JS_EXCEPTION;
}

static JSValue js_std_sprintf(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    return js_printf_internal(ctx, argc, argv, false);
}

static JSValue js_std_printf(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    return js_printf_internal(ctx, argc, argv, true);
}

static const JSCFunctionListEntry js_misc_funcs[] = {
    JS_CFUNC_DEF("throw_exception", 1, js_throw_exception),
    JS_CFUNC_DEF("sprintf", 1, js_std_sprintf),
    JS_CFUNC_DEF("printf", 1, js_std_printf),
};

// TextDecoder decode method
static JSValue js_text_decoder_decode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t data_len = 0;
    uint8_t *data = NULL;
    size_t per_element = 0;
    JSValue value = JS_GetTypedArrayBuffer(ctx, argv[0], NULL, NULL, &per_element);

    if (JS_IsException(value) || per_element != 1) {
        return JS_ThrowTypeError(ctx, "Invalid argument type");
    }
    data = JS_GetArrayBuffer(ctx, &data_len, value);
    JSValue result = JS_NewStringLen(ctx, (char *)data, data_len);
    JS_FreeValue(ctx, value);
    return result;
}

// TextEncoder encode method
static JSValue js_text_encoder_encode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t str_len;
    const char *str = JS_ToCStringLen(ctx, &str_len, argv[0]);
    if (!str) {
        return JS_ThrowTypeError(ctx, "Expected string");
    }
    JSValue uint8_array = qjs_create_uint8_array(ctx, (uint8_t *)str, str_len);
    JS_FreeCString(ctx, str);
    return uint8_array;
}

// TextDecoder constructor
static JSValue js_text_decoder_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
    return JS_NewObjectClass(ctx, js_text_decoder_class_id);
}

// TextEncoder constructor
static JSValue js_text_encoder_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
    return JS_NewObjectClass(ctx, js_text_encoder_class_id);
}

// TextDecoder class definition
static const JSCFunctionListEntry js_text_decoder_proto_funcs[] = {
    JS_CFUNC_DEF("decode", 1, js_text_decoder_decode),
};

static const JSClassDef js_text_decoder_class = {
    "TextDecoder",
};

// TextEncoder class definition
static const JSCFunctionListEntry js_text_encoder_proto_funcs[] = {
    JS_CFUNC_DEF("encode", 1, js_text_encoder_encode),
};

static const JSClassDef js_text_encoder_class = {
    "TextEncoder",
};

// Update the init function to include TextEncoder and TextDecoder
int qjs_init_module_misc_lazy(JSContext *ctx, JSModuleDef *m) {
    JSValue proto, obj;

    // Initialize SMT class
    JS_NewClassID(&js_smt_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_smt_class_id, &js_smt_class);

    // Create and setup prototype
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_smt_proto_funcs, countof(js_smt_proto_funcs));

    // Create constructor
    obj = JS_NewCFunction2(ctx, js_smt_constructor, "Smt", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, obj, proto);
    JS_SetClassProto(ctx, js_smt_class_id, proto);

    // Export the SMT constructor and encoding functions
    JS_SetModuleExport(ctx, m, "Smt", obj);

    // Create hex object and add functions
    JSValue hex = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, hex, js_hex_funcs, countof(js_hex_funcs));
    JS_SetModuleExport(ctx, m, "hex", hex);

    // Create base64 object and add functions
    JSValue base64 = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, base64, js_base64_funcs, countof(js_base64_funcs));
    JS_SetModuleExport(ctx, m, "base64", base64);

    // Initialize TextDecoder class
    JS_NewClassID(&js_text_decoder_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_text_decoder_class_id, &js_text_decoder_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_text_decoder_proto_funcs, countof(js_text_decoder_proto_funcs));
    obj = JS_NewCFunction2(ctx, js_text_decoder_constructor, "TextDecoder", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, obj, proto);
    JS_SetClassProto(ctx, js_text_decoder_class_id, proto);
    JS_SetModuleExport(ctx, m, "TextDecoder", obj);

    // Initialize TextEncoder class
    JS_NewClassID(&js_text_encoder_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_text_encoder_class_id, &js_text_encoder_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_text_encoder_proto_funcs, countof(js_text_encoder_proto_funcs));
    obj = JS_NewCFunction2(ctx, js_text_encoder_constructor, "TextEncoder", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, obj, proto);
    JS_SetClassProto(ctx, js_text_encoder_class_id, proto);
    JS_SetModuleExport(ctx, m, "TextEncoder", obj);

    // functions without submodule
    JS_SetModuleExportList(ctx, m, js_misc_funcs, countof(js_misc_funcs));

    return 0;
}

int qjs_init_module_misc(JSContext *ctx, JSModuleDef *m) {
    JS_AddModuleExport(ctx, m, "Smt");
    JS_AddModuleExport(ctx, m, "hex");
    JS_AddModuleExport(ctx, m, "base64");
    JS_AddModuleExport(ctx, m, "TextDecoder");
    JS_AddModuleExport(ctx, m, "TextEncoder");
    JS_AddModuleExportList(ctx, m, js_misc_funcs, countof(js_misc_funcs));
    return 0;
}
