#include <stdbool.h>
#include "utils.h"
#include "cutils.h"
#include "quickjs.h"

bool qjs_bad_int_arg(JSContext *ctx, JSValue val, int index) {
    uint32_t tag = JS_VALUE_GET_TAG(val);
    if (tag == JS_TAG_INT) {
        return false;
    } else {
        JS_ThrowTypeError(ctx, "Invalid argument: expected integer at index %d", index);
        return true;
    }
}

bool qjs_bad_bigint_arg(JSContext *ctx, JSValue val, int index) {
    uint32_t tag = JS_VALUE_GET_TAG(val);
    if (tag == JS_TAG_INT || tag == JS_TAG_BIG_INT || tag == JS_TAG_BIG_FLOAT || tag == JS_TAG_FLOAT64) {
        return false;
    } else {
        JS_ThrowTypeError(ctx, "Invalid argument: expected (big) integer at index %d", index);
        return true;
    }
}

bool qjs_bad_str_arg(JSContext *ctx, JSValue val, int index) {
    uint32_t tag = JS_VALUE_GET_TAG(val);
    if (tag == JS_TAG_STRING) {
        return false;
    } else {
        JS_ThrowTypeError(ctx, "Invalid argument: expected string at index %d", index);
        return true;
    }
}

void qjs_dbuf_init(JSContext *ctx, DynBuf *s) { dbuf_init2(s, JS_GetRuntime(ctx), (DynBufReallocFunc *)js_realloc_rt); }

JSValue qjs_create_uint8_array(JSContext *ctx, const uint8_t *data, size_t length) {
    // Create ArrayBuffer with the data
    JSValue buffer = JS_NewArrayBufferCopy(ctx, data, length);
    if (JS_IsException(buffer)) {
        return JS_EXCEPTION;
    }

    // Create constructor arguments
    JSValue args[3];
    args[0] = buffer;
    args[1] = JS_NewInt64(ctx, 0);       // offset
    args[2] = JS_NewInt64(ctx, length);  // length

    // Get Uint8Array constructor
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue constructor = JS_GetPropertyStr(ctx, global, "Uint8Array");

    // Create the array
    JSValue array = JS_CallConstructor(ctx, constructor, 3, args);

    // Clean up
    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, constructor);
    JS_FreeValue(ctx, buffer);

    return array;
}
