#include <stdbool.h>
#include "cutils.h"
#include "secp256k1_module.h"
#include "secp256k1.h"
#include "secp256k1_recovery.h"
#include "group.h"

#define COMPRESSED_PUBKEY_LENGTH 33    // 1 byte prefix + 32 bytes x coordinate
#define UNCOMPRESSED_PUBKEY_LENGTH 65  // 1 byte prefix + 32 bytes x + 32 bytes y

// from the secp256k1 library. The original table is used for signing operations and
// public key generation, and consumes significant memory. Since we only need verification
// functionality in our on-chain scripts (not signing or key generation), we can safely
// use this minimal placeholder to reduce memory usage.
const secp256k1_ge_storage secp256k1_ecmult_gen_prec_table[0][0];
const secp256k1_context *g_secp256k1_context = NULL;

static void free_array_buffer(JSRuntime *rt, void *_opaque, void *ptr) { js_free_rt(rt, ptr); }

static JSValue recover(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
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
    if (!success) {
        return JS_ThrowInternalError(ctx, "invalid signature");
    }
    // Return the public key with 64-bytes
    return JS_NewArrayBufferCopy(ctx, pubkey.data, sizeof(pubkey));
}

static JSValue serialize_pubkey(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t pubkey_len;
    uint8_t *pubkey_data;
    secp256k1_pubkey pubkey;
    unsigned int flags = SECP256K1_EC_COMPRESSED;

    // Check argument count
    if (argc != 2) return JS_ThrowTypeError(ctx, "wrong number of arguments");

    // Get public key from first argument
    pubkey_data = JS_GetArrayBuffer(ctx, &pubkey_len, argv[0]);
    if (!pubkey_data || pubkey_len != sizeof(secp256k1_pubkey)) {
        return JS_ThrowTypeError(ctx, "invalid public key format");
    }

    // Copy the public key data
    memcpy(&pubkey, pubkey_data, sizeof(secp256k1_pubkey));

    // Get compression flag from second argument if provided
    flags = JS_ToBool(ctx, argv[1]) ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED;

    // Allocate output buffer (33 bytes for compressed, 65 for uncompressed)
    size_t output_len = flags == SECP256K1_EC_COMPRESSED ? COMPRESSED_PUBKEY_LENGTH : UNCOMPRESSED_PUBKEY_LENGTH;
    uint8_t *output = js_malloc(ctx, output_len);
    if (!output) {
        return JS_ThrowOutOfMemory(ctx);
    }

    // Serialize the public key
    size_t serialized_len = output_len;
    if (!secp256k1_ec_pubkey_serialize(g_secp256k1_context, output, &serialized_len, &pubkey, flags)) {
        js_free(ctx, output);
        return JS_ThrowInternalError(ctx, "serialization failed");
    }

    return JS_NewArrayBuffer(ctx, output, serialized_len, free_array_buffer, ctx, false);
}

static JSValue parse_pubkey(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t input_len;
    uint8_t *input;
    secp256k1_pubkey pubkey;

    // Check argument count
    if (argc != 1) return JS_ThrowTypeError(ctx, "wrong number of arguments");

    // Get serialized public key from argument
    input = JS_GetArrayBuffer(ctx, &input_len, argv[0]);
    if (!input || (input_len != COMPRESSED_PUBKEY_LENGTH && input_len != UNCOMPRESSED_PUBKEY_LENGTH)) {
        return JS_ThrowTypeError(ctx, "invalid public key format");
    }

    // Parse the public key
    if (!secp256k1_ec_pubkey_parse(g_secp256k1_context, &pubkey, input, input_len)) {
        return JS_ThrowTypeError(ctx, "invalid public key");
    }

    // Return the parsed public key
    return JS_NewArrayBufferCopy(ctx, pubkey.data, sizeof(pubkey));
}

static JSValue verify(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t msg_len, sig_len, pubkey_len;
    uint8_t *msg = NULL, *sig = NULL, *pubkey_data = NULL;
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_signature signature;

    // Check argument count
    if (argc != 3) return JS_ThrowTypeError(ctx, "wrong number of arguments");

    // Get signature from first argument
    sig = JS_GetArrayBuffer(ctx, &sig_len, argv[0]);
    if (!sig || sig_len != 64) return JS_ThrowTypeError(ctx, "invalid signature format");

    // Parse the signature
    if (!secp256k1_ecdsa_signature_parse_compact(g_secp256k1_context, &signature, sig)) {
        return JS_ThrowTypeError(ctx, "invalid signature");
    }

    // Get message hash from second argument
    msg = JS_GetArrayBuffer(ctx, &msg_len, argv[1]);
    if (!msg || msg_len != 32) return JS_ThrowTypeError(ctx, "message must be 32 bytes");

    // Get public key from third argument
    pubkey_data = JS_GetArrayBuffer(ctx, &pubkey_len, argv[2]);
    if (!pubkey_data || pubkey_len != sizeof(secp256k1_pubkey)) {
        return JS_ThrowTypeError(ctx, "invalid public key format");
    }
    // Copy the public key data
    memcpy(&pubkey, pubkey_data, sizeof(secp256k1_pubkey));

    // Perform the verification
    int result = secp256k1_ecdsa_verify(g_secp256k1_context, &signature, msg, &pubkey);

    return JS_NewBool(ctx, result == 1);
}

static const JSCFunctionListEntry js_secp256k1_funcs[] = {
    JS_CFUNC_DEF("recover", 3, recover),
    JS_CFUNC_DEF("serializePubkey", 2, serialize_pubkey),
    JS_CFUNC_DEF("parsePubkey", 1, parse_pubkey),
    JS_CFUNC_DEF("verify", 3, verify),
};

static int js_secp256k1_init(JSContext *ctx, JSModuleDef *m) {
    JS_SetModuleExportList(ctx, m, js_secp256k1_funcs, countof(js_secp256k1_funcs));
    return 0;
}

int js_init_module_secp256k1(JSContext *js_ctx) {
    g_secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    JSModuleDef *m = JS_NewCModule(js_ctx, "secp256k1", js_secp256k1_init);
    if (!m) {
        return -1;
    }
    JS_AddModuleExportList(js_ctx, m, js_secp256k1_funcs, countof(js_secp256k1_funcs));
    return 0;
}
