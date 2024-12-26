#ifndef CKB_C_STDLIB_FENV_H_
#define CKB_C_STDLIB_FENV_H_
#define FE_TONEAREST 0
#define FE_DOWNWARD 0
#define FE_UPWARD 0
int fesetround(int round);
int fegetround();
#endif
