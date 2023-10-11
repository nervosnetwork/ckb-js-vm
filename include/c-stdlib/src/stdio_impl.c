#include "../my_stdio.h"

#include <stdlib.h>
#include <string.h>

FILE *stdin;
FILE *stdout;
FILE *stderr;

int ckb_exit(signed char code);

static int s_local_access_enabled = 0;
void enable_local_access(int b) { s_local_access_enabled = b; }

static int s_fs_access_enabled = 0;
void enable_fs_access(int b) { s_fs_access_enabled = b; }
int fs_access_enabled() { return s_fs_access_enabled; }

#define memory_barrier() asm volatile("fence" ::: "memory")

static inline long __internal_syscall(long n, long _a0, long _a1, long _a2, long _a3, long _a4, long _a5) {
    register long a0 asm("a0") = _a0;
    register long a1 asm("a1") = _a1;
    register long a2 asm("a2") = _a2;
    register long a3 asm("a3") = _a3;
    register long a4 asm("a4") = _a4;
    register long a5 asm("a5") = _a5;

#ifdef __riscv_32e
    register long syscall_id asm("t0") = n;
#else
    register long syscall_id asm("a7") = n;
#endif

    asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(syscall_id));
    /*
     * Syscalls might modify memory sent as pointer, adding a barrier here
     * ensures gcc won't do incorrect optimization.
     */
    memory_barrier();

    return a0;
}

#define ckb_syscall(n, a, b, c, d, e, f) \
    __internal_syscall(n, (long)(a), (long)(b), (long)(c), (long)(d), (long)(e), (long)(f))

