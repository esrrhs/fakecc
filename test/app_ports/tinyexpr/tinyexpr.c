/*
 * TINYEXPR - Tiny recursive descent parser and evaluation engine in C
 * FakeCC port
 *
 * Copyright (c) 2015-2026 Lewis Van Winkle
 * Adapted for FakeCC by FakeCC contributors
 *
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

package tinyexpr;

import runtime;

extern double fabs(double x);
extern double acos(double x);
extern double asin(double x);
extern double atan(double x);
extern double atan2(double y, double x);
extern double ceil(double x);
extern double cos(double x);
extern double cosh(double x);
extern double exp(double x);
extern double floor(double x);
extern double log(double x);
extern double log10(double x);
extern double pow(double x, double y);
extern double sin(double x);
extern double sinh(double x);
extern double sqrt(double x);
extern double tan(double x);
extern double tanh(double x);
extern double fmod(double x, double y);

enum {
    TE_VARIABLE = 0,

    TE_FUNCTION0 = 8, TE_FUNCTION1, TE_FUNCTION2, TE_FUNCTION3,
    TE_FUNCTION4, TE_FUNCTION5, TE_FUNCTION6, TE_FUNCTION7,

    TE_CLOSURE0 = 16, TE_CLOSURE1, TE_CLOSURE2, TE_CLOSURE3,
    TE_CLOSURE4, TE_CLOSURE5, TE_CLOSURE6, TE_CLOSURE7,

    TE_FLAG_PURE = 32
};

enum {
    TE_CONSTANT = 1,
    TOK_NULL = TE_CLOSURE7 + 1,
    TOK_ERROR, TOK_END, TOK_SEP,
    TOK_OPEN, TOK_CLOSE, TOK_NUMBER, TOK_VARIABLE, TOK_INFIX
};

enum { TE_MAX_DEPTH = 512 };

typedef struct te_expr {
    int type;
    union {
        double value;
        const double *bound;
        const void *function;
    };
    void *parameters[];
} te_expr;

typedef struct te_expr te_expr;

typedef struct te_variable {
    const char *name;
    const void *address;
    int type;
    void *context;
} te_variable;

typedef struct te_variable te_variable;

typedef struct state {
    const char *start;
    const char *next;
    int type;
    union {
        double value;
        const double *bound;
        const void *function;
    };
    void *context;

    const te_variable *lookup;
    int lookup_len;
    int applied_unary;
    int depth;
} state;

typedef double (*te_fun0)(void);
typedef double (*te_fun1)(double);
typedef double (*te_fun2)(double, double);
typedef double (*te_fun3)(double, double, double);
typedef double (*te_fun4)(double, double, double, double);
typedef double (*te_fun5)(double, double, double, double, double);
typedef double (*te_fun6)(double, double, double, double, double, double);
typedef double (*te_fun7)(double, double, double, double, double, double, double);

typedef double (*te_clo0)(void *);
typedef double (*te_clo1)(void *, double);
typedef double (*te_clo2)(void *, double, double);
typedef double (*te_clo3)(void *, double, double, double);
typedef double (*te_clo4)(void *, double, double, double, double);
typedef double (*te_clo5)(void *, double, double, double, double, double);
typedef double (*te_clo6)(void *, double, double, double, double, double, double);
typedef double (*te_clo7)(void *, double, double, double, double, double, double, double);

void te_free(te_expr *n);
double te_eval(const te_expr *n);
double te_value(const te_expr *n);
te_expr *te_compile(const char *expression, const te_variable *variables, int var_count, int *error);
double te_interp(const char *expression, int *error);
void te_print(const te_expr *n);

static int type_mask(int type) { return type & 0x0000001F; }
static int is_pure(int type) { return (type & TE_FLAG_PURE) != 0; }
static int is_closure(int type) { return (type & TE_CLOSURE0) != 0; }
static int arity(int type) {
    if (type & (TE_FUNCTION0 | TE_CLOSURE0))
        return type & 0x00000007;
    return 0;
}

static double nan_value(void) { return __builtin_nan(""); }
static double inf_value(void) { return __builtin_inf(); }

static te_expr *new_expr(int type, const te_expr **parameters) {
    int ar = arity(type);
    int psize = (int)sizeof(void *) * ar;
    int extra = is_closure(type) ? (int)sizeof(void *) : 0;
    int size = (int)sizeof(te_expr) + psize + extra;
    te_expr *ret = runtime.malloc(size);
    if (!ret) return 0;
    runtime.memset(ret, 0, size);
    if (ar && parameters)
        runtime.memcpy(ret->parameters, parameters, psize);
    ret->type = type;
    ret->bound = 0;
    return ret;
}

static te_expr *new_expr0(int type) {
    return new_expr(type, 0);
}

static te_expr *new_expr1(int type, te_expr *a) {
    const te_expr *ps[1];
    ps[0] = a;
    return new_expr(type, ps);
}

static te_expr *new_expr2(int type, te_expr *a, te_expr *b) {
    const te_expr *ps[2];
    ps[0] = a;
    ps[1] = b;
    return new_expr(type, ps);
}

static void te_free_parameters(te_expr *n) {
    if (!n) return;
    switch (type_mask(n->type)) {
        case TE_FUNCTION7: case TE_CLOSURE7: te_free(n->parameters[6]); /* fall through */
        case TE_FUNCTION6: case TE_CLOSURE6: te_free(n->parameters[5]); /* fall through */
        case TE_FUNCTION5: case TE_CLOSURE5: te_free(n->parameters[4]); /* fall through */
        case TE_FUNCTION4: case TE_CLOSURE4: te_free(n->parameters[3]); /* fall through */
        case TE_FUNCTION3: case TE_CLOSURE3: te_free(n->parameters[2]); /* fall through */
        case TE_FUNCTION2: case TE_CLOSURE2: te_free(n->parameters[1]); /* fall through */
        case TE_FUNCTION1: case TE_CLOSURE1: te_free(n->parameters[0]);
    }
}

