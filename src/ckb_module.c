#include <stdint.h>
#include <stdbool.h>
#include "ckb_module.h"
#include "cutils.h"
#include "ckb_syscalls.h"
#include "molecule/blockchain.h"
#include "molecule/molecule_reader.h"
#include "ckb_cell_fs.h"
#include "qjs.h"

// For syscalls supporting partial loading, the arguments are described as:
// argument 1: index
// argument 2: source
// argument 3: length (optional, default to full length)
// argument 4: offset (optional, default to 0)
//
#define NO_VALUE ((size_t) - 1)

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

static JSValue ThrowError(JSContext *ctx, int32_t error_code, const char *message) {
    JSValue obj, ret;
    obj = JS_NewError(ctx);
    if (unlikely(JS_IsException(obj))) {
        obj = JS_NULL;
    } else {
        JS_DefinePropertyValueStr(ctx, obj, "message", JS_NewString(ctx, message),
                                  JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
        JS_DefinePropertyValueStr(ctx, obj, "errorCode", JS_NewInt32(ctx, error_code),
                                  JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
    }
    // Syscall errors are expected to occur frequently during normal operation.
    // We intentionally skip generating stack traces here to avoid performance overhead
    ret = JS_Throw(ctx, obj);
    return ret;
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
    if (JS_ToBigInt64(ctx, &source, argv[1])) {
        if (JS_ToInt64(ctx, &source, argv[1])) return JS_EXCEPTION;
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
        data->length = 0;
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
        ThrowError(ctx, err, "ckb syscall error");
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

static JSValue syscall_spawn_cell(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int err = 0;
    size_t code_hash_len = 0;
    uint8_t code_hash[32];
    uint32_t hash_type = 0;
    uint32_t offset = 0;
    uint32_t length = 0;
    uint32_t spgs_argc = 0;
    const char *spgs_argv[32] = {};
    uint64_t spgs_pid = 0;
    uint64_t spgs_fds[32] = {0};
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
    JSValue val;
    val = JS_GetPropertyStr(ctx, argv[4], "argv");
    CHECK2(!JS_IsException(val), SyscallErrorArgument);
    if (!JS_IsUndefined(val)) {
        for (int i = 0; i < 32; i++) {
            const JSValue elem = JS_GetPropertyUint32(ctx, val, i);
            if (JS_IsUndefined(elem)) {
                break;
            }
            const char *str = JS_ToCString(ctx, elem);
            spgs_argc += 1;
            spgs_argv[i] = str;
            JS_FreeValue(ctx, elem);
        }
    }
    JS_FreeValue(ctx, val);
    val = JS_GetPropertyStr(ctx, argv[4], "inherited_fds");
    CHECK2(!JS_IsException(val), SyscallErrorArgument);
    if (!JS_IsUndefined(val)) {
        uint32_t temp;
        for (int i = 0; i < 32; i++) {
            const JSValue elem = JS_GetPropertyUint32(ctx, val, i);
            if (JS_IsUndefined(elem)) {
                break;
            }
            err = JS_ToUint32(ctx, &temp, elem);
            CHECK(err);
            spgs_fds[i] = temp;
            JS_FreeValue(ctx, elem);
        }
    }
    JS_FreeValue(ctx, val);
    spawn_args_t spgs = {
        .argc = spgs_argc,
        .argv = spgs_argv,
        .process_id = &spgs_pid,
        .inherited_fds = &spgs_fds[0],
    };
    err = ckb_spawn_cell(code_hash, hash_type, offset, length, &spgs);
    CHECK(err);
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return JS_NewInt64(ctx, spgs_pid);
    }
}
static JSValue syscall_pipe(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int err = 0;
    uint64_t fds[2];
    err = ckb_pipe(fds);
    CHECK(err);
    JSValue obj = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, obj, 0, JS_NewUint32(ctx, fds[0]));
    JS_SetPropertyUint32(ctx, obj, 1, JS_NewUint32(ctx, fds[1]));
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return obj;
    }
}
static JSValue syscall_inherited_fds(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int err = 0;
    uint64_t fds[64];
    uint64_t length = 64;
    err = ckb_inherited_fds(fds, &length);
    CHECK(err);
    JSValue obj = JS_NewArray(ctx);
    for (int i = 0; i < length; i++) {
        JS_SetPropertyUint32(ctx, obj, i, JS_NewUint32(ctx, (uint32_t)fds[i]));
    }
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return obj;
    }
}
static JSValue syscall_read(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int err = 0;
    uint64_t fd = 0;
    size_t length = 0;
    uint32_t u32 = 0;
    err = JS_ToUint32(ctx, &u32, argv[0]);
    CHECK(err);
    fd = u32;
    err = JS_ToUint32(ctx, &u32, argv[1]);
    CHECK(err);
    length = u32;
    uint8_t *buffer = (uint8_t *)malloc(length);
    err = ckb_read(fd, buffer, &length);
    CHECK(err);
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return JS_NewArrayBuffer(ctx, buffer, length, my_free, buffer, false);
    }
}
static JSValue syscall_write(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int err = 0;
    uint64_t fd = 0;
    uint32_t u32 = 0;
    err = JS_ToUint32(ctx, &u32, argv[0]);
    CHECK(err);
    fd = (uint64_t)u32;
    size_t length = 0;
    JSValue buffer = JS_GetTypedArrayBuffer(ctx, argv[1], NULL, NULL, NULL);
    CHECK2(!JS_IsException(buffer), SyscallErrorArgument);
    uint8_t *content = JS_GetArrayBuffer(ctx, &length, buffer);
    CHECK2(content != NULL, SyscallErrorUnknown);
    err = ckb_write(fd, content, &length);
    CHECK(err);
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return JS_UNDEFINED;
    }
}
static JSValue syscall_close(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int err = 0;
    uint32_t fd = 0;
    err = JS_ToUint32(ctx, &fd, argv[0]);
    CHECK(err);
    err = ckb_close((uint64_t)fd);
    CHECK(err);
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return JS_UNDEFINED;
    }
}
static JSValue syscall_wait(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    int err = 0;
    uint32_t pid = 0;
    int8_t exit = 0;
    err = JS_ToUint32(ctx, &pid, argv[0]);
    CHECK(err);
    err = ckb_wait((uint64_t)pid, &exit);
    CHECK(err);
exit:
    if (err != 0) {
        return JS_EXCEPTION;
    } else {
        return JS_NewInt32(ctx, exit);
    }
}
static JSValue syscall_process_id(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    uint64_t pid = ckb_process_id();
    return JS_NewUint32(ctx, (uint32_t)pid);
}
static int _load_block_extension(void *addr, uint64_t *len, LoadData *data) {
    return ckb_load_block_extension(addr, len, data->offset, data->index, data->source);
}
static JSValue syscall_load_block_extension(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    LoadData data = {0};
    JSValue ret = parse_args(ctx, &data, false, argc, argv, _load_block_extension);
    if (JS_IsException(ret)) {
        return ret;
    }
    return syscall_load(ctx, &data);
}

