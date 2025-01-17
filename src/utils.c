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
