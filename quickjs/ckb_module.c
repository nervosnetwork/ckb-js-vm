#include <stdint.h>
#include <stdbool.h>
#include "ckb_module.h"
#include "cutils.h"
#include "ckb_syscalls.h"
#include "molecule/blockchain.h"
#include "molecule/molecule_reader.h"

// For syscalls supporting partial loading, the arguments are described as:
// argument 1: index
// argument 2: source
// argument 3: length (optional, default to full length)
// argument 4: offset (optional, default to 0)
//
#define NO_VALUE ((size_t)-1)

enum SyscallErrorCode {
    SyscallErrorUnknown = 80,
    SyscallErrorMemory = 81,
    SyscallErrorArgument = 82,
};

static JSValue syscall_exit(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    int32_t status;
    if (JS_ToInt32(ctx, &status, argv[0])) return JS_EXCEPTION;
    ckb_exit((int8_t)status);
    return JS_UNDEFINED;
}

static void my_free(JSRuntime *rt, void *opaque, void *_ptr) { free(opaque); }
struct LoadData;
typedef int (*LoadFunc)(void *addr, uint64_t *len, struct LoadData *data);

typedef struct LoadData {
    uint64_t length;
    size_t index;
    size_t source;
    size_t offset;
    size_t field;
    LoadFunc func;
} LoadData;

static JSValue parse_args(JSContext *ctx, LoadData *data, bool has_field, int argc, JSValueConst *argv, LoadFunc func) {
    int64_t index;
    int64_t source;
    int64_t length = NO_VALUE;
    int64_t offset = NO_VALUE;
    int64_t field = NO_VALUE;

    if (JS_ToInt64(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    if (JS_ToInt64(ctx, &source, argv[1])) {
        return JS_EXCEPTION;
    }
    int var_arg_index = 2;
    if (has_field) {
        if (argc > 2) {
            if (JS_ToInt64(ctx, &field, argv[2])) {
                return JS_EXCEPTION;
            }
        }
        var_arg_index = 3;
    }
    if (argc > var_arg_index) {
        if (JS_ToInt64(ctx, &length, argv[var_arg_index])) {
            return JS_EXCEPTION;
        }
    }
    if (argc > (var_arg_index + 1)) {
        if (JS_ToInt64(ctx, &offset, argv[var_arg_index + 1])) {
            return JS_EXCEPTION;
        }
    }

    data->func = func;
    data->length = length;
    data->offset = offset;
    data->index = index;
    data->source = source;
    data->field = field;
    return JS_TRUE;
}

static JSValue syscall_load(JSContext *ctx, LoadData *data) {
    int err = 0;
    JSValue ret = JS_EXCEPTION;
    uint8_t *addr = 0;
    // no offset specified, default to 0
    if (data->offset == NO_VALUE) {
        data->offset = 0;
    }
    // get actual length
    if (data->length == 0) {
        err = data->func(0, &data->length, data);
        CHECK(err);
        return JS_NewUint32(ctx, (uint32_t)data->length);
    }
    // no length specified, read to the end
    if (data->length == NO_VALUE) {
        err = data->func(0, &data->length, data);
        CHECK(err);
    }

    addr = (uint8_t *)malloc(data->length);
    CHECK2(addr != NULL, SyscallErrorMemory);
    uint64_t len = data->length;
    err = data->func(addr, &len, data);
    CHECK(err);
    int real_len = len < data->length ? len : data->length;
    ret = JS_NewArrayBuffer(ctx, addr, real_len, my_free, addr, false);
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return ret;
    }
}

static int _load_tx_hash(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_tx_hash(addr, len, data->offset);
}

static JSValue syscall_load_tx_hash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {
        .func = _load_tx_hash,
        .length = 32,
        .offset = 0,
    };
    return syscall_load(ctx, &data);
}

static int _load_transaction(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_transaction(addr, len, data->offset);
}

