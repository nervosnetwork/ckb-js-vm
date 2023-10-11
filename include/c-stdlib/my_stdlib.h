#ifndef C_STDLIB_STDLIB_H_
#define C_STDLIB_STDLIB_H_

float strtof(const char *__restrict, char **__restrict);
double strtod(const char *__restrict, char **__restrict);
long double strtold(const char *__restrict, char **__restrict);
int atoi(const char *);

int abs(int);
void exit(int);
void abort(void);

#endif /* C_STDLIB_STDLIB_H_ */
