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
#include "my_stdlib.h"
#include <stdio.h>
#include "my_stdio.h"
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include "my_malloc.h"
#include "cutils.h"
#include "std_module.h"
#include "ckb_module.h"
#include "ckb_exec.h"
#include "cmdopt.h"

#define MAIN_FILE_NAME "main.js"
#define MAIN_FILE_NAME_BC "main.bc"

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

void js_std_loop(JSContext *ctx) {
    JSContext *ctx1;
    int err;
    for (;;) {
        err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
        if (err <= 0) {
            if (err < 0) {
                js_std_dump_error(ctx1);
            }
            break;
        }
    }
}

int compile_from_file(JSContext *ctx) {
    enable_local_access(1);
    char buf[1024 * 512];
    int buf_len = read_local_file(buf, sizeof(buf));
    if (buf_len < 0 || buf_len == sizeof(buf)) {
        if (buf_len == sizeof(buf)) {
            printf("Error while reading from file: file too large\n");
        } else {
            printf("Error while reading from file: %d\n", buf_len);
        }
        return -1;
    }

    JSValue val;
    val = JS_Eval(ctx, buf, buf_len, "", JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        return -1;
    }
    uint8_t *out_buf;
    size_t out_buf_len;
    out_buf = JS_WriteObject(ctx, &out_buf_len, val, JS_WRITE_OBJ_BYTECODE);
    if (!out_buf) return -1;
    char msg_buf[65];
    for (int i = 0; i < out_buf_len; i += 32) {
        uint32_t size = i + 32 > out_buf_len ? out_buf_len - i : 32;
        _exec_bin2hex(&out_buf[i], size, msg_buf, 65, &size, true);
        msg_buf[size - 1] = 0;
        printf("%s", msg_buf);
    }
    return 0;
}

static int eval_buf(JSContext *ctx, const void *buf, int buf_len, const char *filename, int eval_flags) {
    JSValue val;
    int ret;

    if (((const char *)buf)[0] == (char)BC_VERSION) {
        val = JS_ReadObject(ctx, buf, buf_len, JS_READ_OBJ_BYTECODE);
        if (JS_IsException(val)) {
            js_std_dump_error(ctx);
            return -1;
        }
        if (JS_VALUE_GET_TAG(val) == JS_TAG_MODULE) {
            if (JS_ResolveModule(ctx, val) < 0) {
                JS_FreeValue(ctx, val);
                js_std_dump_error(ctx);
                return -1;
            }
            js_module_set_import_meta(ctx, val, FALSE, TRUE);
        }
        val = JS_EvalFunction(ctx, val);
    } else if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
        /* for the modules, we compile then run to be able to set
           import.meta */
        val = JS_Eval(ctx, buf, buf_len, filename, eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JS_IsException(val)) {
            // TODO:
            // js_module_set_import_meta(ctx, val, TRUE, TRUE);
            val = JS_EvalFunction(ctx, val);
        }
    } else {
        val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
    }
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        ret = -1;
    } else {
        ret = 0;
    }
    JS_FreeValue(ctx, val);
    return ret;
}

int run_from_file_system_buf(JSContext *ctx, char *buf, size_t buf_size) {
    int err = ckb_load_fs(buf, buf_size);
    CHECK(err);

    FSFile *main_file = NULL;
    err = ckb_get_file(MAIN_FILE_NAME, &main_file);
    if (err != 0) {
        err = ckb_get_file(MAIN_FILE_NAME_BC, &main_file);
    }
    CHECK(err);
    CHECK2(main_file->size > 0, -1);
    err = eval_buf(ctx, main_file->content, main_file->size, MAIN_FILE_NAME, JS_EVAL_TYPE_MODULE);
    CHECK(err);

exit:
    return err;
}

static int run_from_local_file(JSContext *ctx, bool enable_fs) {
    printf("Run from file, local access enabled. For Testing only.");
    enable_local_access(1);
    char buf[1024 * 512];
    int count = read_local_file(buf, sizeof(buf));
    if (count < 0 || count == sizeof(buf)) {
        if (count == sizeof(buf)) {
            printf("Error while reading from file: file too large\n");
        } else {
            printf("Error while reading from file: %d\n", count);
        }
        return -1;
    }
    if (enable_fs) {
        return run_from_file_system_buf(ctx, buf, (size_t)count);
    } else {
        buf[count] = 0;
        return eval_buf(ctx, buf, count, "<run_from_file>", 0);
    }
}