static JSValue syscall_load_transaction(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, false, argc, argv, _load_transaction);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static int _load_script_hash(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_script_hash(addr, len, data->offset);
}

static JSValue syscall_load_script_hash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {
        .func = _load_script_hash,
        .length = 32,
        .offset = 0,
    };
    return syscall_load(ctx, &data);
}

static int _load_script(void *addr, uint64_t *len, LoadData *data) { return ckb_load_script(addr, len, data->offset); }

static JSValue syscall_load_script(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, false, argc, argv, _load_script);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static JSValue syscall_debug(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    const char *str = JS_ToCString(ctx, argv[0]);
    ckb_debug(str);
    return JS_UNDEFINED;
}

static int _load_cell(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_cell(addr, len, data->offset, data->index, data->source);
}

static JSValue syscall_load_cell(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, false, argc, argv, _load_cell);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static int _load_input(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_input(addr, len, data->offset, data->index, data->source);
}

static JSValue syscall_load_input(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, false, argc, argv, _load_input);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static int _load_header(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_header(addr, len, data->offset, data->index, data->source);
}

static JSValue syscall_load_header(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, false, argc, argv, _load_header);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static int _load_witness(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_witness(addr, len, data->offset, data->index, data->source);
}

static JSValue syscall_load_witness(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, false, argc, argv, _load_witness);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static int _load_cell_data(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_cell_data(addr, len, data->offset, data->index, data->source);
}

static JSValue syscall_load_cell_data(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, false, argc, argv, _load_cell_data);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static int _load_cell_by_field(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_cell_by_field(addr, len, data->offset, data->index, data->source, data->field);
}

static JSValue syscall_load_cell_by_field(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, true, argc, argv, _load_cell_by_field);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static int _load_header_by_field(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_header_by_field(addr, len, data->offset, data->index, data->source, data->field);
}

static JSValue syscall_load_header_by_field(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, true, argc, argv, _load_header_by_field);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static int _load_input_by_field(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_input_by_field(addr, len, data->offset, data->index, data->source, data->field);
}

static JSValue syscall_load_input_by_field(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, true, argc, argv, _load_input_by_field);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static JSValue syscall_vm_version(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int32_t version = ckb_vm_version();
    return JS_NewInt32(ctx, version);
}

static JSValue syscall_current_cycles(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    uint64_t cycles = ckb_current_cycles();
    return JS_NewInt64(ctx, cycles);
}

static JSValue syscall_exec_cell(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    const size_t argv_offset = 4;
    int err = 0;
    size_t code_hash_len = 0;
    uint32_t hash_type = 0;
    uint32_t offset = 0;
    uint32_t length = 0;
    const char *passed_argv[256] = {0};
    uint8_t code_hash[32];

    JSValue buffer = JS_GetTypedArrayBuffer(ctx, argv[0], NULL, NULL, NULL);
    CHECK2(!JS_IsException(buffer), SyscallErrorArgument);
    uint8_t *p = JS_GetArrayBuffer(ctx, &code_hash_len, buffer);
    CHECK2(code_hash_len == 32 && p != NULL, -1);
    memcpy(code_hash, p, 32);

    err = JS_ToUint32(ctx, &hash_type, argv[1]);
    CHECK(err);

    err = JS_ToUint32(ctx, &offset, argv[2]);
    CHECK(err);

    err = JS_ToUint32(ctx, &length, argv[3]);
    CHECK(err);

    for (int i = argv_offset; i < argc; i++) {
        passed_argv[i - argv_offset] = JS_ToCString(ctx, argv[i]);
    }
    ckb_exec_cell(code_hash, (uint8_t)hash_type, offset, length, argc - argv_offset, passed_argv);
    // never reach here
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return JS_UNDEFINED;
    }
}

int read_local_file(char *buf, int size) {
    int ret = syscall(9000, buf, size, 0, 0, 0, 0);
    return ret;
}

