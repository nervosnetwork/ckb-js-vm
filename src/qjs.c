/*
 * QuickJS stand alone interpreter
 *
 * Copyright (c) 2017-2021 Fabrice Bellard
 * Copyright (c) 2017-2021 Charlie Gordon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <malloc.h>
#include "cutils.h"
#include "std_module.h"
#include "ckb_module.h"
#include "secp256k1_module.h"
#include "hash_module.h"
#include "misc_module.h"
#include "ckb_exec.h"
#include "qjs.h"

#define INIT_FILE_NAME "init.js"
#define INIT_FILE_NAME_BC "init.bc"
#define ENTRY_FILE_NAME "index.js"
#define ENTRY_FILE_NAME_BC "index.bc"

static void js_dump_obj(JSContext *ctx, JSValueConst val) {
    const char *str;

    str = JS_ToCString(ctx, val);
    if (str) {
        printf("%s", str);
        JS_FreeCString(ctx, str);
    } else {
        printf("[exception]");
    }
}

static void js_std_dump_error1(JSContext *ctx, JSValueConst exception_val) {
    JSValue val;
    BOOL is_error;

    is_error = JS_IsError(ctx, exception_val);
    js_dump_obj(ctx, exception_val);
    if (is_error) {
        val = JS_GetPropertyStr(ctx, exception_val, "stack");
        if (!JS_IsUndefined(val)) {
            js_dump_obj(ctx, val);
        }
        JS_FreeValue(ctx, val);
    }
}

void js_std_dump_error(JSContext *ctx) {
    JSValue exception_val;
    exception_val = JS_GetException(ctx);
    js_std_dump_error1(ctx, exception_val);
    JS_FreeValue(ctx, exception_val);
}

int js_std_loop(JSContext *ctx) {
    JSContext *ctx1;
    int ret = 0;
    int err;
    for (;;) {
        err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
        if (err <= 0) {
            if (err < 0) {
                js_std_dump_error(ctx1);
                ret = QJS_ERROR_GENERIC;
            }
            break;
        }
    }
    return ret;
}

static inline long syscall(long n, long _a0, long _a1, long _a2, long _a3, long _a4, long _a5) {
    register long a0 asm("a0") = _a0;
    register long a1 asm("a1") = _a1;
    register long a2 asm("a2") = _a2;
    register long a3 asm("a3") = _a3;
    register long a4 asm("a4") = _a4;
    register long a5 asm("a5") = _a5;
    register long syscall_id asm("a7") = n;

    asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(syscall_id));
    /*
     * Syscalls might modify memory sent as pointer, adding a barrier here
     * ensures gcc won't do incorrect optimization.
     */
    asm volatile("fence" ::: "memory");

    return a0;
}

// syscalls only enabled in ckb-debugger
long ckb_debugger_fopen(const char *file_name, const char *mode) {
    return syscall(9003, (long)file_name, (long)mode, 0, 0, 0, 0);
}

void ckb_debugger_fclose(long handle) { syscall(9009, (long)handle, 0, 0, 0, 0, 0); }

int ckb_debugger_fwrite(const void *ptr, size_t size, size_t nitems, long stream) {
    return syscall(9012, (long)ptr, (long)size, (long)nitems, (long)stream, 0, 0);
}

int compile_from_file(JSContext *ctx, const char *bytecode_filename) {
    enable_local_access(1);
    char buf[1024 * 512];
    int buf_len = read_local_file(buf, sizeof(buf));
    if (buf_len < 0 || buf_len == sizeof(buf)) {
        if (buf_len == sizeof(buf)) {
            printf("Error while reading from file: file too large\n");
            return QJS_ERROR_FILE_TOO_LARGE;
        } else {
            printf("Error while reading from file: %d\n", buf_len);
            return QJS_ERROR_FILE_READ;
        }
    }

    JSValue val;
    val = JS_Eval(ctx, buf, buf_len, "", JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        return QJS_ERROR_EVAL;
    }
    uint8_t *out_buf;
    size_t out_buf_len;
    out_buf = JS_WriteObject(ctx, &out_buf_len, val, JS_WRITE_OBJ_BYTECODE);
    if (!out_buf) return QJS_ERROR_MEMORY_ALLOCATION;
    long handle = ckb_debugger_fopen(bytecode_filename, "wb");
    int written = ckb_debugger_fwrite(out_buf, 1, out_buf_len, handle);
    if (written != out_buf_len) {
        ckb_debugger_fclose(handle);
        printf("Error while writing to file %s", bytecode_filename);
        return QJS_ERROR_GENERIC;
    }
    ckb_debugger_fclose(handle);
    return 0;
}