void te_free(te_expr *n) {
    if (!n) return;
    te_free_parameters(n);
    runtime.free(n);
}

/* Accessor so importers can read the folded constant; anonymous union
 * members are not visible across FakeCC packages. */
double te_value(const te_expr *n) {
    return n->value;
}

static double pi(void) { return 3.14159265358979323846; }
static double e(void) { return 2.71828182845904523536; }

static double fac(double a) {
    unsigned int ua;
    unsigned long result;
    unsigned long i;
    if (!(a >= 0.0))
        return nan_value();
    if (a > 4294967295.0)
        return inf_value();
    ua = (unsigned int)a;
    result = 1;
    for (i = 1; i <= ua; i++) {
        if (i > ((unsigned long)-1) / result)
            return inf_value();
        result *= i;
    }
    return (double)result;
}

static double ncr(double n, double r) {
    unsigned long un, ur, i, result;
    if (!(n >= 0.0) || !(r >= 0.0) || n < r) return nan_value();
    if (n > 4294967295.0 || r > 4294967295.0) return inf_value();
    un = (unsigned int)n;
    ur = (unsigned int)r;
    result = 1;
    if (ur > un / 2) ur = un - ur;
    for (i = 1; i <= ur; i++) {
        if (result > ((unsigned long)-1) / (un - ur + i))
            return inf_value();
        result *= un - ur + i;
        result /= i;
    }
    return (double)result;
}

static double npr(double n, double r) { return ncr(n, r) * fac(r); }

