#include <time.h>

int clock_gettime(int c, struct timespec *ts) { return 0; }

struct tm *localtime_r(const time_t *timer, struct tm *buf) { return 0; };