static JSValue mount(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv) {
    JSValue buf = syscall_load_cell_data(ctx, this_value, argc-1, argv);
    if (JS_IsException(buf)) {
        return JS_EXCEPTION;
    }
    const char* prefix = JS_ToCString(ctx, argv[2]);
    size_t psize = 0;
    uint8_t *addr = JS_GetArrayBuffer(ctx, &psize, buf);
    int err = ckb_load_fs(prefix, addr, psize);
    if (err != 0) {
        ThrowError(ctx, SyscallErrorUnknown, "ckb_load_fs failed");
        return JS_EXCEPTION;
    } else {
        return JS_UNDEFINED;
    }
}

JSValue eval_script(JSContext *ctx, const char *str, int len, bool enable_module) {
    int eval_flags = enable_module ? JS_EVAL_TYPE_MODULE : (JS_EVAL_FLAG_ASYNC | JS_EVAL_TYPE_GLOBAL);

    JSValue val = JS_Eval(ctx, str, len, "<evalScript>", eval_flags);

    if (JS_IsException(val)) {
        JS_Throw(ctx, val);
        return JS_EXCEPTION;
    }

    // Handle promise states
    int promise_state = JS_PromiseState(ctx, val);
    if (promise_state >= 0) {
        switch (promise_state) {
            case JS_PROMISE_REJECTED: {
                JSValue error = JS_PromiseResult(ctx, val);
                JS_FreeValue(ctx, val);
                JS_Throw(ctx, error);
                val = JS_EXCEPTION;
                break;
            }
            case JS_PROMISE_FULFILLED: {
                JSValue ret = JS_PromiseResult(ctx, val);
                JS_FreeValue(ctx, val);
                val = ret;
                break;
            }
            case JS_PROMISE_PENDING: {
                JS_FreeValue(ctx, val);
                val = JS_ThrowInternalError(ctx, "invalid promise state in evalScript: pending");
                break;
            }
            default: {
                JS_FreeValue(ctx, val);
                printf("unknown promise state: %d", promise_state);
                val = JS_ThrowInternalError(ctx, "unknown promise state in evalScript");
                break;
            }
        }
    }

    // Non-promise result
    return val;
}