static const te_variable functions[] = {
    {"abs", (const void *)fabs, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"acos", (const void *)acos, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"asin", (const void *)asin, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"atan", (const void *)atan, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"atan2", (const void *)atan2, TE_FUNCTION2 | TE_FLAG_PURE, 0},
    {"ceil", (const void *)ceil, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"cos", (const void *)cos, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"cosh", (const void *)cosh, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"e", (const void *)e, TE_FUNCTION0 | TE_FLAG_PURE, 0},
    {"exp", (const void *)exp, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"fac", (const void *)fac, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"floor", (const void *)floor, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"ln", (const void *)log, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"log", (const void *)log10, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"log10", (const void *)log10, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"ncr", (const void *)ncr, TE_FUNCTION2 | TE_FLAG_PURE, 0},
    {"npr", (const void *)npr, TE_FUNCTION2 | TE_FLAG_PURE, 0},
    {"pi", (const void *)pi, TE_FUNCTION0 | TE_FLAG_PURE, 0},
    {"pow", (const void *)pow, TE_FUNCTION2 | TE_FLAG_PURE, 0},
    {"sin", (const void *)sin, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"sinh", (const void *)sinh, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"sqrt", (const void *)sqrt, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"tan", (const void *)tan, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"tanh", (const void *)tanh, TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {0, 0, 0, 0}
};

static const te_variable *find_builtin(const char *name, int len) {
    int imin = 0;
    int imax = (int)(sizeof(functions) / sizeof(te_variable)) - 2;

    while (imax >= imin) {
        const int i = imin + ((imax - imin) / 2);
        int c = runtime.strncmp(name, functions[i].name, len);
        if (!c) c = '\0' - functions[i].name[len];
        if (c == 0)
            return functions + i;
        else if (c > 0)
            imin = i + 1;
        else
            imax = i - 1;
    }
    return 0;
}

static const te_variable *find_lookup(const state *s, const char *name, int len) {
    int iters;
    const te_variable *var;
    if (!s->lookup) return 0;
    for (var = s->lookup, iters = s->lookup_len; iters; ++var, --iters) {
        if (runtime.strncmp(name, var->name, len) == 0 && var->name[len] == '\0')
            return var;
    }
    return 0;
}

static double add(double a, double b) { return a + b; }
static double sub(double a, double b) { return a - b; }
static double mul(double a, double b) { return a * b; }
static double divide(double a, double b) { return a / b; }
static double negate(double a) { return -a; }
static double comma(double a, double b) { return b; }

static double greater(double a, double b) { return a > b; }
static double greater_eq(double a, double b) { return a >= b; }
static double lower(double a, double b) { return a < b; }
static double lower_eq(double a, double b) { return a <= b; }
static double equal(double a, double b) { return a == b; }
static double not_equal(double a, double b) { return a != b; }
static double logical_and(double a, double b) { return a != 0.0 && b != 0.0; }
static double logical_or(double a, double b) { return a != 0.0 || b != 0.0; }
static double logical_not(double a) { return a == 0.0; }
static double logical_notnot(double a) { return a != 0.0; }
static double negate_logical_not(double a) { return -(a == 0.0); }
static double negate_logical_notnot(double a) { return -(a != 0.0); }

static double parse_number(state *s) {
    const char *p = s->next;
    double value = 0.0;

    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X') &&
            runtime.isxdigit((unsigned char)p[2])) {
        p += 2;
        while (runtime.isxdigit((unsigned char)p[0])) {
            const char c = p[0];
            const int d = runtime.isdigit((unsigned char)c) ? c - '0' :
                (c >= 'a' ? c - 'a' + 10 : c - 'A' + 10);
            value = value * 16.0 + d;
            p++;
        }
    } else {
        int exponent = 0, digits = 0;

        while (runtime.isdigit((unsigned char)p[0])) {
            value = value * 10.0 + (p[0] - '0');
            p++; digits++;
        }

        if (p[0] == '.') {
            p++;
            while (runtime.isdigit((unsigned char)p[0])) {
                value = value * 10.0 + (p[0] - '0');
                p++; digits++; exponent--;
            }
        }

        if (digits == 0) {
            s->type = TOK_ERROR;
            return 0.0;
        }

        if (p[0] == 'e' || p[0] == 'E') {
            const char *ep = p + 1;
            int esign = 1, eexp = 0;

            if (ep[0] == '+') {
                ep++;
            } else if (ep[0] == '-') {
                esign = -1;
                ep++;
            }

            if (runtime.isdigit((unsigned char)ep[0])) {
                while (runtime.isdigit((unsigned char)ep[0])) {
                    if (eexp < 100000000) eexp = eexp * 10 + (ep[0] - '0');
                    ep++;
                }
                exponent += esign * eexp;
                p = ep;
            }
        }

        if (exponent && value != 0.0) value *= pow(10.0, exponent);
    }

    s->type = TOK_NUMBER;
    s->next = p;
    return value;
}

