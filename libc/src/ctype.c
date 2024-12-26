#include <ctype.h>

int islower(int c) { return (unsigned)c - 'a' < 26; }

int isupper(int c) { return (unsigned)c - 'A' < 26; }

int tolower(int c) {
    if (isupper(c)) return c | 32;
    return c;
}

int toupper(int c) {
    if (islower(c)) return c & 0x5f;
    return c;
}
