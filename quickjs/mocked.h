

#ifndef __MOCKED_H__
#define __MOCKED_H__
#define FE_TONEAREST 0
#define assert(f) (void)(f)

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

int fesetround(int round);
int fegetround();
struct tm *localtime_r(const time_t *timer, struct tm *buf);
int gettimeofday(struct timeval *restrict tv, struct timezone *restrict tz);

#endif