static void next_token(state *s) {
    s->type = TOK_NULL;

    do {
        if (!*s->next) {
            s->type = TOK_END;
            return;
        }

        if ((s->next[0] >= '0' && s->next[0] <= '9') || s->next[0] == '.') {
            s->value = parse_number(s);
        } else {
            if (runtime.isalpha((unsigned char)s->next[0])) {
                const char *start = s->next;
                const te_variable *var;
                while (runtime.isalpha((unsigned char)s->next[0]) ||
                       runtime.isdigit((unsigned char)s->next[0]) ||
                       (s->next[0] == '_'))
                    s->next++;

                var = find_lookup(s, start, (int)(s->next - start));
                if (!var) var = find_builtin(start, (int)(s->next - start));

                if (!var) {
                    s->type = TOK_ERROR;
                } else {
                    switch (type_mask(var->type)) {
                        case TE_VARIABLE:
                            s->type = TOK_VARIABLE;
                            s->bound = var->address;
                            break;
                        case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:
                        case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
                            s->context = var->context;
                            /* fall through */
                        case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:
                        case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
                            s->type = var->type;
                            s->function = var->address;
                            break;
                    }
                }
            } else {
                switch (s->next++[0]) {
                    case '+': s->type = TOK_INFIX; s->function = (const void *)add; break;
                    case '-': s->type = TOK_INFIX; s->function = (const void *)sub; break;
                    case '*': s->type = TOK_INFIX; s->function = (const void *)mul; break;
                    case '/': s->type = TOK_INFIX; s->function = (const void *)divide; break;
                    case '^': s->type = TOK_INFIX; s->function = (const void *)pow; break;
                    case '%': s->type = TOK_INFIX; s->function = (const void *)fmod; break;
                    case '!':
                        if (s->next++[0] == '=') {
                            s->type = TOK_INFIX; s->function = (const void *)not_equal;
                        } else {
                            s->next--;
                            s->type = TOK_INFIX; s->function = (const void *)logical_not;
                        }
                        break;
                    case '=':
                        if (s->next++[0] == '=') {
                            s->type = TOK_INFIX; s->function = (const void *)equal;
                        } else {
                            s->next--;
                            s->type = TOK_ERROR;
                        }
                        break;
                    case '<':
                        if (s->next++[0] == '=') {
                            s->type = TOK_INFIX; s->function = (const void *)lower_eq;
                        } else {
                            s->next--;
                            s->type = TOK_INFIX; s->function = (const void *)lower;
                        }
                        break;
                    case '>':
                        if (s->next++[0] == '=') {
                            s->type = TOK_INFIX; s->function = (const void *)greater_eq;
                        } else {
                            s->next--;
                            s->type = TOK_INFIX; s->function = (const void *)greater;
                        }
                        break;
                    case '&':
                        if (s->next++[0] == '&') {
                            s->type = TOK_INFIX; s->function = (const void *)logical_and;
                        } else {
                            s->next--;
                            s->type = TOK_ERROR;
                        }
                        break;
                    case '|':
                        if (s->next++[0] == '|') {
                            s->type = TOK_INFIX; s->function = (const void *)logical_or;
                        } else {
                            s->next--;
                            s->type = TOK_ERROR;
                        }
                        break;
                    case '(': s->type = TOK_OPEN; break;
                    case ')': s->type = TOK_CLOSE; break;
                    case ',': s->type = TOK_SEP; break;
                    case ' ': case '\t': case '\n': case '\r': break;
                    default: s->type = TOK_ERROR; break;
                }
            }
        }
    } while (s->type == TOK_NULL);
}

