#ifndef CKB_C_STDLIB_MALLOC_H_
#define CKB_C_STDLIB_MALLOC_H_

#include <stdint.h>

size_t malloc_usable_size(void *ptr);
void malloc_config(uintptr_t min, uintptr_t max);
size_t malloc_usage();

#endif  // CKB_C_STDLIB_MALLOC_H_
