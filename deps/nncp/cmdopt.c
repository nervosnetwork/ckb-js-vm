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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "cutils.h"

#include "cmdopt.h"

typedef struct {
    const CMDOptDesc *desc;
    const char *optarg; /* NULL if no argument */
} CMDOpt;

struct CMDOption {
    int desc_count;
    int opt_count;
    int opt_size;
    CMDOpt *opt_tab;

    const CMDOptDesc *desc_tab[16];
};

static const char *cmd_prog_name;

void cmd_error(const char *fmt, ...)
{
    va_list ap;
    
    va_start(ap, fmt);
    fprintf(stderr, "%s: ", cmd_prog_name);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static const CMDOptDesc *find_opt(CMDOption *s, const char *opt)
{
    const CMDOptDesc *d;
    int i;
    size_t opt_len, len;
    const char *p, *r;
    
    opt_len = strlen(opt);
    for(i = 0; i < s->desc_count; i++) {
        for(d = s->desc_tab[i]; d->opt != NULL; d++) {
            p = d->opt;
            for(;;) {
                r = strchr(p, ',');
                if (r)
                    len = r - p;
                else
                    len = strlen(p);
                if (len == opt_len && !memcmp(p, opt, opt_len))
                    return d;
                if (!r)
                    break;
                p = r + 1;
            }
        }
    }
    return NULL;
}

static void add_opt(CMDOption *s, const CMDOptDesc *d,
                    const char *optarg)
{
    int new_size;
    CMDOpt *cs;

    if ((s->opt_count + 1) > s->opt_size) {
        new_size = max_int(4, max_int(s->opt_count + 1,
                                      s->opt_size + s->opt_size / 2));
        s->opt_tab = realloc(s->opt_tab, new_size * sizeof(s->opt_tab[0]));
        s->opt_size = new_size;
    }
    cs = &s->opt_tab[s->opt_count++];
    cs->desc = d;
    cs->optarg = optarg;
}

CMDOption *cmdopt_init(const char *prog_name)
{
    CMDOption *s;

    cmd_prog_name = prog_name;
    s = malloc(sizeof(*s));
    memset(s, 0, sizeof(*s));
    return s;
}

void cmdopt_add_desc(CMDOption *s, const CMDOptDesc *desc)
{
    if (s->desc_count >= countof(s->desc_tab))
        cmd_error("too many cmd desc");
    s->desc_tab[s->desc_count++] = desc;
}

int cmdopt_parse(CMDOption *s, int argc, const char **argv)
{
    const char *arg, *optarg;
    const char **param;
    char optbuf[2];
    const CMDOptDesc *d;
    int optind, c, param_len, i, arg_pos;

    param = malloc(sizeof(param[0]) * argc);
    param_len = 0;
    arg_pos = 1;
    for(optind = 1; optind < argc; ) {
        arg = argv[optind];
        if (*arg != '-') {
            param[param_len++] = arg;
            optind++;
        } else {
            argv[arg_pos++] = arg;
            optind++;
            arg++;
            if (*arg == '-') {
                arg++;
                if (*arg == '\0') {
                    /* '--' stops option parsing */
                    while (optind < argc) {
                        param[param_len++] = argv[optind++];
                    }
                    break;
                }
                if (strlen(arg) > 1)
                    d = find_opt(s, arg);
                else
                    d = NULL;
                if (!d)
                    cmd_error("unknown option: '--%s'", arg);
                if (d->flags & CMD_HAS_ARG) {
                    if (optind >= argc)
                        cmd_error("option '--%s' must have an argument", arg);
                    optarg = argv[optind++];
                    argv[arg_pos++] = optarg;
                } else {
                    optarg = NULL;
                }
                add_opt(s, d, optarg);
            } else {
                while (*arg != '\0') {
                    c = *arg++;
                    optbuf[0] = c;
                    optbuf[1] = '\0';
                    d = find_opt(s, optbuf);
                    if (!d)
                        cmd_error("unknown option: '-%c'", c);
                    if (d->flags & CMD_HAS_ARG) {
                        /* arguments expected */
                        if (*arg != '\0') {
                            optarg = arg;
                        } else {
                            if (optind >= argc)
                                cmd_error("option '-%c' must have an argument", c);
                            optarg = argv[optind++];
                            argv[arg_pos++] = optarg;
                        }
                    } else {
                        optarg = NULL;
                    }
                    add_opt(s, d, optarg);
                }
            }
        }
    }

    /* put the parameters after the options */
    optind = arg_pos;
    for(i = 0; i < param_len; i++)
        argv[arg_pos++] = param[i];
    assert(arg_pos == argc);
    free(param);
#if 0
    {
        printf("reorderd argv: ");
        for(i = 0; i < argc; i++) {
            if (i == optind)
                printf(" |");
            printf(" %s", argv[i]);
        }
        printf("\n");
    }
#endif    
    return optind;
}

void cmdopt_show_desc(const CMDOptDesc *desc)
{
    const CMDOptDesc *d;
    int col, pos, opt_width;
    size_t len, i;
    const char *p, *r;
    
    opt_width = 24;
    for(d = desc; d->opt != NULL; d++) {
        col = 0;
        p = d->opt;
        for(;;) {
            r = strchr(p, ',');
            if (r)
                len = r - p;
            else
                len = strlen(p);
            if (p != d->opt) {
                putchar(' ');
                col++;
            }
            putchar('-');
            if (len > 1) {
                putchar('-');
                col++;
            }
            for(i = 0; i < len; i++)
                putchar(p[i]);
            col += len + 1;
            
            if (!r)
                break;
            p = r + 1;
        }
        
        if (d->flags & CMD_HAS_ARG) {
            if (d->arg_desc)
                col += printf(" %s", d->arg_desc);
            else
                col += printf(" arg");
        }
        if (col < opt_width) {
            pos = opt_width;
        } else {
            pos = ((col - opt_width + 8) & ~7) + opt_width;
        }
        while (col < pos) {
            putchar(' ');
            col++;
        }
        printf("%s\n", d->desc);
    }
}

static const char *cmdopt_get_internal(CMDOption *s, const char *opt,
                                       BOOL has_arg, int *pcount)
{
    const CMDOptDesc *d;
    int i, count;

    d = find_opt(s, opt);
    if (!d) {
        cmd_error("option '-%s%s' does not exist",
                  strlen(opt) > 1 ? "-" : "", opt);
    }

    /* check the argument consistency */
    if (((d->flags & CMD_HAS_ARG) != 0) != has_arg) {
        if (d->flags & CMD_HAS_ARG) {
            cmd_error("option '-%s%s' has an argument",
                      strlen(opt) > 1 ? "-" : "", opt);
        } else {
            cmd_error("option '-%s%s' does not have an argument",
                      strlen(opt) > 1 ? "-" : "", opt);
        }
    }
    
    /* the last option is used */
    count = 0;
    for(i = s->opt_count - 1; i >= 0; i--) {
        CMDOpt *cs = &s->opt_tab[i];
        if (cs->desc == d) {
            if (has_arg) {
                if (pcount)
                    *pcount = 1;
                return cs->optarg;
            } else {
                count++;
            }
        }
    }
    if (pcount)
        *pcount = count;
    if (count != 0)
        return "";
    else
        return NULL;
}

const char *cmdopt_get(CMDOption *s, const char *opt)
{
    return cmdopt_get_internal(s, opt, TRUE, NULL);
}

BOOL cmdopt_has(CMDOption *s, const char *opt)
{
    return (cmdopt_get_internal(s, opt, FALSE, NULL) != NULL);
}

int cmdopt_get_count(CMDOption *s, const char *opt)
{
    int count;
    cmdopt_get_internal(s, opt, FALSE, &count);
    return count;
}

int cmdopt_get_int(CMDOption *s, const char *opt, int def_val)
{
    const char *str, *p;
    double d;
    int val;
    
    str = cmdopt_get(s, opt);
    if (!str)
        return def_val;
    d = strtod(str, (char **)&p);
    val = (int)d;
    if (*p != '\0' || d != (double)val)
        cmd_error("option -%s%s expects an integer",
                  strlen(opt) > 1 ? "-" : "", opt);
    return val;
}

float cmdopt_get_float(CMDOption *s, const char *opt, float def_val)
{
    const char *str, *p;
    float val;
    
    str = cmdopt_get(s, opt);
    if (!str)
        return def_val;
    val = strtod(str, (char **)&p);
    if (*p != '\0')
        cmd_error("option -%s%s expects a floating point value",
                  strlen(opt) > 1 ? "-" : "", opt);
    return val;
}

void cmdopt_free(CMDOption *s)
{
    free(s->opt_tab);
    free(s);
}
