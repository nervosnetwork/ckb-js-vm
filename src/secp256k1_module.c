#include "cutils.h"
#include "secp256k1_module.h"
#include "secp256k1.h"
#include "secp256k1_recovery.h"
#include "group.h"
#undef CHECK
#undef CHECK2
#include "qjs.h"

// from the secp256k1 library. The original table is used for signing operations and
// public key generation, and consumes significant memory. Since we only need verification
// functionality in our on-chain scripts (not signing or key generation), we can safely
// use this minimal placeholder to reduce memory usage.
const secp256k1_ge_storage secp256k1_ecmult_gen_prec_table[1][1];
const secp256k1_context *g_secp256k1_context = NULL;

static JSValue recover(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    int err = 0;
    JSValue ret = JS_EXCEPTION;
    size_t msg_len, sig_len;
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_recoverable_signature signature;
    uint8_t *msg = NULL, *sig = NULL;

    // Check argument count
    if (argc != 3) return JS_ThrowTypeError(ctx, "wrong number of arguments");

    // Get signature from first argument
    sig = JS_GetArrayBuffer(ctx, &sig_len, argv[0]);
    if (!sig || sig_len != 64) return JS_ThrowTypeError(ctx, "invalid signature format");

    // Get recovery id from second argument
    int recid;
    if (JS_ToInt32(ctx, &recid, argv[1])) {
        return JS_ThrowTypeError(ctx, "invalid recovery id");
    }
    if (recid < 0 || recid > 3) {
        return JS_ThrowTypeError(ctx, "recovery id must be between 0 and 3");
    }

    // Parse the recoverable signature
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(g_secp256k1_context, &signature, sig, recid)) {
        return JS_ThrowTypeError(ctx, "invalid signature");
    }

    // Get message hash from third argument
    msg = JS_GetArrayBuffer(ctx, &msg_len, argv[2]);
    if (!msg || msg_len != 32) return JS_ThrowTypeError(ctx, "message must be 32 bytes");

    // Perform the recovery
    int success = secp256k1_ecdsa_recover(g_secp256k1_context, &pubkey, &signature, msg);
    CHECK2(success, -1);

    // Return the public key with 64-bytes
    ret = JS_NewArrayBufferCopy(ctx, pubkey.data, sizeof(pubkey));

exit:
    if (err) return JS_EXCEPTION;
    return ret;
}

static const JSCFunctionListEntry js_secp256k1_funcs[] = {
    JS_CFUNC_DEF("recover", 3, recover),
};

static int js_secp256k1_init(JSContext *ctx, JSModuleDef *m) {
    JS_SetModuleExportList(ctx, m, js_secp256k1_funcs, countof(js_secp256k1_funcs));
    return 0;
}

int js_init_module_secp256k1(JSContext *js_context) {
    g_secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    JSModuleDef *m = JS_NewCModule(js_context, "secp256k1", js_secp256k1_init);
    if (!m) {
        return -1;
    }
    return 0;
}
