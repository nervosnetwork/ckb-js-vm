#include "../my_math.h"
#include <stdint.h>
#include "my_stdio.h"
#include "my_assert.h"

#ifndef DBL_EPSILON
#define DBL_EPSILON 2.22044604925031308085e-16
#endif

double acos(double x) {
    assert(0);
    return 0;
}

double asin(double x) {
    assert(0);
    return 0;
}

double atan2(double x, double y) {
    assert(0);
    return 0;
}

double cos(double x) {
    assert(0);
    return 0;
}

double cosh(double x) {
    assert(0);
    return 0;
}

double exp(double x) {
    assert(0);
    return 0;
}

#define EPS DBL_EPSILON
double floor(double x) {
    static const double toint = 1 / EPS;
    union {
        double f;
        uint64_t i;
    } u = {x};
    int e = u.i >> 52 & 0x7ff;
    double y;

    if (e >= 0x3ff + 52 || x == 0) return x;
    /* y = int(x) - x, where int(x) is an integer neighbor of x */
    if (u.i >> 63)
        y = x - toint + toint - x;
    else
        y = x + toint - toint - x;
    /* special case because of non-nearest rounding modes */
    if (e <= 0x3ff - 1) {
        FORCE_EVAL(y);
        return u.i >> 63 ? -1 : 0;
    }
    if (y > 0) return x + y - 1;
    return x + y;
}

double ceil(double x) {
    static const double toint = 1 / EPS;
    union {
        double f;
        uint64_t i;
    } u = {x};
    int e = u.i >> 52 & 0x7ff;
    double y;

    if (e >= 0x3ff + 52 || x == 0) return x;
    /* y = int(x) - x, where int(x) is an integer neighbor of x */
    if (u.i >> 63)
        y = x - toint + toint - x;
    else
        y = x + toint - toint - x;
    /* special case because of non-nearest rounding modes */
    if (e <= 0x3ff - 1) {
        FORCE_EVAL(y);
        return u.i >> 63 ? -0.0 : 1;
    }
    if (y < 0) return x + y + 1;
    return x + y;
}

double fabs(double x) {
    union {
        double f;
        uint64_t i;
    } u = {x};
    u.i &= -1ULL / 2;
    return u.f;
}

double ldexp(double x, int n) { return scalbn(x, n); }

double log2(double x) {
    assert(0);
    return 0;
}

double log10(double x) {
    assert(0);
    return 0;
}

double sin(double x) {
    assert(0);
    return 0;
}

double sinh(double x) {
    assert(0);
    return 0;
}

double sqrt(double x) {
    assert(0);
    return 0;
}

double tan(double x) {
    assert(0);
    return 0;
}

double tanh(double x) {
    assert(0);
    return 0;
}

double rint(double x) {
    static const double toint = 1 / EPS;
    union {
        double f;
        uint64_t i;
    } u = {x};
    int e = u.i >> 52 & 0x7ff;
    int s = u.i >> 63;
    double y;

    if (e >= 0x3ff + 52) return x;
    if (s)
        y = x - toint + toint;
    else
        y = x + toint - toint;
    if (y == 0) return s ? -0.0 : 0;
    return y;
}

long int lrint(double x) { return rint(x); }

double cbrt(double x) {
    assert(0);
    return 0;
}

double fmod(double numer, double denom);

double fmin(double x, double y) { return x > y ? y : x; }

double fmax(double x, double y) { return x < y ? y : x; }

double hypot(double x, double y) {
    assert(0);
    return 0;
}

int signbit(double num) { return num > 0; }

double frexp(double x, int *e) {
    union {
        double d;
        uint64_t i;
    } y = {x};
    int ee = y.i >> 52 & 0x7ff;

    if (!ee) {
        if (x) {
            x = frexp(x * 0x1p64, e);
            *e -= 64;
        } else
            *e = 0;
        return x;
    } else if (ee == 0x7ff) {
        return x;
    }

    *e = ee - 0x3fe;
    y.i &= 0x800fffffffffffffull;
    y.i |= 0x3fe0000000000000ull;
    return y.d;
}

double fmod(double x, double y) {
    union {
        double f;
        uint64_t i;
    } ux = {x}, uy = {y};
    int ex = ux.i >> 52 & 0x7ff;
    int ey = uy.i >> 52 & 0x7ff;
    int sx = ux.i >> 63;
    uint64_t i;

    /* in the followings uxi should be ux.i, but then gcc wrongly adds */
    /* float load/store to inner loops ruining performance and code size */
    uint64_t uxi = ux.i;

    if (uy.i << 1 == 0 || __builtin_isnan(y) || ex == 0x7ff) return (x * y) / (x * y);
    if (uxi << 1 <= uy.i << 1) {
        if (uxi << 1 == uy.i << 1) return 0 * x;
        return x;
    }

    /* normalize x and y */
    if (!ex) {
        for (i = uxi << 12; i >> 63 == 0; ex--, i <<= 1)
            ;
        uxi <<= -ex + 1;
    } else {
        uxi &= -1ULL >> 12;
        uxi |= 1ULL << 52;
    }
    if (!ey) {
        for (i = uy.i << 12; i >> 63 == 0; ey--, i <<= 1)
            ;
        uy.i <<= -ey + 1;
    } else {
        uy.i &= -1ULL >> 12;
        uy.i |= 1ULL << 52;
    }

    /* x mod y */
    for (; ex > ey; ex--) {
        i = uxi - uy.i;
        if (i >> 63 == 0) {
            if (i == 0) return 0 * x;
            uxi = i;
        }
        uxi <<= 1;
    }
    i = uxi - uy.i;
    if (i >> 63 == 0) {
        if (i == 0) return 0 * x;
        uxi = i;
    }
    for (; uxi >> 52 == 0; uxi <<= 1, ex--)
        ;

    /* scale result */
    if (ex > 0) {
        uxi -= 1ULL << 52;
        uxi |= (uint64_t)ex << 52;
    } else {
        uxi >>= -ex + 1;
    }
    uxi |= (uint64_t)sx << 63;
    ux.i = uxi;
    return ux.f;
}

int abs(int a) { return a > 0 ? a : -a; }