static te_expr *list(state *s);
static te_expr *expr(state *s);
static te_expr *power(state *s);
static te_expr *base_impl(state *s);

static te_expr *base(state *s) {
    te_expr *ret;
    if (s->depth >= TE_MAX_DEPTH) {
        s->type = TOK_ERROR;
        return 0;
    }
    s->depth++;
    ret = base_impl(s);
    s->depth--;
    return ret;
}

static te_expr *base_impl(state *s) {
    te_expr *ret;
    int ar;

    switch (type_mask(s->type)) {
        case TOK_NUMBER:
            ret = new_expr0(TE_CONSTANT);
            if (!ret) return 0;
            ret->value = s->value;
            next_token(s);
            break;

        case TOK_VARIABLE:
            ret = new_expr0(TE_VARIABLE);
            if (!ret) return 0;
            ret->bound = s->bound;
            next_token(s);
            break;

        case TE_FUNCTION0:
        case TE_CLOSURE0:
            ret = new_expr0(s->type);
            if (!ret) return 0;
            ret->function = s->function;
            if (is_closure(s->type)) ret->parameters[0] = s->context;
            next_token(s);
            if (s->type == TOK_OPEN) {
                next_token(s);
                if (s->type != TOK_CLOSE)
                    s->type = TOK_ERROR;
                else
                    next_token(s);
            }
            break;

        case TE_FUNCTION1:
        case TE_CLOSURE1:
            ret = new_expr0(s->type);
            if (!ret) return 0;
            ret->function = s->function;
            if (is_closure(s->type)) ret->parameters[1] = s->context;
            next_token(s);
            ret->parameters[0] = power(s);
            if (!ret->parameters[0]) {
                te_free(ret);
                return 0;
            }
            break;

        case TE_FUNCTION2: case TE_FUNCTION3: case TE_FUNCTION4:
        case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
        case TE_CLOSURE2: case TE_CLOSURE3: case TE_CLOSURE4:
        case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
            ar = arity(s->type);
            ret = new_expr0(s->type);
            if (!ret) return 0;
            ret->function = s->function;
            if (is_closure(s->type)) ret->parameters[ar] = s->context;
            next_token(s);
            if (s->type != TOK_OPEN) {
                s->type = TOK_ERROR;
            } else {
                int i;
                for (i = 0; i < ar; i++) {
                    next_token(s);
                    ret->parameters[i] = expr(s);
                    if (!ret->parameters[i]) {
                        te_free(ret);
                        return 0;
                    }
                    if (s->type != TOK_SEP)
                        break;
                }
                if (s->type != TOK_CLOSE || i != ar - 1)
                    s->type = TOK_ERROR;
                else
                    next_token(s);
            }
            break;

        case TOK_OPEN:
            next_token(s);
            ret = list(s);
            if (!ret) return 0;
            if (s->type != TOK_CLOSE)
                s->type = TOK_ERROR;
            else
                next_token(s);
            break;

        default:
            ret = new_expr0(0);
            if (!ret) return 0;
            s->type = TOK_ERROR;
            ret->value = nan_value();
            break;
    }

    return ret;
}

