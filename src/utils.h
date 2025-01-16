#ifndef __UTILS_H__
#define __UTILS_H__

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "quickjs.h"

bool qjs_bad_int_arg(JSContext *ctx, JSValue val, int index);
bool qjs_bad_bigint_arg(JSContext *ctx, JSValue val, int index);
bool qjs_bad_str_arg(JSContext *ctx, JSValue val, int index);

#endif
