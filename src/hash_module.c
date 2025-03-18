#include <stdbool.h>
#include "cutils.h"
#include "hash_module.h"
#include "sha256.h"
#include "ckb_keccak256.h"
#include "blake2b.h"
#include "ripemd160.h"
#include "qjs.h"

#define BLAKE2B_HASH_SIZE 32

static int js_blake2b_init(blake2b_state *S, size_t outlen, char personal[BLAKE2B_PERSONALBYTES]) {
    blake2b_param P[1];

    if ((!outlen) || (outlen > BLAKE2B_HASH_SIZE)) return QJS_ERROR_GENERIC;

    P->digest_length = (uint8_t)outlen;
    P->key_length = 0;
    P->fanout = 1;
    P->depth = 1;
    store32(&P->leaf_length, 0);
    store32(&P->node_offset, 0);
    store32(&P->xof_length, 0);
    P->node_depth = 0;
    P->inner_length = 0;
    memset(P->reserved, 0, sizeof(P->reserved));
    memset(P->salt, 0, sizeof(P->salt));
    memset(P->personal, 0, sizeof(P->personal));
    for (int i = 0; i < BLAKE2B_PERSONALBYTES; ++i) {
        (P->personal)[i] = personal[i];
    }
    return blake2b_init_param(S, P);
}

static void free_hash_context(JSRuntime *rt, void *opaque, void *ptr) { js_free_rt(rt, ptr); }

static JSClassID js_sha256_class_id;

static void js_sha256_finalizer(JSRuntime *rt, JSValue val) {
    SHA256_CTX *hash = JS_GetOpaque(val, js_sha256_class_id);
    if (hash) {
        js_free_rt(rt, hash);
    }
}

static JSClassDef js_sha256_class = {
    "Sha256",
    .finalizer = js_sha256_finalizer,
};

// Constructor for Sha256 class
static JSValue js_sha256_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
    JSValue obj = JS_UNDEFINED;
    JSValue proto;
    SHA256_CTX *hash;

    // Create the object with proper prototype
    if (JS_IsUndefined(new_target)) {
        proto = JS_GetClassProto(ctx, js_sha256_class_id);
    } else {
        proto = JS_GetPropertyStr(ctx, new_target, "prototype");
        if (JS_IsException(proto)) goto fail;
    }
    obj = JS_NewObjectProtoClass(ctx, proto, js_sha256_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj)) goto fail;

    // Initialize hash context
    hash = js_mallocz(ctx, sizeof(*hash));
    if (!hash) goto fail;

    sha256_init(hash);
    JS_SetOpaque(obj, hash);
    return obj;

fail:
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

// Method definitions for Sha256 prototype
static JSValue js_sha256_write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t data_len;
    uint8_t *data;
    SHA256_CTX *hash;

    hash = JS_GetOpaque2(ctx, this_val, js_sha256_class_id);
    if (!hash) return JS_EXCEPTION;

    data = JS_GetArrayBuffer(ctx, &data_len, argv[0]);
    if (!data) return JS_ThrowTypeError(ctx, "invalid data");

    sha256_update(hash, data, data_len);
    return JS_UNDEFINED;
}

static JSValue js_sha256_finalize(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    SHA256_CTX *hash;
    uint8_t *output;

    hash = JS_GetOpaque2(ctx, this_val, js_sha256_class_id);
    if (!hash) return JS_EXCEPTION;

    output = js_malloc(ctx, SHA256_BLOCK_SIZE);
    if (!output) return JS_ThrowOutOfMemory(ctx);

    sha256_final(hash, output);
    return JS_NewArrayBuffer(ctx, output, SHA256_BLOCK_SIZE, free_hash_context, NULL, false);
}

static const JSCFunctionListEntry js_sha256_proto_funcs[] = {
    JS_CFUNC_DEF("update", 1, js_sha256_write),
    JS_CFUNC_DEF("finalize", 0, js_sha256_finalize),
};

static JSClassID js_keccak256_class_id;

static void js_keccak256_finalizer(JSRuntime *rt, JSValue val) {
    SHA3_CTX *hash = JS_GetOpaque(val, js_keccak256_class_id);
    if (hash) {
        js_free_rt(rt, hash);
    }
}

static JSClassDef js_keccak256_class = {
    "Keccak256",
    .finalizer = js_keccak256_finalizer,
};

// Constructor for Keccak256 class
static JSValue js_keccak256_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
    JSValue obj = JS_UNDEFINED;
    JSValue proto;
    SHA3_CTX *hash;

    if (JS_IsUndefined(new_target)) {
        proto = JS_GetClassProto(ctx, js_keccak256_class_id);
    } else {
        proto = JS_GetPropertyStr(ctx, new_target, "prototype");
        if (JS_IsException(proto)) goto fail;
    }
    obj = JS_NewObjectProtoClass(ctx, proto, js_keccak256_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj)) goto fail;

    hash = js_mallocz(ctx, sizeof(*hash));
    if (!hash) goto fail;

    keccak_init(hash);
    JS_SetOpaque(obj, hash);
    return obj;

fail:
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

