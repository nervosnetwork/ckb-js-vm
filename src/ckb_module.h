#ifndef _CKB_MODULE_H_
#define _CKB_MODULE_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "quickjs.h"

int qjs_init_module_ckb(JSContext *ctx, JSModuleDef *m);
int qjs_init_module_ckb_lazy(JSContext *ctx, JSModuleDef *m);

int read_local_file(char *buf, int size);

int load_cell_code_info_explicit(size_t *buf_size, size_t *index, const uint8_t *code_hash, uint8_t hash_type);
int load_cell_code_info(size_t *buf_size, size_t *index, bool *use_filesystem);
int load_cell_code(size_t buf_size, size_t index, uint8_t *buf);
JSValue eval_script(JSContext *ctx, const char *str, int len, bool enable_module);

#endif  // _CKB_MODULE_H_