static int eval_buf(JSContext *ctx, const void *buf, int buf_len, const char *filename, bool is_main) {
    JSValue val;
    int ret;
    /* Use module mode by default for better security and modern JS features:
     * - Enables strict mode automatically
     * - Disables legacy unsafe features like 'with' statements
     * - Allows import/export statements
     * - Provides better scoping isolation
     */
    int eval_flags = JS_EVAL_TYPE_MODULE;
    if (((const char *)buf)[0] == (char)BC_VERSION) {
        val = JS_ReadObject(ctx, buf, buf_len, JS_READ_OBJ_BYTECODE);
        if (JS_IsException(val)) {
            js_std_dump_error(ctx);
            return QJS_ERROR_GENERIC;
        }
        if (JS_VALUE_GET_TAG(val) == JS_TAG_MODULE) {
            if (JS_ResolveModule(ctx, val) < 0) {
                JS_FreeValue(ctx, val);
                js_std_dump_error(ctx);
                return QJS_ERROR_GENERIC;
            }
            js_module_set_import_meta(ctx, val, FALSE, is_main);
        }
        val = JS_EvalFunction(ctx, val);
    } else if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
        /* for the modules, we compile then run to be able to set
           import.meta */
        val = JS_Eval(ctx, buf, buf_len, filename, eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JS_IsException(val)) {
            js_module_set_import_meta(ctx, val, false, is_main);
            val = JS_EvalFunction(ctx, val);
        }
    } else {
        val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
    }
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        ret = QJS_ERROR_GENERIC;
    } else {
        if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
            int promise_state = JS_PromiseState(ctx, val);
            if (promise_state == JS_PROMISE_REJECTED) {
                JSValue error = JS_PromiseResult(ctx, val);
                js_std_dump_error1(ctx, error);
                JS_FreeValue(ctx, error);
                ret = QJS_ERROR_EXCEPTION;
            } else {
                ret = 0;
            }
        } else {
            ret = 0;
        }
    }
    JS_FreeValue(ctx, val);
    return ret;
}

int run_from_file_system_buf(JSContext *ctx, char *buf, size_t buf_size) {
    int err = ckb_load_fs("/", buf, buf_size);
    CHECK(err);

    FSFile *init_file = NULL;
    err = ckb_get_file(INIT_FILE_NAME, &init_file);
    if (err != 0) {
        ckb_get_file(INIT_FILE_NAME_BC, &init_file);
        // skip error checking
    }
    if (init_file) {
        err = eval_buf(ctx, init_file->content, init_file->size, INIT_FILE_NAME, false);
        CHECK(err);
    }

    FSFile *entry_file = NULL;
    err = ckb_get_file(ENTRY_FILE_NAME, &entry_file);
    if (err != 0) {
        err = ckb_get_file(ENTRY_FILE_NAME_BC, &entry_file);
    }
    CHECK(err);
    CHECK2(entry_file->size > 0, QJS_ERROR_EMPTY_FILE);
    err = eval_buf(ctx, entry_file->content, entry_file->size, ENTRY_FILE_NAME, true);
    CHECK(err);

exit:
    return err;
}

