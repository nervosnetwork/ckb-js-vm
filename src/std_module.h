#ifndef _STD_MODULE_H_
#define _STD_MODULE_H_
#include <stdio.h>
#include <stdlib.h>

#include "quickjs.h"

void js_std_add_helpers(JSContext *ctx, int argc, const char *argv[]);
uint8_t *js_load_file(JSContext *ctx, size_t *pbuf_len, const char *filename);
int js_module_set_import_meta(JSContext *ctx, JSValueConst func_val, JS_BOOL use_realpath, JS_BOOL is_main);
JSModuleDef *js_module_loader(JSContext *ctx, const char *module_name, void *opaque);

static int js_module_dummy_init(JSContext *ctx, JSModuleDef *m);
JSModuleDef *js_module_dummy_loader(JSContext *ctx, const char *module_name, void *opaque);

#endif
