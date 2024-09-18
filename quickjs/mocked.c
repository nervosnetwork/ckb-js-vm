#include <sys/time.h>
#include <time.h>

int __wrap_fesetround(int _round) { return 0; }

int __wrap_gettimeofday(struct timeval *restrict tv, struct timezone *restrict tz) { return 0; }

struct tm *__wrap_localtime_r(const time_t *a, struct tm *b) { return 0; }