static te_expr *power(state *s) {
    int sign = 1;
    int logical = 0;
    te_expr *ret;

    while (s->type == TOK_INFIX && (s->function == (const void *)add || s->function == (const void *)sub)) {
        if (s->function == (const void *)sub) sign = -sign;
        next_token(s);
    }

    while (s->type == TOK_INFIX && (s->function == (const void *)add ||
                                    s->function == (const void *)sub ||
                                    s->function == (const void *)logical_not)) {
        if (s->function == (const void *)logical_not) {
            if (logical == 0)
                logical = -1;
            else
                logical = -logical;
        }
        next_token(s);
    }

    if (sign == 1) {
        if (logical == 0) {
            ret = base(s);
        } else {
            te_expr *b = base(s);
            if (!b) return 0;
            ret = new_expr1(TE_FUNCTION1 | TE_FLAG_PURE, b);
            if (!ret) {
                te_free(b);
                return 0;
            }
            if (logical == -1)
                ret->function = (const void *)logical_not;
            else
                ret->function = (const void *)logical_notnot;
        }
    } else {
        te_expr *b = base(s);
        if (!b) return 0;
        ret = new_expr1(TE_FUNCTION1 | TE_FLAG_PURE, b);
        if (!ret) {
            te_free(b);
            return 0;
        }
        if (logical == 0)
            ret->function = (const void *)negate;
        else if (logical == -1)
            ret->function = (const void *)negate_logical_not;
        else
            ret->function = (const void *)negate_logical_notnot;
    }

    s->applied_unary = (sign != 1 || logical != 0);
    return ret;
}

static te_expr *factor(state *s) {
    te_expr *ret = power(s);
    if (!ret) return 0;

    while (s->type == TOK_INFIX && (s->function == (const void *)pow)) {
        const void *t = s->function;
        te_expr *p;
        te_expr *prev;
        next_token(s);
        p = power(s);
        if (!p) {
            te_free(ret);
            return 0;
        }
        prev = ret;
        ret = new_expr2(TE_FUNCTION2 | TE_FLAG_PURE, ret, p);
        if (!ret) {
            te_free(p);
            te_free(prev);
            return 0;
        }
        ret->function = t;
    }
    return ret;
}

static te_expr *term(state *s) {
    te_expr *ret = factor(s);
    if (!ret) return 0;

    while (s->type == TOK_INFIX && (s->function == (const void *)mul ||
                                    s->function == (const void *)divide ||
                                    s->function == (const void *)fmod)) {
        const void *t = s->function;
        te_expr *f;
        te_expr *prev;
        next_token(s);
        f = factor(s);
        if (!f) {
            te_free(ret);
            return 0;
        }
        prev = ret;
        ret = new_expr2(TE_FUNCTION2 | TE_FLAG_PURE, ret, f);
        if (!ret) {
            te_free(f);
            te_free(prev);
            return 0;
        }
        ret->function = t;
    }
    return ret;
}

static te_expr *sum_expr(state *s) {
    te_expr *ret = term(s);
    if (!ret) return 0;

    while (s->type == TOK_INFIX && (s->function == (const void *)add || s->function == (const void *)sub)) {
        const void *t = s->function;
        te_expr *te;
        te_expr *prev;
        next_token(s);
        te = term(s);
        if (!te) {
            te_free(ret);
            return 0;
        }
        prev = ret;
        ret = new_expr2(TE_FUNCTION2 | TE_FLAG_PURE, ret, te);
        if (!ret) {
            te_free(te);
            te_free(prev);
            return 0;
        }
        ret->function = t;
    }
    return ret;
}

static te_expr *rel_expr(state *s) {
    te_expr *ret = sum_expr(s);
    if (!ret) return 0;

    while (s->type == TOK_INFIX && (s->function == (const void *)greater ||
                                    s->function == (const void *)greater_eq ||
                                    s->function == (const void *)lower ||
                                    s->function == (const void *)lower_eq)) {
        const void *t = s->function;
        te_expr *e;
        te_expr *prev;
        next_token(s);
        e = sum_expr(s);
        if (!e) {
            te_free(ret);
            return 0;
        }
        prev = ret;
        ret = new_expr2(TE_FUNCTION2 | TE_FLAG_PURE, ret, e);
        if (!ret) {
            te_free(e);
            te_free(prev);
            return 0;
        }
        ret->function = t;
    }
    return ret;
}