static int get_property(JSContext *ctx, JSValueConst *obj, const char *prop, int64_t *value) {
    int err = 0;
    JSValue val = JS_GetPropertyStr(ctx, *obj, prop);
    CHECK2(!JS_IsException(val), SyscallErrorArgument);
    if (!JS_IsUndefined(val)) {
        err = JS_ToInt64(ctx, value, val);
        CHECK(err);
    }
    JS_FreeValue(ctx, val);
exit:
    return err;
}
/*
ckb.spawn_cell
argument 1: code_hash, ArrayBuffer
argument 2: hash_type, int
argument 3: spawn_args, including:
    content_length, optional, int
    memory_limit, optional, int
    offset, optional, int
    length, optional, int
    argument 4~n: arguments
return: object with
    exit_code, int
    content, ArrayBuffer
Example usage:
```javascript
let code_hash = new ArrayBuffer(32);
code_hash[0] = 0x01;
...
...
let hash_type = ckb.SCRIPT_HASH_TYPE_TYPE;
let spawn_args = {
    content_length: 1024,
    memory_limit: 8,
    offset: 0,
    length: 0,
};
let arg1 = "hello";
let arg2 = "world";
let ret = ckb.spawn_cell(code_hash, hash_type, spawn_args, arg1, arg2, arg3);
let exit_code = ret.exit_code;
let content = ret.content;
```
*/
static JSValue syscall_spawn_cell(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int err = 0;
    JSValue ret = JS_UNDEFINED;
    const size_t argv_offset = 3;
    size_t code_hash_len = 0;
    uint32_t hash_type = 0;
    int64_t content_length = 0;
    int64_t memory_limit = 8;  // default value
    int64_t offset = 0;
    int64_t length = 0;
    const char *passed_argv[256] = {0};
    int8_t exit_code = 0;
    uint8_t code_hash[32];

    JSValue buffer = JS_GetTypedArrayBuffer(ctx, argv[0], NULL, NULL, NULL);
    CHECK2(!JS_IsException(buffer), SyscallErrorArgument);
    uint8_t *p = JS_GetArrayBuffer(ctx, &code_hash_len, buffer);
    CHECK2(code_hash_len == 32 && p != NULL, -1);
    memcpy(code_hash, p, 32);

    err = JS_ToUint32(ctx, &hash_type, argv[1]);
    CHECK(err);
    err = get_property(ctx, &argv[2], "content_length", &content_length);
    CHECK(err);
    err = get_property(ctx, &argv[2], "memory_limit", &memory_limit);
    CHECK(err);
    err = get_property(ctx, &argv[2], "offset", &offset);
    CHECK(err);
    err = get_property(ctx, &argv[2], "length", &length);
    CHECK(err);

    for (int i = argv_offset; i < argc; i++) {
        passed_argv[i - argv_offset] = JS_ToCString(ctx, argv[i]);
    }
    void *content = malloc(content_length);
    CHECK2(content != NULL, SyscallErrorArgument);
    spawn_args_t spgs = {
        .memory_limit = memory_limit,
        .exit_code = &exit_code,
        .content = content,
        .content_length = (uint64_t *)&content_length,
    };
    err = ckb_spawn_cell(code_hash, (uint8_t)hash_type, offset, length, argc - argv_offset, passed_argv, &spgs);
    CHECK(err);
    ret = JS_NewObject(ctx);
    CHECK2(!JS_IsException(ret), SyscallErrorMemory);
    JS_DefinePropertyValueStr(ctx, ret, "exit_code", JS_NewUint32(ctx, (uint32_t)exit_code), JS_PROP_C_W_E);
    JSValue content2 = JS_NewArrayBuffer(ctx, spgs.content, *spgs.content_length, my_free, spgs.content, false);
    JS_DefinePropertyValueStr(ctx, ret, "content", content2, JS_PROP_C_W_E);
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return ret;
    }
}

