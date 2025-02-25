#include <stdbool.h>
#include "cutils.h"
#include "secp256k1_module.h"
#include "secp256k1.h"
#include "secp256k1_recovery.h"
#include "secp256k1_schnorrsig.h"
#include "secp256k1_extrakeys.h"
#include "group.h"
#undef CHECK
#undef CHECK2
#include "qjs.h"
#include "utils.h"

#define COMPRESSED_PUBKEY_LENGTH 33    // 1 byte prefix + 32 bytes x coordinate
#define UNCOMPRESSED_PUBKEY_LENGTH 65  // 1 byte prefix + 32 bytes x + 32 bytes y

// from the secp256k1 library. The original table is used for signing
// operations and public key generation, and consumes significant memory.
// Since we only need verification functionality in our on-chain scripts
// (not signing or key generation), we can safely use this minimal
// placeholder to reduce memory usage.
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
    if (qjs_bad_int_arg(ctx, argv[1], 1)) {
        return JS_EXCEPTION;
    }
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

static const JSCFunctionListEntry secp256k1_obj_funcs[] = {
    JS_CFUNC_DEF("recover", 3, recover),
    JS_CFUNC_DEF("serializePubkey", 2, serialize_pubkey),
    JS_CFUNC_DEF("parsePubkey", 1, parse_pubkey),
    JS_CFUNC_DEF("verify", 3, verify),
};

static JSValue schnorr_xonly_serialize_pubkey(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t pubkey_len;
    uint8_t *pubkey_data;
    secp256k1_xonly_pubkey pubkey;

    // Check argument count
    if (argc != 1) return JS_ThrowTypeError(ctx, "wrong number of arguments");

    // Get public key from argument
    pubkey_data = JS_GetArrayBuffer(ctx, &pubkey_len, argv[0]);
    if (!pubkey_data || pubkey_len != sizeof(secp256k1_xonly_pubkey)) {
        return JS_ThrowTypeError(ctx, "invalid x-only public key format");
    }

    // Copy the public key data
    memcpy(&pubkey, pubkey_data, sizeof(secp256k1_xonly_pubkey));

    // Serialize the x-only public key (32 bytes)
    uint8_t *output = js_malloc(ctx, 32);
    if (!output) {
        return JS_ThrowOutOfMemory(ctx);
    }

    if (!secp256k1_xonly_pubkey_serialize(g_secp256k1_context, output, &pubkey)) {
        js_free(ctx, output);
        return JS_ThrowInternalError(ctx, "serialization failed");
    }

    return JS_NewArrayBuffer(ctx, output, 32, free_array_buffer, ctx, false);
}

static JSValue schnorr_tagged_sha256(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t tag_len, msg_len;
    uint8_t *tag, *msg;

    // Check argument count
    if (argc != 2) return JS_ThrowTypeError(ctx, "wrong number of arguments");

    // Get tag from first argument
    tag = JS_GetArrayBuffer(ctx, &tag_len, argv[0]);
    if (!tag) return JS_ThrowTypeError(ctx, "invalid tag format");

    // Get message from second argument
    msg = JS_GetArrayBuffer(ctx, &msg_len, argv[1]);
    if (!msg) return JS_ThrowTypeError(ctx, "invalid message format");

    // Allocate output buffer for hash (32 bytes)
    uint8_t *output = js_malloc(ctx, 32);
    if (!output) {
        return JS_ThrowOutOfMemory(ctx);
    }

    (void)secp256k1_tagged_sha256(g_secp256k1_context, output, tag, tag_len, msg, msg_len);

    return JS_NewArrayBuffer(ctx, output, 32, free_array_buffer, ctx, false);
}

static JSValue schnorr_xonly_parse_pubkey(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t input_len;
    uint8_t *input;
    secp256k1_xonly_pubkey pubkey;

    // Check argument count
    if (argc != 1) return JS_ThrowTypeError(ctx, "wrong number of arguments");

    // Get serialized x-only public key from argument
    input = JS_GetArrayBuffer(ctx, &input_len, argv[0]);
    if (!input || input_len != 32) {
        return JS_ThrowTypeError(ctx, "invalid x-only public key format (must be 32 bytes)");
    }

    // Parse the x-only public key
    if (!secp256k1_xonly_pubkey_parse(g_secp256k1_context, &pubkey, input)) {
        return JS_ThrowTypeError(ctx, "invalid x-only public key");
    }

    // Return the parsed public key
    return JS_NewArrayBufferCopy(ctx, pubkey.data, sizeof(pubkey));
}

static JSValue schnorr_verify(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    size_t msg_len, sig_len, pubkey_len;
    uint8_t *msg = NULL, *sig = NULL, *pubkey_data = NULL;
    secp256k1_xonly_pubkey pubkey;

    // Check argument count
    if (argc != 3) return JS_ThrowTypeError(ctx, "wrong number of arguments");

    // Get signature from first argument
    sig = JS_GetArrayBuffer(ctx, &sig_len, argv[0]);
    if (!sig || sig_len != 64) return JS_ThrowTypeError(ctx, "invalid signature format");

    // Get message from second argument
    msg = JS_GetArrayBuffer(ctx, &msg_len, argv[1]);
    if (!msg || msg_len != 32) return JS_ThrowTypeError(ctx, "message must be 32 bytes");

    // Get public key from third argument
    pubkey_data = JS_GetArrayBuffer(ctx, &pubkey_len, argv[2]);
    if (!pubkey_data || pubkey_len != sizeof(secp256k1_xonly_pubkey)) {
        return JS_ThrowTypeError(ctx, "invalid x-only public key format");
    }

    // Copy the public key data
    memcpy(&pubkey, pubkey_data, sizeof(secp256k1_xonly_pubkey));

    // Perform the verification
    int result = secp256k1_schnorrsig_verify(g_secp256k1_context, sig, msg, msg_len, &pubkey);

    return JS_NewBool(ctx, result == 1);
}

static const JSCFunctionListEntry schnorr_obj_funcs[] = {
    JS_CFUNC_DEF("xonlySerializePubkey", 1, schnorr_xonly_serialize_pubkey),
    JS_CFUNC_DEF("taggedSha256", 2, schnorr_tagged_sha256),
    JS_CFUNC_DEF("xonlyParsePubkey", 1, schnorr_xonly_parse_pubkey),
    JS_CFUNC_DEF("verify", 3, schnorr_verify),
};

int qjs_init_module_secp256k1_lazy(JSContext *ctx, JSModuleDef *m) {
    JSValue secp256k1_obj, schnorr_obj;

    secp256k1_obj = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, secp256k1_obj, secp256k1_obj_funcs, countof(secp256k1_obj_funcs));
    JS_SetModuleExport(ctx, m, "secp256k1", secp256k1_obj);

    schnorr_obj = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, schnorr_obj, schnorr_obj_funcs, countof(schnorr_obj_funcs));
    JS_SetModuleExport(ctx, m, "schnorr", schnorr_obj);

    return 0;
}

int qjs_init_module_secp256k1(JSContext *js_ctx, JSModuleDef *m) {
    g_secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    JS_AddModuleExport(js_ctx, m, "secp256k1");
    JS_AddModuleExport(js_ctx, m, "schnorr");
    return 0;
}
