#ifndef _CKB_MODULE_H_
#define _CKB_MODULE_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "quickjs.h"

int qjs_init_module_ckb(JSContext *ctx, JSModuleDef *m);
int qjs_init_module_ckb_lazy(JSContext *ctx, JSModuleDef *m);
/**
 * Throws a JavaScript error with a custom error code.
 *
 * This function creates and throws a JavaScript error object with an additional
 * 'errorCode' property, but without generating a stack trace. Using error codes
 * allows for more precise error handling in JavaScript code.
 *
 * @param ctx          The JavaScript context
 * @param error_code   Custom error code to identify the specific error type
 * @param message      Human-readable error message
 * @return             The JavaScript error value (always throws in JS context)
 *
 * Common use cases:
 * - CKB_INDEX_OUT_OF_BOUND 1
 * - CKB_ITEM_MISSING 2
 */
JSValue qjs_throw_error(JSContext *ctx, int32_t error_code, const char *message);
JSValue qjs_eval_script(JSContext *ctx, const char *str, int len, bool enable_module);

int qjs_read_local_file(char *buf, int size);
int qjs_load_cell_code_info_explicit(size_t *buf_size, size_t *index, const uint8_t *code_hash, uint8_t hash_type);
int qjs_load_cell_code_info(size_t *buf_size, size_t *index, bool *use_filesystem);
int qjs_load_cell_code(size_t buf_size, size_t index, uint8_t *buf);

#endif  // _CKB_MODULE_H_
