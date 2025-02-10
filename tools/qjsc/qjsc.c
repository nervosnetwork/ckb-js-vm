/**
 * A simple compiler turning javascript code into bytecode
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "quickjs.h"
#include "quickjs-libc.h"
#include "cutils.h"


static void output_object_code(JSContext *ctx, FILE *fo, JSValueConst obj, const char *c_name, BOOL load_only) {
    uint8_t *out_buf;
    size_t out_buf_len;
    int flags;
    flags = JS_WRITE_OBJ_BYTECODE;
    out_buf = JS_WriteObject(ctx, &out_buf_len, obj, flags);
    if (!out_buf) {
        js_std_dump_error(ctx);
        exit(1);
    }

    fwrite(out_buf, out_buf_len, 1, fo);

    js_free(ctx, out_buf);
}

static void compile_file(JSContext *ctx, FILE *fo, const char *filename, const char *c_name1, int module) {
    uint8_t *buf;
    int eval_flags;
    JSValue obj;
    size_t buf_len;

    buf = js_load_file(ctx, &buf_len, filename);
    if (!buf) {
        fprintf(stderr, "Could not load '%s'\n", filename);
        exit(1);
    }
    eval_flags = JS_EVAL_FLAG_COMPILE_ONLY;
    if (module < 0) {
        module = (has_suffix(filename, ".mjs") || JS_DetectModule((const char *)buf, buf_len));
    }
    if (module)
        eval_flags |= JS_EVAL_TYPE_MODULE;
    else
        eval_flags |= JS_EVAL_TYPE_GLOBAL;
    obj = JS_Eval(ctx, (const char *)buf, buf_len, filename, eval_flags);
    if (JS_IsException(obj)) {
        js_std_dump_error(ctx);
        exit(1);
    }
    js_free(ctx, buf);
    output_object_code(ctx, fo, obj, filename, false);
    JS_FreeValue(ctx, obj);
}

void print_usage(void) {
    printf("Usage: qjsc <input.js> <output.bin>\n");
    printf("Compile JavaScript source code to QuickJS bytecode\n\n");
    printf("Arguments:\n");
    printf("  <input.js>    Input JavaScript source file\n");
    printf("  <output.bin>  Output binary bytecode file\n");
}

static int js_module_dummy_init(JSContext *ctx, JSModuleDef *m) { return 0; }

JSModuleDef *js_module_dummy_loader(JSContext *ctx, const char *module_name, void *opaque) {
    return JS_NewCModule(ctx, module_name, js_module_dummy_init);
}

int main(int argc, char **argv) {
    JSRuntime* rt = JS_NewRuntime();
    JSContext* ctx = JS_NewContext(rt);
    JS_AddIntrinsicBigFloat(ctx);
    JS_AddIntrinsicBigDecimal(ctx);
    JS_AddIntrinsicOperators(ctx);
    JS_EnableBignumExt(ctx, true);

    /* loader for ES6 modules */
    JS_SetModuleLoaderFunc(rt, NULL, js_module_dummy_loader, NULL);
    if (argc != 3) {
        print_usage();
        return 1;
    }

    const char *filename = argv[1];
    const char *output = argv[2];
    FILE *fo = fopen(output, "w");
    compile_file(ctx, fo, filename, NULL, 1);
    fclose(fo);
    return 0;
}
