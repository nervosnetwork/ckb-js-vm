#ifndef HASH_MODULE_H
#define HASH_MODULE_H

#include <quickjs.h>

int qjs_init_module_hash(JSContext* ctx, JSModuleDef* m);
int qjs_init_module_hash_lazy(JSContext* ctx, JSModuleDef* m);

#endif /* HASH_MODULE_H */
