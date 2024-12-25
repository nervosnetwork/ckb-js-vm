/*
 * Yet another command line option parser
 * 
 * Copyright (c) 2021 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef CMDOPT_H
#define CMDOPT_H

#include "cutils.h"

typedef struct CMDOption CMDOption;

#define CMD_HAS_ARG (1 << 0)

typedef struct {
    const char *opt;
    int flags;
    const char *desc;
    const char *arg_desc;
} CMDOptDesc;

void __attribute__((noreturn, format(printf, 1, 2))) cmd_error(const char *fmt, ...);
CMDOption *cmdopt_init(const char *prog_name);
void cmdopt_add_desc(CMDOption *s, const CMDOptDesc *desc);
int cmdopt_parse(CMDOption *s, int argc, const char **argv);
void cmdopt_free(CMDOption *s);

void cmdopt_show_desc(const CMDOptDesc *desc);
const char *cmdopt_get(CMDOption *s, const char *opt);
BOOL cmdopt_has(CMDOption *s, const char *opt);
int cmdopt_get_int(CMDOption *s, const char *opt, int def_val);
float cmdopt_get_float(CMDOption *s, const char *opt, float def_val);
int cmdopt_get_count(CMDOption *s, const char *opt);

#endif /* CMDOPT_H */