static JSValue js_eval_script(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *str;
    size_t len;
    JSValue ret;
    bool enable_module = false;

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "evalScript requires at least 1 argument");
    }

    // Get enable_module flag if provided
    if (argc > 1) {
        enable_module = JS_ToBool(ctx, argv[1]);
    }

    str = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!str) return JS_EXCEPTION;

    ret = eval_script(ctx, str, len, enable_module);
    JS_FreeCString(ctx, str);
    return ret;
}

static JSValue js_load_file(JSContext *ctx, JSValueConst _this_val, int argc, JSValueConst *argv) {
    uint8_t *buf = NULL;
    const char *filename = NULL;
    JSValue ret = JS_EXCEPTION;
    size_t buf_len = 0;
    int err = 0;

    filename = JS_ToCString(ctx, argv[0]);
    CHECK2(filename != NULL, -1);

    size_t index = 0;
    bool use_filesystem = false;
    err = load_cell_code_info(&buf_len, &index, &use_filesystem);
    CHECK(err);

    buf = js_malloc(ctx, buf_len + 1);
    CHECK2(buf != NULL, 0);

    err = load_cell_code(buf_len, index, buf);
    CHECK(err);

    if (use_filesystem) {
        // don't need to load file system for a single file.
        // it should be mounted or initialized before.
        FSFile *file_handler = NULL;
        err = ckb_get_file(filename, &file_handler);
        CHECK(err);
        ret = JS_NewStringLen(ctx, file_handler->content, file_handler->size);
    } else {
        ret = JS_ThrowInternalError(ctx, "loadFile fail: filesystem is disabled.");
    }

exit:
    if (buf) {
        js_free(ctx, buf);
    }
    if (filename) {
        JS_FreeCString(ctx, filename);
    }
    if (err) {
        return JS_EXCEPTION;
    } else {
        return ret;
    }
}

static JSValue js_load_script(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    bool enable_module = false;

    // Get enable_module flag if provided
    if (argc > 1) {
        enable_module = JS_ToBool(ctx, argv[1]);
    }

    JSValue ret = js_load_file(ctx, this_val, argc, argv);
    if (JS_IsException(ret)) return ret;

    size_t len = 0;
    const char *str = JS_ToCStringLen(ctx, &len, ret);
    JS_FreeValue(ctx, ret);
    ret = eval_script(ctx, str, len, enable_module);
    JS_FreeCString(ctx, str);
    return ret;
}