static te_expr *eq_expr(state *s) {
    te_expr *ret = rel_expr(s);
    if (!ret) return 0;

    while (s->type == TOK_INFIX && (s->function == (const void *)equal ||
                                    s->function == (const void *)not_equal)) {
        const void *t = s->function;
        te_expr *e;
        te_expr *prev;
        next_token(s);
        e = rel_expr(s);
        if (!e) {
            te_free(ret);
            return 0;
        }
        prev = ret;
        ret = new_expr2(TE_FUNCTION2 | TE_FLAG_PURE, ret, e);
        if (!ret) {
            te_free(e);
            te_free(prev);
            return 0;
        }
        ret->function = t;
    }
    return ret;
}

static te_expr *and_expr(state *s) {
    te_expr *ret = eq_expr(s);
    if (!ret) return 0;

    while (s->type == TOK_INFIX && s->function == (const void *)logical_and) {
        te_expr *e;
        te_expr *prev;
        next_token(s);
        e = eq_expr(s);
        if (!e) {
            te_free(ret);
            return 0;
        }
        prev = ret;
        ret = new_expr2(TE_FUNCTION2 | TE_FLAG_PURE, ret, e);
        if (!ret) {
            te_free(e);
            te_free(prev);
            return 0;
        }
        ret->function = (const void *)logical_and;
    }
    return ret;
}

static te_expr *expr(state *s) {
    te_expr *ret = and_expr(s);
    if (!ret) return 0;

    while (s->type == TOK_INFIX && s->function == (const void *)logical_or) {
        te_expr *e;
        te_expr *prev;
        next_token(s);
        e = and_expr(s);
        if (!e) {
            te_free(ret);
            return 0;
        }
        prev = ret;
        ret = new_expr2(TE_FUNCTION2 | TE_FLAG_PURE, ret, e);
        if (!ret) {
            te_free(e);
            te_free(prev);
            return 0;
        }
        ret->function = (const void *)logical_or;
    }
    return ret;
}

static te_expr *list(state *s) {
    te_expr *ret = expr(s);
    if (!ret) return 0;

    while (s->type == TOK_SEP) {
        te_expr *e;
        te_expr *prev;
        next_token(s);
        e = expr(s);
        if (!e) {
            te_free(ret);
            return 0;
        }
        prev = ret;
        ret = new_expr2(TE_FUNCTION2 | TE_FLAG_PURE, ret, e);
        if (!ret) {
            te_free(e);
            te_free(prev);
            return 0;
        }
        ret->function = (const void *)comma;
    }
    return ret;
}

