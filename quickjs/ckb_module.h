#ifndef _CKB_MODULE_H_
#define _CKB_MODULE_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "quickjs.h"

int js_init_module_ckb(JSContext *ctx);
int read_local_file(char *buf, int size);

int load_cell_code_info(size_t *buf_size, size_t *index);
int load_cell_code(size_t buf_size, size_t index, uint8_t *buf);

#endif  // _CKB_MODULE_H_
