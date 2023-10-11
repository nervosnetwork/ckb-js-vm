#ifdef __cplusplus
extern "C" {
#endif

#define NAN (__builtin_nanf(""))
#define INFINITY (__builtin_inff())

/* fp_force_eval ensures that the input value is computed when that's
   otherwise unused.  To prevent the constant folding of the input
   expression, an additional fp_barrier may be needed or a compilation
   mode that does so (e.g. -frounding-math in gcc). Then it can be
   used to evaluate an expression for its fenv side-effects only.   */

#ifndef fp_force_evalf
#define fp_force_evalf fp_force_evalf
static inline void fp_force_evalf(float x) {
    volatile float y;
    y = x;
    (void)y;
}
#endif

#ifndef fp_force_eval
#define fp_force_eval fp_force_eval
static inline void fp_force_eval(double x) {
    volatile double y;
    y = x;
    (void)y;
}
#endif

#ifndef fp_force_evall
#define fp_force_evall fp_force_evall
static inline void fp_force_evall(long double x) {
    volatile long double y;
    y = x;
    (void)y;
}
#endif

#define FORCE_EVAL(x)                             \
    do {                                          \
        if (sizeof(x) == sizeof(float)) {         \
            fp_force_evalf(x);                    \
        } else if (sizeof(x) == sizeof(double)) { \
            fp_force_eval(x);                     \
        } else {                                  \
            fp_force_evall(x);                    \
        }                                         \
    } while (0)

double acos(double);

double asin(double);

double atan2(double, double);

double cos(double);

double cosh(double);

double exp(double);

double fabs(double);

double log(double);

double log2(double);

double log10(double);

double pow(double, double);

double sin(double);

double sinh(double);

double sqrt(double);

double tan(double);

double tanh(double);

double scalbn(double, int);

double ldexp(double, int);

double round(double x);
int isnan(double x);
int isfinite(double x);
double trunc(double x);
long int lrint(double x);
double floor(double x);
double cbrt(double x);
double fmod(double numer, double denom);
double fmin(double x, double y);
double fmax(double x, double y);
double hypot(double x, double y);
int signbit(double num);

double ceil(double x);

#ifdef __cplusplus
}
#endif
