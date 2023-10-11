#include <stdlib.h>
#include "my_stdlib.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>
#include "cutils.h"
#include "std_module.h"
#include "ckb_syscall_apis.h"
#include "my_string.h"
#include "ckb_cell_fs.h"

/* console.log */
static JSValue js_print(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    for (int i = 0; i < argc; i++) {
        int tag = JS_VALUE_GET_TAG(argv[i]);
        if (JS_TAG_IS_FLOAT64(tag)) {
            double d = JS_VALUE_GET_FLOAT64(argv[i]);
            printf("%f", d);
        } else {
            size_t len;
            const char *str = JS_ToCStringLen(ctx, &len, argv[i]);
            if (!str) return JS_EXCEPTION;
            ckb_debug(str);
            JS_FreeCString(ctx, str);
        }
    }
    return JS_UNDEFINED;
}

static JSValue js_assert(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    bool success = JS_ToBool(ctx, argv[0]);
    if (!success) {
        js_print(ctx, this_val, argc - 1, argv + 1);
        return JS_EXCEPTION;
    }
    return JS_UNDEFINED;
}

void js_std_add_helpers(JSContext *ctx, int argc, const char *argv[]) {
    JSValue global_obj, console, args;
    int i;

    /* XXX: should these global definitions be enumerable? */
    global_obj = JS_GetGlobalObject(ctx);

    console = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, console, "log", JS_NewCFunction(ctx, js_print, "log", 1));
    JS_SetPropertyStr(ctx, console, "assert", JS_NewCFunction(ctx, js_assert, "assert", 1));
    JS_SetPropertyStr(ctx, global_obj, "console", console);

    /* same methods as the mozilla JS shell */
    if (argc >= 0) {
        args = JS_NewArray(ctx);
        for (i = 0; i < argc; i++) {
            JS_SetPropertyUint32(ctx, args, i, JS_NewString(ctx, argv[i]));
        }
        JS_SetPropertyStr(ctx, global_obj, "scriptArgs", args);
    }

    JS_SetPropertyStr(ctx, global_obj, "print", JS_NewCFunction(ctx, js_print, "print", 1));
    // TODO:
    // JS_SetPropertyStr(ctx, global_obj, "__loadScript",
    //                   JS_NewCFunction(ctx, js_loadScript, "__loadScript",
    //                   1));

    JS_FreeValue(ctx, global_obj);
}

uint8_t *js_load_file(JSContext *ctx, size_t *pbuf_len, const char *filename) {
    FSFile *f = NULL;

    int err = ckb_get_file(filename, &f);
    if (err) {
        return NULL;
    }
    if (f->size == 0) {
        return NULL;
    }

    *pbuf_len = f->size;
    return (uint8_t *)f->content;
}

int js_module_set_import_meta(JSContext *ctx, JSValueConst func_val, JS_BOOL use_realpath, JS_BOOL is_main) {
    JSModuleDef *m;
    char buf[PATH_MAX + 16];
    JSValue meta_obj;
    JSAtom module_name_atom;
    const char *module_name;

    if (JS_VALUE_GET_TAG(func_val) != JS_TAG_MODULE) {
        return -1;
    }
    m = JS_VALUE_GET_PTR(func_val);

    module_name_atom = JS_GetModuleName(ctx, m);
    module_name = JS_AtomToCString(ctx, module_name_atom);
    JS_FreeAtom(ctx, module_name_atom);
    if (!module_name) return -1;
    pstrcpy(buf, sizeof(buf), module_name);
    JS_FreeCString(ctx, module_name);

    meta_obj = JS_GetImportMeta(ctx, m);
    if (JS_IsException(meta_obj)) return -1;
    JS_DefinePropertyValueStr(ctx, meta_obj, "url", JS_NewString(ctx, buf), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, meta_obj, "main", JS_NewBool(ctx, is_main), JS_PROP_C_W_E);
    JS_FreeValue(ctx, meta_obj);
    return 0;
}

JSModuleDef *js_module_loader(JSContext *ctx, const char *module_name, void *opaque) {
    JSModuleDef *m;

    size_t buf_len;
    uint8_t *buf;
    JSValue func_val;

    buf = js_load_file(ctx, &buf_len, module_name);
    if (!buf) {
        if (strlen(module_name) <= 3) {
            JS_ThrowReferenceError(ctx, "could not load module filename '%s'", module_name);
            return NULL;
        }
        if (strcmp(module_name + strlen(module_name) - 3, ".js") != 0) {
            JS_ThrowReferenceError(ctx, "could not load module filename '%s'", module_name);
            return NULL;
        }
        char secend_name[256];
        strcpy(secend_name, module_name);
        strcpy(secend_name + strlen(module_name) - 3, ".bc");
        buf = js_load_file(ctx, &buf_len, secend_name);
        if (!buf) {
            JS_ThrowReferenceError(ctx, "could not load module filename '%s'", module_name);
            return NULL;
        }
    }

    if (((const char *)buf)[0] == (char)BC_VERSION) {
        func_val = JS_ReadObject(ctx, buf, buf_len, JS_READ_OBJ_BYTECODE);
    } else {
        /* compile the module */
        func_val = JS_Eval(ctx, (char *)buf, buf_len, module_name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    }

    // js_free(ctx, buf);
    if (JS_IsException(func_val)) return NULL;
    /* XXX: could propagate the exception */
    js_module_set_import_meta(ctx, func_val, TRUE, FALSE);
    /* the module is already referenced, so we must free it */
    m = JS_VALUE_GET_PTR(func_val);
    JS_FreeValue(ctx, func_val);
    return m;
}

static int js_module_dummy_init(JSContext *ctx, JSModuleDef *m) { return 0; }

JSModuleDef *js_module_dummy_loader(JSContext *ctx, const char *module_name, void *opaque) {
    return JS_NewCModule(ctx, module_name, js_module_dummy_init);
}