static int run_from_cell_data(JSContext *ctx, bool enable_fs) {
    int err = 0;
    size_t buf_size = 0;
    size_t index = 0;
    err = load_cell_code_info(&buf_size, &index);
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
        err = eval_buf(ctx, buf, buf_size, "<run_from_file>", 0);
        free(buf);
        return err;
    }
}

static int run_from_target(JSContext *ctx, const char *target, bool enable_fs) {
    if (strlen(target) < 66) {
        return -1;
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
        err = eval_buf(ctx, buf, buf_size, "<run_from_file>", 0);
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

static const CMDOptDesc js_vm_options[] = {
    { "h,help", 0, "show the help" },
    { "c", 0, "compile javascript to bytecode" },
    { "e", CMD_HAS_ARG, "run javascript from argument value" },
    { "r", 0, "read from file" },
    { "t", CMD_HAS_ARG, "specify target code_hash and hash_type in hex" },
    { "f", 0, "use file system" },
    { NULL },
};

int main(int argc, const char **argv) {
    int optind;
    CMDOption *co;
    co = cmdopt_init("ckb-js-vm");
    cmdopt_add_desc(co, js_vm_options);
    optind = cmdopt_parse(co, argc, argv);
    if (optind > argc) {
        cmdopt_show_desc(js_vm_options);
        exit(1);
    }
    if (cmdopt_has(co, "help")) {
        cmdopt_show_desc(js_vm_options);
        exit(1);
    }

    int err = 0;
    JSRuntime *rt = NULL;
    JSContext *ctx = NULL;
    size_t memory_limit = 0;
    size_t stack_size = 1024 * 1020;
    rt = JS_NewRuntime();
    if (!rt) {
        printf("qjs: cannot allocate JS runtime\n");
        return -2;
    }
    if (memory_limit != 0) JS_SetMemoryLimit(rt, memory_limit);
    if (stack_size != 0) JS_SetMaxStackSize(rt, stack_size);
    // TODO:
    // js_std_set_worker_new_context_func(JS_NewCustomContext);
    // js_std_init_handlers(rt);
    ctx = JS_NewCustomContext(rt);
    CHECK2(ctx != NULL, -1);
    /* loader for ES6 modules */
    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);
    js_std_add_helpers(ctx, argc - optind, &argv[optind]);
    err = js_init_module_ckb(ctx);
    CHECK(err);

    bool c_bool = cmdopt_has(co, "c");
    const char *e_data = cmdopt_get(co, "e");
    bool r_bool = cmdopt_has(co, "r");
    bool f_bool = cmdopt_has(co, "f");
    const char *t_data = cmdopt_get(co, "t");

    if (c_bool) {
        JS_SetModuleLoaderFunc(rt, NULL, js_module_dummy_loader, NULL);
        err = compile_from_file(ctx);
    } else if (e_data) {
        err = eval_buf(ctx, e_data, strlen(e_data), "<cmdline>", 0);
    } else if (r_bool && f_bool) {
        err = run_from_local_file(ctx, true);
    } else if (r_bool) {
        err = run_from_local_file(ctx, false);
    } else if (t_data && f_bool) {
        err = run_from_target(ctx, t_data, true);
    } else if (t_data) {
        err = run_from_target(ctx, t_data, false);
    } else if (f_bool) {
        err = run_from_cell_data(ctx, true);
    } else {
        err = run_from_cell_data(ctx, false);
    }
    if (err == 0) {
        js_std_loop(ctx);
    }
    CHECK(err);

#ifdef MEMORY_USAGE
    size_t heap_usage = malloc_usage();
    printf("Total bytes used by allocator(malloc/realloc) is %d K", heap_usage / 1024);
    size_t stack_usage = JS_GetStackPeak();
    printf("Total bytes used by stack(peak value) is %d K", (4 * 1024 * 1024 - stack_usage) / 1024);
#endif

exit:
    // No cleanup is needed.
    // js_std_free_handlers(rt);
    // JS_FreeContext(ctx);
    // JS_FreeRuntime(rt);
    return err;
}