static JSValue syscall_set_content(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int err = 0;
    size_t content_length = 0;
    JSValue buffer = JS_GetTypedArrayBuffer(ctx, argv[0], NULL, NULL, NULL);
    CHECK2(!JS_IsException(buffer), SyscallErrorArgument);
    uint8_t *content = JS_GetArrayBuffer(ctx, &content_length, buffer);
    CHECK2(content != NULL, SyscallErrorUnknown);
    uint64_t content_length2 = (uint64_t)content_length;
    err = ckb_set_content(content, &content_length2);
    CHECK(err);
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return JS_UNDEFINED;
    }
}

static JSValue syscall_get_memory_limit(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int memory_limit = ckb_get_memory_limit();
    return JS_NewUint32(ctx, (uint32_t)memory_limit);
}

static JSValue syscall_current_memory(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int size = ckb_current_memory();
    return JS_NewUint32(ctx, (uint32_t)size);
}

/*
TODO:
// who allocated the memory indicated by aligned_addr?
int ckb_dlopen2(const uint8_t* dep_cell_hash, uint8_t hash_type,
                uint8_t* aligned_addr, size_t aligned_size, void** handle,
                size_t* consumed_size);
void* ckb_dlsym(void* handle, const char* symbol);
*/
int js_init_module_ckb(JSContext *ctx) {
    JSValue global_obj, ckb;
    global_obj = JS_GetGlobalObject(ctx);
    ckb = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, ckb, "exit", JS_NewCFunction(ctx, syscall_exit, "exit", 1));
    JS_SetPropertyStr(ctx, ckb, "load_tx_hash", JS_NewCFunction(ctx, syscall_load_tx_hash, "load_tx_hash", 1));
    JS_SetPropertyStr(ctx, ckb, "load_transaction",
                      JS_NewCFunction(ctx, syscall_load_transaction, "load_transaction", 1));
    JS_SetPropertyStr(ctx, ckb, "load_script_hash",
                      JS_NewCFunction(ctx, syscall_load_script_hash, "load_script_hash", 1));
    JS_SetPropertyStr(ctx, ckb, "load_script", JS_NewCFunction(ctx, syscall_load_script, "load_script", 1));
    JS_SetPropertyStr(ctx, ckb, "debug", JS_NewCFunction(ctx, syscall_debug, "debug", 1));
    JS_SetPropertyStr(ctx, ckb, "load_cell", JS_NewCFunction(ctx, syscall_load_cell, "load_cell", 3));
    JS_SetPropertyStr(ctx, ckb, "load_input", JS_NewCFunction(ctx, syscall_load_input, "load_input", 3));
    JS_SetPropertyStr(ctx, ckb, "load_header", JS_NewCFunction(ctx, syscall_load_header, "load_header", 3));
    JS_SetPropertyStr(ctx, ckb, "load_witness", JS_NewCFunction(ctx, syscall_load_witness, "load_witness", 3));
    JS_SetPropertyStr(ctx, ckb, "load_cell_data", JS_NewCFunction(ctx, syscall_load_cell_data, "load_cell_data", 3));
    JS_SetPropertyStr(ctx, ckb, "load_cell_by_field",
                      JS_NewCFunction(ctx, syscall_load_cell_by_field, "load_cell_by_field", 4));
    JS_SetPropertyStr(ctx, ckb, "load_header_by_field",
                      JS_NewCFunction(ctx, syscall_load_header_by_field, "load_header_by_field", 4));
    JS_SetPropertyStr(ctx, ckb, "load_input_by_field",
                      JS_NewCFunction(ctx, syscall_load_input_by_field, "load_input_by_field", 4));
    JS_SetPropertyStr(ctx, ckb, "vm_version", JS_NewCFunction(ctx, syscall_vm_version, "vm_version", 0));
    JS_SetPropertyStr(ctx, ckb, "current_cycles", JS_NewCFunction(ctx, syscall_current_cycles, "current_cycles", 0));
    JS_SetPropertyStr(ctx, ckb, "exec_cell", JS_NewCFunction(ctx, syscall_exec_cell, "exec_cell", 4));
    JS_SetPropertyStr(ctx, ckb, "spawn_cell", JS_NewCFunction(ctx, syscall_spawn_cell, "spawn_cell", 3));
    JS_SetPropertyStr(ctx, ckb, "set_content", JS_NewCFunction(ctx, syscall_set_content, "set_content", 1));
    JS_SetPropertyStr(ctx, ckb, "get_memory_limit",
                      JS_NewCFunction(ctx, syscall_get_memory_limit, "get_memory_limit", 0));
    JS_SetPropertyStr(ctx, ckb, "current_memory",
                      JS_NewCFunction(ctx, syscall_current_memory, "current_memory", 0));
    JS_SetPropertyStr(ctx, ckb, "SOURCE_INPUT", JS_NewInt64(ctx, CKB_SOURCE_INPUT));
    JS_SetPropertyStr(ctx, ckb, "SOURCE_OUTPUT", JS_NewInt64(ctx, CKB_SOURCE_OUTPUT));
    JS_SetPropertyStr(ctx, ckb, "SOURCE_CELL_DEP", JS_NewInt64(ctx, CKB_SOURCE_CELL_DEP));
    JS_SetPropertyStr(ctx, ckb, "SOURCE_HEADER_DEP", JS_NewInt64(ctx, CKB_SOURCE_HEADER_DEP));
    // Should use bigint. If Int64 is used, when it's too big(> 0xFFFFFFFF), it is stored as float number.
    JS_SetPropertyStr(ctx, ckb, "SOURCE_GROUP_INPUT", JS_NewBigUint64(ctx, CKB_SOURCE_GROUP_INPUT));
    JS_SetPropertyStr(ctx, ckb, "SOURCE_GROUP_OUTPUT", JS_NewBigUint64(ctx, CKB_SOURCE_GROUP_OUTPUT));

    JS_SetPropertyStr(ctx, ckb, "CELL_FIELD_CAPACITY", JS_NewInt64(ctx, CKB_CELL_FIELD_CAPACITY));
    JS_SetPropertyStr(ctx, ckb, "CELL_FIELD_DATA_HASH", JS_NewInt64(ctx, CKB_CELL_FIELD_DATA_HASH));
    JS_SetPropertyStr(ctx, ckb, "CELL_FIELD_LOCK", JS_NewInt64(ctx, CKB_CELL_FIELD_LOCK));
    JS_SetPropertyStr(ctx, ckb, "CELL_FIELD_LOCK_HASH", JS_NewInt64(ctx, CKB_CELL_FIELD_LOCK_HASH));
    JS_SetPropertyStr(ctx, ckb, "CELL_FIELD_TYPE", JS_NewInt64(ctx, CKB_CELL_FIELD_TYPE));
    JS_SetPropertyStr(ctx, ckb, "CELL_FIELD_TYPE_HASH", JS_NewInt64(ctx, CKB_CELL_FIELD_TYPE_HASH));
    JS_SetPropertyStr(ctx, ckb, "CELL_FIELD_OCCUPIED_CAPACITY", JS_NewInt64(ctx, CKB_CELL_FIELD_OCCUPIED_CAPACITY));

    JS_SetPropertyStr(ctx, ckb, "HEADER_FIELD_EPOCH_NUMBER", JS_NewInt64(ctx, CKB_HEADER_FIELD_EPOCH_NUMBER));
    JS_SetPropertyStr(ctx, ckb, "HEADER_FIELD_EPOCH_START_BLOCK_NUMBER",
                      JS_NewInt64(ctx, CKB_HEADER_FIELD_EPOCH_START_BLOCK_NUMBER));
    JS_SetPropertyStr(ctx, ckb, "HEADER_FIELD_EPOCH_LENGTH", JS_NewInt64(ctx, CKB_HEADER_FIELD_EPOCH_LENGTH));
    JS_SetPropertyStr(ctx, ckb, "INPUT_FIELD_OUT_POINT", JS_NewInt64(ctx, CKB_INPUT_FIELD_OUT_POINT));
    JS_SetPropertyStr(ctx, ckb, "INPUT_FIELD_SINCE", JS_NewInt64(ctx, CKB_INPUT_FIELD_SINCE));

    // https://github.com/nervosnetwork/ckb/blob/45104e08c18852260282f23858ce21afae4c9f34/util/gen-types/src/core.rs#L14
    JS_SetPropertyStr(ctx, ckb, "SCRIPT_HASH_TYPE_DATA", JS_NewInt64(ctx, 0));
    JS_SetPropertyStr(ctx, ckb, "SCRIPT_HASH_TYPE_TYPE", JS_NewInt64(ctx, 1));
    JS_SetPropertyStr(ctx, ckb, "SCRIPT_HASH_TYPE_DATA1", JS_NewInt64(ctx, 2));
    JS_SetPropertyStr(ctx, ckb, "SCRIPT_HASH_TYPE_DATA2", JS_NewInt64(ctx, 4));

    JS_SetPropertyStr(ctx, global_obj, "ckb", ckb);
    JS_FreeValue(ctx, global_obj);
    return 0;
}

