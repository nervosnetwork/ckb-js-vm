#include <stdlib.h>
#include <string.h>
#include "cutils.h"
#include "misc_module.h"
#define BLAKE2_IMPL_H
#define BLAKE2_REF_C
#include "blake2b.h"
#include "ckb_smt.h"

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

    static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t encoded_len = ((data_len + 2) / 3) * 4;
    char *base64 = js_malloc(ctx, encoded_len + 1);
    if (!base64) return JS_ThrowOutOfMemory(ctx);

    size_t i = 0, j = 0;
    while (i < data_len) {
        uint32_t octet_a = i < data_len ? data[i++] : 0;
        uint32_t octet_b = i < data_len ? data[i++] : 0;
        uint32_t octet_c = i < data_len ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        base64[j++] = base64_chars[(triple >> 18) & 0x3F];
        base64[j++] = base64_chars[(triple >> 12) & 0x3F];
        base64[j++] = base64_chars[(triple >> 6) & 0x3F];
        base64[j++] = base64_chars[triple & 0x3F];
    }

    // Add padding
    if (data_len % 3 >= 1) base64[encoded_len - 1] = '=';
    if (data_len % 3 == 1) base64[encoded_len - 2] = '=';
    base64[encoded_len] = '\0';

    JSValue result = JS_NewString(ctx, base64);
    js_free(ctx, base64);
    return result;
}

// Convert base64 string to ArrayBuffer
static JSValue js_decode_base64(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *base64 = JS_ToCString(ctx, argv[0]);
    if (!base64) return JS_ThrowTypeError(ctx, "Expected string");

    static const uint8_t base64_table[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57,
        58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 64, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
        37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};

    size_t base64_len = strlen(base64);
    if (base64_len % 4 != 0) {
        JS_FreeCString(ctx, base64);
        return JS_ThrowTypeError(ctx, "Invalid base64 string length");
    }

    size_t padding = 0;
    if (base64_len > 0 && base64[base64_len - 1] == '=') padding++;
    if (base64_len > 1 && base64[base64_len - 2] == '=') padding++;

    size_t data_len = (base64_len / 4) * 3 - padding;
    uint8_t *data = js_malloc(ctx, data_len);
    if (!data) {
        JS_FreeCString(ctx, base64);
        return JS_ThrowOutOfMemory(ctx);
    }

    size_t i = 0, j = 0;
    while (i < base64_len) {
        uint32_t sextet_a = base64_table[(uint8_t)base64[i++]];
        uint32_t sextet_b = base64_table[(uint8_t)base64[i++]];
        uint32_t sextet_c = base64_table[(uint8_t)base64[i++]];
        uint32_t sextet_d = base64_table[(uint8_t)base64[i++]];

        if (sextet_a == 64 || sextet_b == 64 || sextet_c == 64 || sextet_d == 64) {
            js_free(ctx, data);
            JS_FreeCString(ctx, base64);
            return JS_ThrowTypeError(ctx, "Invalid base64 character");
        }

        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

        if (j < data_len) data[j++] = (triple >> 16) & 0xFF;
        if (j < data_len) data[j++] = (triple >> 8) & 0xFF;
        if (j < data_len) data[j++] = triple & 0xFF;
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


static int js_misc_init(JSContext *ctx, JSModuleDef *m) {
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

    return 0;
}

int js_init_module_misc(JSContext *ctx) {
    JSModuleDef *m;
    m = JS_NewCModule(ctx, "misc", js_misc_init);
    if (!m) return -1;

    JS_AddModuleExport(ctx, m, "Smt");
    JS_AddModuleExport(ctx, m, "hex");
    JS_AddModuleExport(ctx, m, "base64");
    return 0;
}
