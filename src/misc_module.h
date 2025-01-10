#ifndef JS_MISC_MODULE_H
#define JS_MISC_MODULE_H

#include "quickjs.h"

// Class ID for Smt
static JSClassID js_smt_class_id;

// Module initialization
int js_init_module_misc(JSContext *ctx);

#endif