#define SCRIPT_SIZE 32768
#define JS_LOADER_ARGS_SIZE 2
#define BLAKE2B_BLOCK_SIZE 32

int load_cell_code_info(size_t *buf_size, size_t *index) {
    int err = 0;
    unsigned char script[SCRIPT_SIZE];
    uint64_t len = SCRIPT_SIZE;
    err = ckb_load_script(script, &len, 0);
    CHECK(err);
    CHECK2(len <= SCRIPT_SIZE, -1);
    mol_seg_t script_seg;
    script_seg.ptr = (uint8_t *)script;
    script_seg.size = len;

    if (MolReader_Script_verify(&script_seg, false) != MOL_OK) {
        return -1;
    }

    // The script arguments are in the following format
    // <lua loader args, 2 bytes> <data, variable length>
    mol_seg_t args_seg = MolReader_Script_get_args(&script_seg);
    mol_seg_t args_bytes_seg = MolReader_Bytes_raw_bytes(&args_seg);
    CHECK2(args_bytes_seg.size >= JS_LOADER_ARGS_SIZE, -1);

    // Loading lua code from dependent cell with code hash and hash type
    // The script arguments are in the following format
    // <lua loader args, 2 bytes> <code hash of lua code, 32 bytes>
    // <hash type of lua code, 1 byte>
    CHECK2(args_bytes_seg.size >= JS_LOADER_ARGS_SIZE + BLAKE2B_BLOCK_SIZE + 1, -1);

    uint8_t *code_hash = args_bytes_seg.ptr + JS_LOADER_ARGS_SIZE;
    uint8_t hash_type = *(args_bytes_seg.ptr + JS_LOADER_ARGS_SIZE + BLAKE2B_BLOCK_SIZE);

    *index = 0;
    err = ckb_look_for_dep_with_hash2(code_hash, hash_type, index);
    CHECK(err);

    *buf_size = 0;
    err = ckb_load_cell_data(NULL, buf_size, 0, *index, CKB_SOURCE_CELL_DEP);
    CHECK(err);
    CHECK2(*buf_size > 0, -1);
exit:
    return err;
}

int load_cell_code(size_t buf_size, size_t index, uint8_t *buf) {
    int ret = ckb_load_cell_data(buf, &buf_size, 0, index, CKB_SOURCE_CELL_DEP);
    if (ret) {
        printf("Error while loading cell data: %d\n", ret);
        return -1;
    }
    return 0;
}
