#ifndef CKB_C_STDLIB_MALLOC_H_
#define CKB_C_STDLIB_MALLOC_H_

#include <stdint.h>
#ifndef CKB_BRK_MAX
#define CKB_BRK_MAX 0x00300000
#endif

#define CKB_MEMORY_LIMIT 0x00400000

size_t malloc_usable_size(void *ptr);
void malloc_config(uintptr_t min, uintptr_t max);
size_t malloc_usage();

#endif  // CKB_C_STDLIB_MALLOC_H_