// Method definitions for Keccak256 prototype
static JSValue js_keccak256_write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t data_len;
    uint8_t *data;
    SHA3_CTX *hash;

    hash = JS_GetOpaque2(ctx, this_val, js_keccak256_class_id);
    if (!hash) return JS_EXCEPTION;

    data = JS_GetArrayBuffer(ctx, &data_len, argv[0]);
    if (!data) return JS_ThrowTypeError(ctx, "invalid data");

    keccak_update(hash, data, data_len);
    return JS_UNDEFINED;
}

static JSValue js_keccak256_finalize(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    SHA3_CTX *hash;
    uint8_t *output;
    const size_t KECCAK256_SIZE = 32;  // 256 bits = 32 bytes

    hash = JS_GetOpaque2(ctx, this_val, js_keccak256_class_id);
    if (!hash) return JS_EXCEPTION;

    output = js_malloc(ctx, KECCAK256_SIZE);
    if (!output) return JS_ThrowOutOfMemory(ctx);

    keccak_final(hash, output);
    return JS_NewArrayBuffer(ctx, output, KECCAK256_SIZE, free_hash_context, NULL, false);
}

static const JSCFunctionListEntry js_keccak256_proto_funcs[] = {
    JS_CFUNC_DEF("update", 1, js_keccak256_write),
    JS_CFUNC_DEF("finalize", 0, js_keccak256_finalize),
};

static JSClassID js_blake2b_class_id;

static void js_blake2b_finalizer(JSRuntime *rt, JSValue val) {
    blake2b_state *hash = JS_GetOpaque(val, js_blake2b_class_id);
    if (hash) {
        js_free_rt(rt, hash);
    }
}

static JSClassDef js_blake2b_class = {
    "Blake2b",
    .finalizer = js_blake2b_finalizer,
};

// Constructor for Blake2b class
static JSValue js_blake2b_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
    JSValue obj = JS_UNDEFINED;
    JSValue proto;
    blake2b_state *hash;
    const char *personal = NULL;
    size_t personal_len;

    // Get personalization string from first argument
    if (argc != 1) {
        JS_ThrowTypeError(ctx, "must provide personal with size of 16");
    }
    personal = JS_ToCStringLen(ctx, &personal_len, argv[0]);
    if (!personal) goto fail;
    if (personal_len != BLAKE2B_PERSONALBYTES) {
        JS_FreeCString(ctx, personal);
        JS_ThrowTypeError(ctx, "personal must be %d bytes", BLAKE2B_PERSONALBYTES);
        goto fail;
    }

    // Create the object with proper prototype
    if (JS_IsUndefined(new_target)) {
        proto = JS_GetClassProto(ctx, js_blake2b_class_id);
    } else {
        proto = JS_GetPropertyStr(ctx, new_target, "prototype");
        if (JS_IsException(proto)) {
            if (personal) JS_FreeCString(ctx, personal);
            goto fail;
        }
    }
    obj = JS_NewObjectProtoClass(ctx, proto, js_blake2b_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj)) {
        if (personal) JS_FreeCString(ctx, personal);
        goto fail;
    }

    // Initialize hash context
    hash = js_mallocz(ctx, sizeof(*hash));
    if (!hash) {
        if (personal) JS_FreeCString(ctx, personal);
        goto fail;
    }

    if (js_blake2b_init(hash, BLAKE2B_HASH_SIZE, (char *)personal) < 0) {
        js_free(ctx, hash);
        if (personal) JS_FreeCString(ctx, personal);
        JS_ThrowInternalError(ctx, "Failed to initialize Blake2b hash");
        goto fail;
    }

    if (personal) JS_FreeCString(ctx, personal);
    JS_SetOpaque(obj, hash);
    return obj;

fail:
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

// Method definitions for Blake2b prototype
static JSValue js_blake2b_write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t data_len;
    uint8_t *data;
    blake2b_state *hash;

    hash = JS_GetOpaque2(ctx, this_val, js_blake2b_class_id);
    if (!hash) return JS_EXCEPTION;

    data = JS_GetArrayBuffer(ctx, &data_len, argv[0]);
    if (!data) return JS_ThrowTypeError(ctx, "invalid data");

    blake2b_update(hash, data, data_len);
    return JS_UNDEFINED;
}

static JSValue js_blake2b_finalize(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    blake2b_state *hash;
    uint8_t *output;

    hash = JS_GetOpaque2(ctx, this_val, js_blake2b_class_id);
    if (!hash) return JS_EXCEPTION;

    output = js_malloc(ctx, BLAKE2B_HASH_SIZE);
    if (!output) return JS_ThrowOutOfMemory(ctx);

    blake2b_final(hash, output, BLAKE2B_HASH_SIZE);
    return JS_NewArrayBuffer(ctx, output, BLAKE2B_HASH_SIZE, free_hash_context, NULL, false);
}

static const JSCFunctionListEntry js_blake2b_proto_funcs[] = {
    JS_CFUNC_DEF("update", 1, js_blake2b_write),
    JS_CFUNC_DEF("finalize", 0, js_blake2b_finalize),
};

static JSClassID js_ripemd160_class_id;