double te_eval(const te_expr *n) {
    if (!n) return nan_value();

    switch (type_mask(n->type)) {
        case TE_CONSTANT: return n->value;
        case TE_VARIABLE: return *n->bound;

        case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:
        case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
            switch (arity(n->type)) {
                case 0: return ((te_fun0)n->function)();
                case 1: return ((te_fun1)n->function)(te_eval(n->parameters[0]));
                case 2: return ((te_fun2)n->function)(te_eval(n->parameters[0]), te_eval(n->parameters[1]));
                case 3: return ((te_fun3)n->function)(te_eval(n->parameters[0]), te_eval(n->parameters[1]), te_eval(n->parameters[2]));
                case 4: return ((te_fun4)n->function)(te_eval(n->parameters[0]), te_eval(n->parameters[1]), te_eval(n->parameters[2]), te_eval(n->parameters[3]));
                case 5: return ((te_fun5)n->function)(te_eval(n->parameters[0]), te_eval(n->parameters[1]), te_eval(n->parameters[2]), te_eval(n->parameters[3]), te_eval(n->parameters[4]));
                case 6: return ((te_fun6)n->function)(te_eval(n->parameters[0]), te_eval(n->parameters[1]), te_eval(n->parameters[2]), te_eval(n->parameters[3]), te_eval(n->parameters[4]), te_eval(n->parameters[5]));
                case 7: return ((te_fun7)n->function)(te_eval(n->parameters[0]), te_eval(n->parameters[1]), te_eval(n->parameters[2]), te_eval(n->parameters[3]), te_eval(n->parameters[4]), te_eval(n->parameters[5]), te_eval(n->parameters[6]));
                default: return nan_value();
            }

        case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:
        case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
            switch (arity(n->type)) {
                case 0: return ((te_clo0)n->function)(n->parameters[0]);
                case 1: return ((te_clo1)n->function)(n->parameters[1], te_eval(n->parameters[0]));
                case 2: return ((te_clo2)n->function)(n->parameters[2], te_eval(n->parameters[0]), te_eval(n->parameters[1]));
                case 3: return ((te_clo3)n->function)(n->parameters[3], te_eval(n->parameters[0]), te_eval(n->parameters[1]), te_eval(n->parameters[2]));
                case 4: return ((te_clo4)n->function)(n->parameters[4], te_eval(n->parameters[0]), te_eval(n->parameters[1]), te_eval(n->parameters[2]), te_eval(n->parameters[3]));
                case 5: return ((te_clo5)n->function)(n->parameters[5], te_eval(n->parameters[0]), te_eval(n->parameters[1]), te_eval(n->parameters[2]), te_eval(n->parameters[3]), te_eval(n->parameters[4]));
                case 6: return ((te_clo6)n->function)(n->parameters[6], te_eval(n->parameters[0]), te_eval(n->parameters[1]), te_eval(n->parameters[2]), te_eval(n->parameters[3]), te_eval(n->parameters[4]), te_eval(n->parameters[5]));
                case 7: return ((te_clo7)n->function)(n->parameters[7], te_eval(n->parameters[0]), te_eval(n->parameters[1]), te_eval(n->parameters[2]), te_eval(n->parameters[3]), te_eval(n->parameters[4]), te_eval(n->parameters[5]), te_eval(n->parameters[6]));
                default: return nan_value();
            }

        default: return nan_value();
    }
}

static void optimize(te_expr *n) {
    if (n->type == TE_CONSTANT) return;
    if (n->type == TE_VARIABLE) return;

    if (is_pure(n->type)) {
        const int ar = arity(n->type);
        int known = 1;
        int i;
        for (i = 0; i < ar; ++i) {
            optimize(n->parameters[i]);
            if (((te_expr *)(n->parameters[i]))->type != TE_CONSTANT)
                known = 0;
        }
        if (known) {
            const double value = te_eval(n);
            te_free_parameters(n);
            n->type = TE_CONSTANT;
            n->value = value;
        }
    }
}

te_expr *te_compile(const char *expression, const te_variable *variables, int var_count, int *error) {
    state s;
    te_expr *root;
    s.start = s.next = expression;
    s.lookup = variables;
    s.lookup_len = var_count;
    s.depth = 0;

    next_token(&s);
    root = list(&s);
    if (root == 0) {
        if (error) *error = -1;
        return 0;
    }

    if (s.type != TOK_END) {
        te_free(root);
        if (error) {
            *error = (int)(s.next - s.start);
            if (*error == 0) *error = 1;
        }
        return 0;
    }
    optimize(root);
    if (error) *error = 0;
    return root;
}

double te_interp(const char *expression, int *error) {
    te_expr *n = te_compile(expression, 0, 0, error);
    double ret;
    if (n) {
        ret = te_eval(n);
        te_free(n);
    } else {
        ret = nan_value();
    }
    return ret;
}

static void pn(const te_expr *n, int depth) {
    int i, ar;
    runtime.printf("%*s", depth, "");

    switch (type_mask(n->type)) {
        case TE_CONSTANT: runtime.printf("%f\n", n->value); break;
        case TE_VARIABLE: runtime.printf("bound %p\n", n->bound); break;
        case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:
        case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
        case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:
        case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
            ar = arity(n->type);
            runtime.printf("f%d", ar);
            for (i = 0; i < ar; i++)
                runtime.printf(" %p", n->parameters[i]);
            runtime.printf("\n");
            for (i = 0; i < ar; i++)
                pn(n->parameters[i], depth + 1);
            break;
    }
}

void te_print(const te_expr *n) {
    pn(n, 0);
}