static int run_from_local_file(JSContext *ctx, bool enable_fs) {
    printf("Run from file, local access enabled. For Testing only.\n");
    enable_local_access(1);
    char buf[1024 * 512];
    int count = read_local_file(buf, sizeof(buf));
    if (count < 0 || count == sizeof(buf)) {
        if (count == sizeof(buf)) {
            printf("Error while reading from file: file too large\n");
            return QJS_ERROR_FILE_TOO_LARGE;
        } else {
            printf("Error while reading from file: %d\n", count);
            return QJS_ERROR_FILE_READ;
        }
    }
    if (enable_fs) {
        return run_from_file_system_buf(ctx, buf, (size_t)count);
    } else {
        buf[count] = 0;
        return eval_buf(ctx, buf, count, "<run_from_file>", true);
    }
}

static int run_from_cell_data(JSContext *ctx, bool enable_fs) {
    int err = 0;
    size_t buf_size = 0;
    size_t index = 0;
    bool use_filesystem = false;
    err = load_cell_code_info(&buf_size, &index, &use_filesystem);
    if (err) {
        return err;
    }

    char *buf = malloc(buf_size + 1);
    err = load_cell_code(buf_size, index, (uint8_t *)buf);
    if (err) {
        return err;
    }
    if (enable_fs || use_filesystem) {
        err = run_from_file_system_buf(ctx, buf, buf_size);
        free(buf);
        return err;
    } else {
        buf[buf_size] = 0;
        err = eval_buf(ctx, buf, buf_size, "<run_from_file>", true);
        free(buf);
        return err;
    }
}

static int run_from_target(JSContext *ctx, const char *target, bool enable_fs) {
    if (strlen(target) < 66) {
        return QJS_ERROR_INVALID_ARGUMENT;
    }

    uint8_t target_byte[33] = {};
    uint32_t length = 0;
    _exec_hex2bin(target, target_byte, 33, &length);
    uint8_t *code_hash = target_byte;
    uint8_t hash_type = target_byte[32];

    int err = 0;
    size_t buf_size = 0;
    size_t index = 0;

    err = load_cell_code_info_explicit(&buf_size, &index, code_hash, hash_type);
    if (err) {
        return err;
    }

    char *buf = malloc(buf_size + 1);
    err = load_cell_code(buf_size, index, (uint8_t *)buf);
    if (err) {
        return err;
    }

    if (enable_fs) {
        err = run_from_file_system_buf(ctx, buf, buf_size);
        free(buf);
        return err;
    } else {
        buf[buf_size] = 0;
        err = eval_buf(ctx, buf, buf_size, "<run_from_file>", true);
        free(buf);
        return err;
    }
}

/* also used to initialize the worker context */
static JSContext *JS_NewCustomContext(JSRuntime *rt) {
    JSContext *ctx;
    ctx = JS_NewContext(rt);
    if (!ctx) return NULL;
    JS_AddIntrinsicBigFloat(ctx);
    JS_AddIntrinsicBigDecimal(ctx);
    JS_AddIntrinsicOperators(ctx);
    JS_EnableBignumExt(ctx, TRUE);
    return ctx;
}

static int init_func(JSContext *ctx, JSModuleDef *m) {
    qjs_init_module_ckb_lazy(ctx, m);
    qjs_init_module_hash_lazy(ctx, m);
    qjs_init_module_misc_lazy(ctx, m);
    qjs_init_module_secp256k1_lazy(ctx, m);
    return 0;
}

static void print_help_message(void) {
    printf("Usage: ckb-js-vm [options]\n");
    printf("Options:\n");
    printf("  -h, --help        show this help message\n");
    printf("  -c                compile javascript to bytecode\n");
    printf("  -e <code>         run javascript from argument value\n");
    printf("  -r                read from file\n");
    printf("  -t <target>       specify target code_hash and hash_type in hex\n");
    printf("  -f                use file system\n");
}

