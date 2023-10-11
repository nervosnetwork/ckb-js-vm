#ifndef _STDLIB_ERRNO_H_
#define _STDLIB_ERRNO_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
__attribute__((const))
#endif
int *__errno_location(void);
#define errno (*__errno_location())

#ifdef __cplusplus
}
#endif

#endif
