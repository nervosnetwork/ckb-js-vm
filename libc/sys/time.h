#ifndef CKB_C_STDLIB_SYS_TIME_H_
#define CKB_C_STDLIB_SYS_TIME_H_

typedef unsigned long long suseconds_t;
typedef unsigned int time_t;

struct tm {
    int tm_gmtoff;
};

#ifndef _STRUCT_TIMEVAL
struct timeval {
    time_t tv_sec;       /* seconds */
    suseconds_t tv_usec; /* microseconds */
};
#endif

struct timezone {
    int tz_minuteswest; /* minutes west of Greenwich */
    int tz_dsttime;     /* type of DST correction */
};

int gettimeofday(struct timeval *restrict tv, struct timezone *restrict tz);

#endif