int main(int argc, const char **argv) {
    int err = 0;
    JSRuntime *rt = NULL;
    JSContext *ctx = NULL;

    // command line parsing
    size_t optind = 0;
    bool c_flag = false;         // compile flag
    bool r_flag = false;         // read from file flag
    bool f_flag = false;         // use filesystem flag
    const char *e_value = NULL;  // eval argument
    const char *t_value = NULL;  // target argument
    const char *bytecode_filename = NULL;

    for (int i = 0; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_help_message();
            return 0;
        } else if (strcmp(arg, "-c") == 0) {
            c_flag = true;
            bytecode_filename = argv[i + 1];
            optind = i + 2;
        } else if (strcmp(arg, "-e") == 0) {
            if (i + 1 < argc) {
                e_value = argv[++i];
                optind = i + 1;
            } else {
                printf("Error: -e requires an argument\n");
                return 1;
            }
        } else if (strcmp(arg, "-r") == 0) {
            r_flag = true;
            optind = i + 1;
        } else if (strcmp(arg, "-f") == 0) {
            f_flag = true;
            optind = i + 1;
        } else if (strcmp(arg, "-t") == 0) {
            if (i + 1 < argc) {
                t_value = argv[++i];
                optind = i + 1;
            } else {
                printf("Error: -t requires an argument\n");
                return 1;
            }
        }
    }

    size_t memory_limit = 0;
    size_t stack_size = 1024 * 1020;
    rt = JS_NewRuntime();
    if (!rt) {
        printf("qjs: cannot allocate JS runtime\n");
        return QJS_ERROR_GENERIC;
    }
    if (memory_limit != 0) JS_SetMemoryLimit(rt, memory_limit);
    if (stack_size != 0) JS_SetMaxStackSize(rt, stack_size);
    // TODO:
    // js_std_set_worker_new_context_func(JS_NewCustomContext);
    // js_std_init_handlers(rt);
    ctx = JS_NewCustomContext(rt);
    CHECK2(ctx != NULL, QJS_ERROR_GENERIC);
    /* loader for ES6 modules */
    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);
    // Now passing remaining arguments after the flags
    js_std_add_helpers(ctx, argc - optind, &argv[optind]);

    JSModuleDef *m = JS_NewCModule(ctx, "@ckb-js-std/bindings", init_func);
    qjs_init_module_ckb(ctx, m);
    qjs_init_module_hash(ctx, m);
    qjs_init_module_misc(ctx, m);
    qjs_init_module_secp256k1(ctx, m);
    const char *require_script =
        "import * as ckb from '@ckb-js-std/bindings';\n"
        "globalThis.__ckb_module = ckb;  \n"
        "globalThis.require = function (name) { \n"
        " if (name === '@ckb-js-std/bindings') { \n"
        " return globalThis.__ckb_module; }\n"
        " throw new Error('cannot find the module: ' + name); } \0";
    err = eval_buf(ctx, require_script, strlen(require_script), "<require script>", false);
    CHECK(err);
    // Replace the command-line handling logic
    if (c_flag) {
        JS_SetModuleLoaderFunc(rt, NULL, js_module_dummy_loader, NULL);
        err = compile_from_file(ctx, bytecode_filename);
    } else if (e_value) {
        err = eval_buf(ctx, e_value, strlen(e_value), "<cmdline>", true);
    } else if (r_flag && f_flag) {
        err = run_from_local_file(ctx, true);
    } else if (r_flag) {
        err = run_from_local_file(ctx, false);
    } else if (t_value && f_flag) {
        err = run_from_target(ctx, t_value, true);
    } else if (t_value) {
        err = run_from_target(ctx, t_value, false);
    } else if (f_flag) {
        err = run_from_cell_data(ctx, true);
    } else {
        err = run_from_cell_data(ctx, false);
    }
    CHECK(err);
    err = js_std_loop(ctx);
    CHECK(err);

#ifdef MEMORY_USAGE
    size_t heap_usage = malloc_usage();
    printf("Total bytes used by allocator(malloc/realloc) is %d K\n", heap_usage / 1024);
    size_t stack_usage = JS_GetStackPeak();
    printf("Total bytes used by stack(peak value) is %d K\n", (4 * 1024 * 1024 - stack_usage) / 1024);
#endif

exit:
    // No cleanup is needed.
    // js_std_free_handlers(rt);
    // JS_FreeContext(ctx);
    // JS_FreeRuntime(rt);
    return err;
}