#define NOT_IMPL(name)                                                 \
    do {                                                               \
        printf("The %s is not implemented in mocked_stdio.c ", #name); \
        ckb_exit(-1);                                                  \
    } while (0)

FILE *allocfile() {
    FILE *file = malloc(sizeof(FILE));
    file->file = 0;
    file->offset = 0;
    return file;
}

void freefile(FILE *file) {
    file->file->rc -= 1;
    free((void *)file);
}

int remove(const char *__filename) {
    NOT_IMPL(remove);
    return 0;
}

int rename(const char *__old, const char *__new) {
    NOT_IMPL(rename);
    return 0;
}

FILE *tmpfile(void) {
    NOT_IMPL(tmpfile);
    return 0;
}

char *tmpnam(char *__s) {
    NOT_IMPL(tmpnam);
    return 0;
}

char *tempnam(const char *__dir, const char *__pfx) {
    NOT_IMPL(tempnam);
    return 0;
}

int fclose(FILE *stream) {
    if (s_local_access_enabled) {
        return ckb_syscall(9009, stream, 0, 0, 0, 0, 0);
    }
    if (!fs_access_enabled()) {
        NOT_IMPL(fclose);
    }
    freefile(stream);
    return 0;
}

int fflush(FILE *__stream) {
    NOT_IMPL(fflush);
    return 0;
}

FILE *fopen(const char *path, const char *mode) {
    if (s_local_access_enabled) {
        return (void *)ckb_syscall(9003, path, mode, 0, 0, 0, 0);
    }

    if (!fs_access_enabled()) {
        NOT_IMPL(fopen);
    }

    FILE *file = allocfile();
    if (file == 0) {
        return 0;
    }

    int ret = ckb_get_file(path, &file->file);
    if (ret != 0) {
        return 0;
    }
    return file;
}

FILE *freopen(const char *path, const char *mode, FILE *stream) {
    if (s_local_access_enabled) {
        return (void *)ckb_syscall(9004, path, mode, stream, 0, 0, 0);
    }
    NOT_IMPL(freopen);
    return 0;
}

void setbuf(FILE *__stream, char *__buf) { NOT_IMPL(setbuf); }

int setvbuf(FILE *__stream, char *__buf, int __modes, size_t __n) {
    NOT_IMPL(setvbuf);
    return 0;
}

int fprintf(FILE *__stream, const char *__format, ...) {
    NOT_IMPL(fprintf);
    return 0;
}

int vfprintf(FILE *__s, const char *__format, ...) {
    NOT_IMPL(vfprintf);
    return 0;
}
int vsprintf(char *__s, const char *__format, ...) {
    NOT_IMPL(vsprintf);
    return 0;
}

int fscanf(FILE *__stream, const char *__format, ...) {
    NOT_IMPL(fscanf);
    return 0;
}

int scanf(const char *__format, ...) {
    NOT_IMPL(scanf);
    return 0;
}

int sscanf(const char *__s, const char *__format, ...) {
    NOT_IMPL(sscanf);
    return 0;
};

int fgetc(FILE *stream) {
    if (s_local_access_enabled) {
        return ckb_syscall(9008, stream, 0, 0, 0, 0, 0);
    }
    if (!fs_access_enabled()) {
        NOT_IMPL(fgetc);
    }
    if (stream == 0 || stream->file->rc == 0 || stream->offset == stream->file->size) {
        return -1;  // EOF
    }
    unsigned char *c = (unsigned char *)stream->file->content + stream->offset;
    stream->offset++;
    return *c;
}

int getc(FILE *stream) { return fgetc(stream); }

int getchar(void) {
    NOT_IMPL(getchar);
    return 0;
}

int fputc(int __c, FILE *__stream) {
    NOT_IMPL(fputc);
    return 0;
}

int putc(int __c, FILE *__stream) {
    NOT_IMPL(putc);
    return 0;
}

int putchar(int __c) {
    NOT_IMPL(putchar);
    return 0;
}

char *fgets(char *__s, int __n, FILE *__stream) {
    NOT_IMPL(fgets);
    return 0;
}

char *gets(char *__s) {
    NOT_IMPL(gets);
    return 0;
}

int getline(char **__lineptr, size_t *__n, FILE *__stream) {
    NOT_IMPL(getline);
    return 0;
}

int fputs(const char *__s, FILE *__stream) {
    NOT_IMPL(fputs);
    return 0;
}

int puts(const char *__s) {
    NOT_IMPL(puts);
    return 0;
}

int ungetc(int __c, FILE *__stream) {
    NOT_IMPL(ungetc);
    return 0;
}

int isvalidfile(FILE *stream) {
    if (stream == 0 || stream->file->rc == 0) {
        return 1;
    }
    return 0;
}

void mustbevaildfile(FILE *stream) {
    if (isvalidfile(stream) != 0) {
        ckb_exit(1);
    }
}

size_t fread(void *ptr, size_t size, size_t nitems, FILE *stream) {
    if (s_local_access_enabled) {
        return ckb_syscall(9005, ptr, size, nitems, stream, 0, 0);
    }
    if (!fs_access_enabled()) {
        NOT_IMPL(fread);
    }
    mustbevaildfile(stream);
    // TODO: How do we handle error here?
    if (stream->offset == stream->file->size) {
        return 0;
    }
    // TODO: handle the case size * nitems is greater than uint32_t max
    // handle size * ntimes overflowing
    uint32_t bytes_to_read = (uint32_t)size * (uint32_t)nitems;
    if (bytes_to_read > stream->file->size - stream->offset) {
        bytes_to_read = stream->file->size - stream->offset;
    }
    memcpy(ptr, stream->file->content + stream->offset, bytes_to_read);
    stream->offset = stream->offset + bytes_to_read;
    // The return value should be the number of items written to the ptr
    uint32_t s = size;
    return (bytes_to_read + s - 1) / s;
}

size_t fwrite(const void *__ptr, size_t __size, size_t __n, FILE *__s) {
    NOT_IMPL(fwrite);
    return 0;
}

int fseek(FILE *stream, long int offset, int whence) {
    if (s_local_access_enabled) {
        return ckb_syscall(9011, stream, offset, whence, 0, 0, 0);
    }
    NOT_IMPL(fseek);
    return 0;
}

long int ftell(FILE *stream) {
    if (s_local_access_enabled) {
        return ckb_syscall(9010, stream, 0, 0, 0, 0, 0);
    }
    NOT_IMPL(ftell);
    return 0;
}

void rewind(FILE *__stream) { NOT_IMPL(rewind); }

void clearerr(FILE *__stream) { NOT_IMPL(clearerr); }

int feof(FILE *stream) {
    if (s_local_access_enabled) {
        return ckb_syscall(9006, stream, 0, 0, 0, 0, 0);
    }
    if (!fs_access_enabled()) {
        NOT_IMPL(feof);
    }
    if (stream->offset == stream->file->size) {
        return 1;
    }
    return 0;
}

int ferror(FILE *stream) {
    if (s_local_access_enabled) {
        return ckb_syscall(9007, stream, 0, 0, 0, 0, 0);
    }
    if (!fs_access_enabled()) {
        NOT_IMPL(ferror);
    }
    if (stream == 0 || stream->file->rc == 0) {
        return 1;
    }
    return 0;
}

void perror(const char *__s) { NOT_IMPL(perror); }

int fileno(FILE *__stream) {
    NOT_IMPL(fileno);
    return 0;
}

FILE *popen(const char *__command, const char *__modes) {
    NOT_IMPL(popen);
    return 0;
}

int pclose(FILE *__stream) {
    NOT_IMPL(pclose);
    return 0;
}