static void js_ripemd160_finalizer(JSRuntime *rt, JSValue val) {
    ripemd160_state *hash = JS_GetOpaque(val, js_ripemd160_class_id);
    if (hash) {
        js_free_rt(rt, hash);
    }
}

static JSClassDef js_ripemd160_class = {
    "Ripemd160",
    .finalizer = js_ripemd160_finalizer,
};

// Constructor for Ripemd160 class
static JSValue js_ripemd160_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
    JSValue obj = JS_UNDEFINED;
    JSValue proto;
    ripemd160_state *hash;

    // Create the object with proper prototype
    if (JS_IsUndefined(new_target)) {
        proto = JS_GetClassProto(ctx, js_ripemd160_class_id);
    } else {
        proto = JS_GetPropertyStr(ctx, new_target, "prototype");
        if (JS_IsException(proto)) goto fail;
    }
    obj = JS_NewObjectProtoClass(ctx, proto, js_ripemd160_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj)) goto fail;

    // Initialize hash context
    hash = js_mallocz(ctx, sizeof(*hash));
    if (!hash) goto fail;

    ripemd160_init(hash);
    JS_SetOpaque(obj, hash);
    return obj;

fail:
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

// Method definitions for Ripemd160 prototype
static JSValue js_ripemd160_write(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t data_len;
    uint8_t *data;
    ripemd160_state *hash;

    hash = JS_GetOpaque2(ctx, this_val, js_ripemd160_class_id);
    if (!hash) return JS_EXCEPTION;

    data = JS_GetArrayBuffer(ctx, &data_len, argv[0]);
    if (!data) return JS_ThrowTypeError(ctx, "invalid data");

    ripemd160_update(hash, data, data_len);
    return JS_UNDEFINED;
}

static JSValue js_ripemd160_finalize(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    ripemd160_state *hash;
    uint8_t *output;
    const size_t RIPEMD160_SIZE = 20;  // 160 bits = 20 bytes

    hash = JS_GetOpaque2(ctx, this_val, js_ripemd160_class_id);
    if (!hash) return JS_EXCEPTION;

    output = js_malloc(ctx, RIPEMD160_SIZE);
    if (!output) return JS_ThrowOutOfMemory(ctx);

    ripemd160_finalize(hash, output);
    return JS_NewArrayBuffer(ctx, output, RIPEMD160_SIZE, free_hash_context, NULL, false);
}

static const JSCFunctionListEntry js_ripemd160_proto_funcs[] = {
    JS_CFUNC_DEF("update", 1, js_ripemd160_write),
    JS_CFUNC_DEF("finalize", 0, js_ripemd160_finalize),
};

int qjs_init_module_hash_lazy(JSContext *ctx, JSModuleDef *m) {
    JSValue proto, obj;

    // Initialize Sha256 class
    JS_NewClassID(&js_sha256_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_sha256_class_id, &js_sha256_class);

    // Create and setup prototype
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_sha256_proto_funcs, countof(js_sha256_proto_funcs));

    // Create constructor
    obj = JS_NewCFunction2(ctx, js_sha256_ctor, "Sha256", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, obj, proto);
    JS_SetClassProto(ctx, js_sha256_class_id, proto);

    // Export the Sha256 constructor
    JS_SetModuleExport(ctx, m, "Sha256", obj);

    // Initialize Keccak256 class
    JS_NewClassID(&js_keccak256_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_keccak256_class_id, &js_keccak256_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_keccak256_proto_funcs, countof(js_keccak256_proto_funcs));
    obj = JS_NewCFunction2(ctx, js_keccak256_ctor, "Keccak256", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, obj, proto);
    JS_SetClassProto(ctx, js_keccak256_class_id, proto);
    JS_SetModuleExport(ctx, m, "Keccak256", obj);

    // Initialize Blake2b class
    JS_NewClassID(&js_blake2b_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_blake2b_class_id, &js_blake2b_class);

    // Create and setup prototype
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_blake2b_proto_funcs, countof(js_blake2b_proto_funcs));

    // Create constructor
    obj = JS_NewCFunction2(ctx, js_blake2b_ctor, "Blake2b", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, obj, proto);
    JS_SetClassProto(ctx, js_blake2b_class_id, proto);

    // Export the Blake2b constructor
    JS_SetModuleExport(ctx, m, "Blake2b", obj);

    // Initialize Ripemd160 class
    JS_NewClassID(&js_ripemd160_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_ripemd160_class_id, &js_ripemd160_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_ripemd160_proto_funcs, countof(js_ripemd160_proto_funcs));
    obj = JS_NewCFunction2(ctx, js_ripemd160_ctor, "Ripemd160", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, obj, proto);
    JS_SetClassProto(ctx, js_ripemd160_class_id, proto);
    JS_SetModuleExport(ctx, m, "Ripemd160", obj);

    return 0;
}

int qjs_init_module_hash(JSContext *ctx, JSModuleDef *m) {
    JS_AddModuleExport(ctx, m, "Sha256");
    JS_AddModuleExport(ctx, m, "Keccak256");
    JS_AddModuleExport(ctx, m, "Blake2b");
    JS_AddModuleExport(ctx, m, "Ripemd160");
    return 0;
}
