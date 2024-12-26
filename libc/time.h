#ifndef CKB_C_STDLIB_TIME_H_
#define CKB_C_STDLIB_TIME_H_

#include <sys/time.h>

#define time_t long
#define CLOCK_MONOTONIC 0

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

int clock_gettime(int c, struct timespec *ts);

struct tm *localtime_r(const time_t *timer, struct tm *buf);

#endif
