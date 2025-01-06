#ifndef _SECP256K1_MODULE_H_
#define _SECP256K1_MODULE_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "quickjs.h"

int js_init_module_secp256k1(JSContext *ctx);

#endif  // _SECP256K1_MODULE_H_
