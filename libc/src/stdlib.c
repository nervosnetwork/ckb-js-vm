#include <stddef.h>
#include <stdlib.h>
#include "ckb_syscall_apis.h"

int isspace(int c) { return c == ' ' || (unsigned)c - '\t' < 5; }

// Copied from dietlibc
float strtof(const char *s, char **endptr) {
  register const char *p = s;
  register float value = 0.;
  int sign = +1;
  float factor;
  unsigned int expo;

  while (isspace(*p)) p++;

  switch (*p) {
    case '-':
      sign = -1; /* fall through */
    case '+':
      p++;
    default:
      break;
  }

  while ((unsigned int)(*p - '0') < 10u) value = value * 10 + (*p++ - '0');

  if (*p == '.') {
    factor = 1.;

    p++;
    while ((unsigned int)(*p - '0') < 10u) {
      factor *= 0.1;
      value += (*p++ - '0') * factor;
    }
  }

  if ((*p | 32) == 'e') {
    expo = 0;
    factor = 10.L;

    switch (*++p) {  // ja hier weiß ich nicht, was mindestens nach einem
                     // 'E' folgenden MUSS.
      case '-':
        factor = 0.1; /* fall through */
      case '+':
        p++;
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        break;
      default:
        value = 0.L;
        p = s;
        goto done;
    }

    while ((unsigned int)(*p - '0') < 10u) expo = 10 * expo + (*p++ - '0');

    while (1) {
      if (expo & 1) value *= factor;
      if ((expo >>= 1) == 0) break;
      factor *= factor;
    }
  }

done:
  if (endptr != NULL) *endptr = (char *)p;

  return value * sign;
}

// Convert char to an int in base `base`,
// `base` must be 10 or 16, return -1 on error.
int char2int(char ch, unsigned int base) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (base == 16) {
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  }
  return -1;
}

#define ldbltype long double
double strtod(const char *s, char **endptr) {
  register const char *p = s;
  register ldbltype value = 0.;
  int sign = +1;
  unsigned int base = 10;
  ldbltype base_inverse = (ldbltype)1 / (ldbltype)base;
  ldbltype factor;
  unsigned int expo;
  unsigned int has_digits = 0;

  while (isspace(*p)) p++;

  switch (*p) {
    case '-':
      sign = -1; /* fall through */
    case '+':
      p++;
    case '0':
      p++;
      if ((*p | 32) == 'x') {
        base = 16;
        base_inverse = (ldbltype)1 / (ldbltype)base;
        p++;
      } else {
        p--;
      }
    default:
      break;
  }

  unsigned int current_value;
  while ((current_value = char2int(*p, base)) != -1) {
    p++;
    value = value * base + current_value;
    has_digits = 1;
  }

  if (*p == '.') {
    factor = 1.;

    p++;
    while ((current_value = char2int(*p, base)) != -1) {
      p++;
      factor *= base_inverse;
      value += current_value * factor;
      has_digits = 1;
    }
  }

  if ((*p | 32) == 'e' && base == 10) {
    expo = 0;
    factor = 10.;

    switch (*++p) {  // ja hier weiß ich nicht, was mindestens nach einem
                     // 'E' folgenden MUSS.
      case '-':
        factor = 0.1; /* fall through */
      case '+':
        p++;
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        break;
      default:
        value = 0.;
        p = s;
        goto done;
    }

    while ((unsigned int)(*p - '0') < 10u) expo = 10 * expo + (*p++ - '0');

    while (1) {
      if (expo & 1) value *= factor;
      if ((expo >>= 1) == 0) break;
      factor *= factor;
    }
  }

  if ((*p | 32) == 'p' && base == 16) {
    // TODO: add specifier p support
    // https://cplusplus.com/reference/cstdlib/strtod/
    //  - A 0x or 0X prefix, then a sequence of hexadecimal digits (as in
    //  isxdigit) optionally containing a period which separates the whole
    //  and fractional number parts. Optionally followed by a power of 2
    //  exponent (a p or P character followed by an optional sign and a
    //  sequence of hexadecimal digits).
  }
done:
  if (endptr != NULL) {
    if (has_digits) {
      *endptr = (char *)p;
    } else {
      *endptr = (char *)s;
    }
  }

  return value * sign;
}

long double strtold(const char *s, char **endptr) {
  register const char *p = s;
  register long double value = 0.L;
  int sign = +1;
  long double factor;
  unsigned int expo;

  while (isspace(*p)) p++;

  switch (*p) {
    case '-':
      sign = -1; /* fall through */
    case '+':
      p++;
    default:
      break;
  }

  while ((unsigned int)(*p - '0') < 10u) value = value * 10 + (*p++ - '0');

  if (*p == '.') {
    factor = 1.;

    p++;
    while ((unsigned int)(*p - '0') < 10u) {
      factor *= 0.1;
      value += (*p++ - '0') * factor;
    }
  }

  if ((*p | 32) == 'e') {
    expo = 0;
    factor = 10.L;

    switch (*++p) {  // ja hier weiß ich nicht, was mindestens nach einem
                     // 'E' folgenden MUSS.
      case '-':
        factor = 0.1; /* fall through */
      case '+':
        p++;
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        break;
      default:
        value = 0.L;
        p = s;
        goto done;
    }

    while ((unsigned int)(*p - '0') < 10u) expo = 10 * expo + (*p++ - '0');

    while (1) {
      if (expo & 1) value *= factor;
      if ((expo >>= 1) == 0) break;
      factor *= factor;
    }
  }

done:
  if (endptr != NULL) *endptr = (char *)p;

  return value * sign;
}

void exit(int status) { ckb_exit(status); }

void abort(void) { ckb_exit(-1); }