static JSValue js_parse_ext_json(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    JSValue obj;
    const char *str;
    size_t len;

    str = JS_ToCStringLen(ctx, &len, argv[0]);
    if (!str) return JS_EXCEPTION;
    obj = JS_ParseJSON2(ctx, str, len, "<input>", JS_PARSE_JSON_EXT);
    JS_FreeCString(ctx, str);
    return obj;
}

static const JSCFunctionListEntry js_ckb_funcs[] = {
    JS_CFUNC_DEF("exit", 1, syscall_exit),
    JS_CFUNC_DEF("load_tx_hash", 1, syscall_load_tx_hash),
    JS_CFUNC_DEF("load_transaction", 1, syscall_load_transaction),
    JS_CFUNC_DEF("load_script_hash", 1, syscall_load_script_hash),
    JS_CFUNC_DEF("load_script", 1, syscall_load_script),
    JS_CFUNC_DEF("debug", 1, syscall_debug),
    JS_CFUNC_DEF("load_cell", 3, syscall_load_cell),
    JS_CFUNC_DEF("load_input", 3, syscall_load_input),
    JS_CFUNC_DEF("load_header", 3, syscall_load_header),
    JS_CFUNC_DEF("load_witness", 3, syscall_load_witness),
    JS_CFUNC_DEF("load_cell_data", 3, syscall_load_cell_data),
    JS_CFUNC_DEF("load_cell_by_field", 4, syscall_load_cell_by_field),
    JS_CFUNC_DEF("load_header_by_field", 4, syscall_load_header_by_field),
    JS_CFUNC_DEF("load_input_by_field", 4, syscall_load_input_by_field),
    JS_CFUNC_DEF("vm_version", 0, syscall_vm_version),
    JS_CFUNC_DEF("current_cycles", 0, syscall_current_cycles),
    JS_CFUNC_DEF("exec_cell", 4, syscall_exec_cell),
    JS_CFUNC_DEF("spawn_cell", 5, syscall_spawn_cell),
    JS_CFUNC_DEF("pipe", 0, syscall_pipe),
    JS_CFUNC_DEF("inherited_fds", 0, syscall_inherited_fds),
    JS_CFUNC_DEF("read", 2, syscall_read),
    JS_CFUNC_DEF("write", 2, syscall_write),
    JS_CFUNC_DEF("close", 1, syscall_close),
    JS_CFUNC_DEF("wait", 1, syscall_wait),
    JS_CFUNC_DEF("process_id", 0, syscall_process_id),
    JS_CFUNC_DEF("load_block_extension", 3, syscall_load_block_extension),
    JS_CFUNC_DEF("mount", 2, mount),
    JS_CFUNC_DEF("evalScript", 2, js_eval_script),
    JS_CFUNC_DEF("loadScript", 2, js_load_script),
    JS_CFUNC_DEF("loadFile", 1, js_load_file),
    JS_CFUNC_DEF("parseExtJSON", 1, js_parse_ext_json),

    // Constants
    JS_PROP_INT64_DEF("SOURCE_INPUT", CKB_SOURCE_INPUT, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("SOURCE_OUTPUT", CKB_SOURCE_OUTPUT, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("SOURCE_CELL_DEP", CKB_SOURCE_CELL_DEP, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("SOURCE_HEADER_DEP", CKB_SOURCE_HEADER_DEP, JS_PROP_ENUMERABLE),

    JS_PROP_INT64_DEF("CELL_FIELD_CAPACITY", CKB_CELL_FIELD_CAPACITY, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("CELL_FIELD_DATA_HASH", CKB_CELL_FIELD_DATA_HASH, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("CELL_FIELD_LOCK", CKB_CELL_FIELD_LOCK, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("CELL_FIELD_LOCK_HASH", CKB_CELL_FIELD_LOCK_HASH, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("CELL_FIELD_TYPE", CKB_CELL_FIELD_TYPE, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("CELL_FIELD_TYPE_HASH", CKB_CELL_FIELD_TYPE_HASH, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("CELL_FIELD_OCCUPIED_CAPACITY", CKB_CELL_FIELD_OCCUPIED_CAPACITY, JS_PROP_ENUMERABLE),

    JS_PROP_INT64_DEF("HEADER_FIELD_EPOCH_NUMBER", CKB_HEADER_FIELD_EPOCH_NUMBER, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("HEADER_FIELD_EPOCH_START_BLOCK_NUMBER", CKB_HEADER_FIELD_EPOCH_START_BLOCK_NUMBER,
                      JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("HEADER_FIELD_EPOCH_LENGTH", CKB_HEADER_FIELD_EPOCH_LENGTH, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("INPUT_FIELD_OUT_POINT", CKB_INPUT_FIELD_OUT_POINT, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("INPUT_FIELD_SINCE", CKB_INPUT_FIELD_SINCE, JS_PROP_ENUMERABLE),

    JS_PROP_INT64_DEF("SCRIPT_HASH_TYPE_DATA", 0, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("SCRIPT_HASH_TYPE_TYPE", 1, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("SCRIPT_HASH_TYPE_DATA1", 2, JS_PROP_ENUMERABLE),
    JS_PROP_INT64_DEF("SCRIPT_HASH_TYPE_DATA2", 4, JS_PROP_ENUMERABLE),
};

static int js_ckb_init(JSContext *ctx, JSModuleDef *m) {
    JS_SetModuleExportList(ctx, m, js_ckb_funcs, countof(js_ckb_funcs));
    JS_SetModuleExport(ctx, m, "SOURCE_GROUP_INPUT", JS_NewBigUint64(ctx, CKB_SOURCE_GROUP_INPUT));
    JS_SetModuleExport(ctx, m, "SOURCE_GROUP_OUTPUT", JS_NewBigUint64(ctx, CKB_SOURCE_GROUP_OUTPUT));
    return 0;
}

int js_init_module_ckb(JSContext *ctx) {
    JSModuleDef *m = JS_NewCModule(ctx, "ckb", js_ckb_init);
    if (!m) {
        return -1;
    }
    JS_AddModuleExportList(ctx, m, js_ckb_funcs, countof(js_ckb_funcs));
    JS_AddModuleExport(ctx, m, "SOURCE_GROUP_INPUT");
    JS_AddModuleExport(ctx, m, "SOURCE_GROUP_OUTPUT");
    return 0;
}

#define SCRIPT_SIZE 32768
#define JS_LOADER_ARGS_SIZE 2
#define BLAKE2B_BLOCK_SIZE 32

int load_cell_code_info_explicit(size_t *buf_size, size_t *index, const uint8_t *code_hash, uint8_t hash_type) {
    int err = 0;
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

int load_cell_code_info(size_t *buf_size, size_t *index, bool *use_filesystem) {
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
    // <js loader args, 2 bytes> <data, variable length>
    mol_seg_t args_seg = MolReader_Script_get_args(&script_seg);
    mol_seg_t args_bytes_seg = MolReader_Bytes_raw_bytes(&args_seg);
    CHECK2(args_bytes_seg.size >= JS_LOADER_ARGS_SIZE, -1);

    // Loading js code from dependent cell with code hash and hash type
    // The script arguments are in the following format
    // <js loader args, 2 bytes> <code hash of js code, 32 bytes>
    // <hash type of js code, 1 byte>
    CHECK2(args_bytes_seg.size >= JS_LOADER_ARGS_SIZE + BLAKE2B_BLOCK_SIZE + 1, -1);
    uint16_t js_loader_args = *(uint16_t *)args_bytes_seg.ptr;
    *use_filesystem = js_loader_args & 0x1;

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
