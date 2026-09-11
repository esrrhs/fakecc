/* TinyExpr FakeCC port tests, adapted from upstream smoke.c. */
package main;

import tinyexpr;
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
extern double strtod(const char *nptr, char **endptr);

typedef struct {
    const char *expr;
    double answer;
} test_case;

typedef struct {
    const char *expr1;
    const char *expr2;
} test_equ;

static int lfails = 0;
static int ltests = 0;

static void lok(int cond, const char *msg) {
    ++ltests;
    if (!cond) {
        ++lfails;
        runtime.printf("FAIL: %s\n", msg);
    }
}

static void lequal(int a, int b, const char *msg) {
    ++ltests;
    if (a != b) {
        ++lfails;
        runtime.printf("FAIL: %s (%d != %d)\n", msg, a, b);
    }
}

static void lfequal(double a, double b, const char *msg) {
    double d;
    ++ltests;
    if (a == b) return;
    d = fabs(a - b);
    if (d > 0.001 || d != d) {
        ++lfails;
        runtime.printf("FAIL: %s (%f != %f)\n", msg, a, b);
    }
}

static void test_results(void) {
    test_case cases[] = {
        {"1", 1},
        {"1 ", 1},
        {"(1)", 1},
        {"pi", 3.14159},
        {"atan(1)*4 - pi", 0},
        {"e", 2.71828},
        {"2+1", 2 + 1},
        {"(((2+(1))))", 2 + 1},
        {"3+2", 3 + 2},
        {"3+2+4", 3 + 2 + 4},
        {"(3+2)+4", 3 + 2 + 4},
        {"3+(2+4)", 3 + 2 + 4},
        {"(3+2+4)", 3 + 2 + 4},
        {"3*2*4", 3 * 2 * 4},
        {"(3*2)*4", 3 * 2 * 4},
        {"3*(2*4)", 3 * 2 * 4},
        {"(3*2*4)", 3 * 2 * 4},
        {"3-2-4", 3 - 2 - 4},
        {"(3-2)-4", (3 - 2) - 4},
        {"3-(2-4)", 3 - (2 - 4)},
        {"(3-2-4)", 3 - 2 - 4},
        {"3/2/4", 3.0 / 2.0 / 4.0},
        {"(3/2)/4", (3.0 / 2.0) / 4.0},
        {"3/(2/4)", 3.0 / (2.0 / 4.0)},
        {"(3/2/4)", 3.0 / 2.0 / 4.0},
        {"(3*2/4)", 3.0 * 2.0 / 4.0},
        {"(3/2*4)", 3.0 / 2.0 * 4.0},
        {"3*(2/4)", 3.0 * (2.0 / 4.0)},
        {"asin sin .5", 0.5},
        {"sin asin .5", 0.5},
        {"ln exp .5", 0.5},
        {"exp ln .5", 0.5},
        {"asin sin-.5", -0.5},
        {"asin sin-0.5", -0.5},
        {"asin sin -0.5", -0.5},
        {"asin (sin -0.5)", -0.5},
        {"asin (sin (-0.5))", -0.5},
        {"asin sin (-0.5)", -0.5},
        {"(asin sin (-0.5))", -0.5},
        {"log10 1000", 3},
        {"log10 1e3", 3},
        {"log10(1000)", 3},
        {"log10(1e3)", 3},
        {"log10 1.0e3", 3},
        {"10^5*5e-5", 5},
        {"log 1000", 3},
        {"ln (e^10)", 10},
        {"100^.5+1", 11},
        {"100 ^.5+1", 11},
        {"100^+.5+1", 11},
        {"100^--.5+1", 11},
        {"100^---+-++---++-+-+-.5+1", 11},
        {"100^-.5+1", 1.1},
        {"100^---.5+1", 1.1},
        {"100^+---.5+1", 1.1},
        {"1e2^+---.5e0+1e0", 1.1},
        {"--(1e2^(+(-(-(-.5e0))))+1e0)", 1.1},
        {"sqrt 100 + 7", 17},
        {"sqrt 100 * 7", 70},
        {"sqrt (100 * 100)", 100},
        {"1,2", 2},
        {"1,2+1", 3},
        {"1+1,2+2,2+1", 3},
        {"1,2,3", 3},
        {"(1,2),3", 3},
        {"1,(2,3)", 3},
        {"-(1,(2,3))", -3},
        {"2^2", 4},
        {"pow(2,2)", 4},
        {"atan2(1,1)", 0.7854},
        {"atan2(1,2)", 0.4636},
        {"atan2(2,1)", 1.1071},
        {"atan2(3,4)", 0.6435},
        {"atan2(3+3,4*2)", 0.6435},
        {"atan2(3+3,(4*2))", 0.6435},
        {"atan2((3+3),4*2)", 0.6435},
        {"atan2((3+3),(4*2))", 0.6435},
        {"0x10", 16},
        {"0xFF", 255},
        {".5", 0.5}
    };
    int i;
    for (i = 0; i < (int)(sizeof(cases) / sizeof(test_case)); ++i) {
        int err;
        const double ev = tinyexpr.te_interp(cases[i].expr, &err);
        lok(!err, cases[i].expr);
        lfequal(ev, cases[i].answer, cases[i].expr);
    }
}

static void test_syntax(void) {
    test_case errors[] = {
        {"", 1},
        {"1+", 2},
        {"1)", 2},
        {"(1", 2},
        {"1**1", 3},
        {"1*2(+4", 4},
        {"1*2(1+4", 4},
        {"a+5", 1},
        {"_a+5", 1},
        {"#a+5", 1},
        {"1^^5", 3},
        {"1**5", 3},
        {"sin(cos5", 8}
    };
    int i;
    for (i = 0; i < (int)(sizeof(errors) / sizeof(test_case)); ++i) {
        int err;
        const double r = tinyexpr.te_interp(errors[i].expr, &err);
        tinyexpr.te_expr *n;
        lequal(err, (int)errors[i].answer, errors[i].expr);
        lok(r != r, errors[i].expr);
        n = tinyexpr.te_compile(errors[i].expr, 0, 0, &err);
        lequal(err, (int)errors[i].answer, errors[i].expr);
        lok(!n, errors[i].expr);
        {
            const double k = tinyexpr.te_interp(errors[i].expr, 0);
            lok(k != k, errors[i].expr);
        }
    }
}

static void test_nans(void) {
    const char *nans[] = {
        "0/0",
        "1%0",
        "1%(1%0)",
        "(1%0)%1",
        "fac(-1)",
        "ncr(2, 4)",
        "ncr(-2, 4)",
        "ncr(2, -4)",
        "npr(2, 4)",
        "npr(-2, 4)",
        "npr(2, -4)"
    };
    int i;
    for (i = 0; i < (int)(sizeof(nans) / sizeof(nans[0])); ++i) {
        int err;
        const double r = tinyexpr.te_interp(nans[i], &err);
        tinyexpr.te_expr *n;
        lequal(err, 0, nans[i]);
        lok(r != r, nans[i]);
        n = tinyexpr.te_compile(nans[i], 0, 0, &err);
        lok(!!n, nans[i]);
        lequal(err, 0, nans[i]);
        if (n) {
            const double c = tinyexpr.te_eval(n);
            lok(c != c, nans[i]);
            tinyexpr.te_free(n);
        }
    }
}

static void test_infs(void) {
    const char *infs[] = {
        "1/0",
        "log(0)",
        "pow(2,10000000)",
        "fac(300)",
        "ncr(300,100)",
        "ncr(300000,100)",
        "ncr(300000,100)*8",
        "npr(3,2)*ncr(300000,100)",
        "npr(100,90)",
        "npr(30,25)"
    };
    int i;
    for (i = 0; i < (int)(sizeof(infs) / sizeof(infs[0])); ++i) {
        int err;
        const double r = tinyexpr.te_interp(infs[i], &err);
        tinyexpr.te_expr *n;
        lequal(err, 0, infs[i]);
        lok(r == r + 1, infs[i]);
        n = tinyexpr.te_compile(infs[i], 0, 0, &err);
        lok(!!n, infs[i]);
        lequal(err, 0, infs[i]);
        if (n) {
            const double c = tinyexpr.te_eval(n);
            lok(c == c + 1, infs[i]);
            tinyexpr.te_free(n);
        }
    }
}

static void test_variables(void) {
    double x, y, test;
    tinyexpr.te_variable lookup[] = {
        {"x", &x, 0, 0},
        {"y", &y, 0, 0},
        {"te_st", &test, 0, 0}
    };
    int err;
    tinyexpr.te_expr *expr1 = tinyexpr.te_compile("cos x + sin y", lookup, 2, &err);
    tinyexpr.te_expr *expr2 = tinyexpr.te_compile("x+x+x-y", lookup, 2, &err);
    tinyexpr.te_expr *expr3 = tinyexpr.te_compile("x*y^3", lookup, 2, &err);
    tinyexpr.te_expr *expr4 = tinyexpr.te_compile("te_st+5", lookup, 3, &err);
    lok(!!expr1, "compile cos x + sin y");
    lok(!!expr2, "compile x+x+x-y");
    lok(!!expr3, "compile x*y^3");
    lok(!!expr4, "compile te_st+5");

    for (y = 2; y < 3; ++y) {
        for (x = 0; x < 5; ++x) {
            double ev;
            if (expr1) {
                ev = tinyexpr.te_eval(expr1);
                lfequal(ev, cos(x) + sin(y), "cos x + sin y");
            }
            if (expr2) {
                ev = tinyexpr.te_eval(expr2);
                lfequal(ev, x + x + x - y, "x+x+x-y");
            }
            if (expr3) {
                ev = tinyexpr.te_eval(expr3);
                lfequal(ev, x * y * y * y, "x*y^3");
            }
            test = x;
            if (expr4) {
                ev = tinyexpr.te_eval(expr4);
                lfequal(ev, x + 5, "te_st+5");
            }
        }
    }

    tinyexpr.te_free(expr1);
    tinyexpr.te_free(expr2);
    tinyexpr.te_free(expr3);
    tinyexpr.te_free(expr4);

    {
        tinyexpr.te_expr *expr5 = tinyexpr.te_compile("xx*y^3", lookup, 2, &err);
        lok(!expr5, "xx*y^3 should fail");
        lok(err != 0, "xx*y^3 error");
        tinyexpr.te_expr *expr6 = tinyexpr.te_compile("tes", lookup, 3, &err);
        lok(!expr6, "tes should fail");
        tinyexpr.te_expr *expr7 = tinyexpr.te_compile("sinn x", lookup, 2, &err);
        lok(!expr7, "sinn x should fail");
        tinyexpr.te_expr *expr8 = tinyexpr.te_compile("si x", lookup, 2, &err);
        lok(!expr8, "si x should fail");
    }
}

static int cross_check(const char *a, double b, tinyexpr.te_variable *lookup) {
    tinyexpr.te_expr *expr;
    int err;
    double got;
    if (b != b) return 0;
    expr = tinyexpr.te_compile(a, lookup, 2, &err);
    if (!expr) {
        lok(0, a);
        return 1;
    }
    got = tinyexpr.te_eval(expr);
    lfequal(got, b, a);
    lok(!err, a);
    tinyexpr.te_free(expr);
    return 0;
}

static void test_functions(void) {
    double x, y;
    tinyexpr.te_variable lookup[] = {
        {"x", &x, 0, 0},
        {"y", &y, 0, 0}
    };
    for (x = -5; x < 5; x += 1.0) {
        cross_check("abs x", fabs(x), lookup);
        cross_check("acos x", acos(x), lookup);
        cross_check("asin x", asin(x), lookup);
        cross_check("atan x", atan(x), lookup);
        cross_check("ceil x", ceil(x), lookup);
        cross_check("cos x", cos(x), lookup);
        cross_check("cosh x", cosh(x), lookup);
        cross_check("exp x", exp(x), lookup);
        cross_check("floor x", floor(x), lookup);
        cross_check("ln x", log(x), lookup);
        cross_check("log10 x", log10(x), lookup);
        cross_check("sin x", sin(x), lookup);
        cross_check("sinh x", sinh(x), lookup);
        cross_check("sqrt x", sqrt(x), lookup);
        cross_check("tan x", tan(x), lookup);
        cross_check("tanh x", tanh(x), lookup);
        for (y = -2; y < 2; y += 1.0) {
            if (fabs(x) < 0.01) break;
            cross_check("atan2(x,y)", atan2(x, y), lookup);
            cross_check("pow(x,y)", pow(x, y), lookup);
        }
    }
}

static double sum0(void) { return 6; }
static double sum1(double a) { return a * 2; }
static double sum2(double a, double b) { return a + b; }
static double sum3(double a, double b, double c) { return a + b + c; }
static double sum4(double a, double b, double c, double d) { return a + b + c + d; }
static double sum5(double a, double b, double c, double d, double e) { return a + b + c + d + e; }
static double sum6(double a, double b, double c, double d, double e, double f) { return a + b + c + d + e + f; }
static double sum7(double a, double b, double c, double d, double e, double f, double g) { return a + b + c + d + e + f + g; }

static void test_dynamic(void) {
    double x, f;
    tinyexpr.te_variable lookup[] = {
        {"x", &x, 0, 0},
        {"f", &f, 0, 0},
        {"sum0", (const void *)sum0, tinyexpr.TE_FUNCTION0, 0},
        {"sum1", (const void *)sum1, tinyexpr.TE_FUNCTION1, 0},
        {"sum2", (const void *)sum2, tinyexpr.TE_FUNCTION2, 0},
        {"sum3", (const void *)sum3, tinyexpr.TE_FUNCTION3, 0},
        {"sum4", (const void *)sum4, tinyexpr.TE_FUNCTION4, 0},
        {"sum5", (const void *)sum5, tinyexpr.TE_FUNCTION5, 0},
        {"sum6", (const void *)sum6, tinyexpr.TE_FUNCTION6, 0},
        {"sum7", (const void *)sum7, tinyexpr.TE_FUNCTION7, 0}
    };
    test_case cases[] = {
        {"x", 2},
        {"f+x", 7},
        {"x+x", 4},
        {"x+f", 7},
        {"f+f", 10},
        {"f+sum0", 11},
        {"sum0+sum0", 12},
        {"sum0()+sum0", 12},
        {"sum0+sum0()", 12},
        {"sum0()+(0)+sum0()", 12},
        {"sum1 sum0", 12},
        {"sum1(sum0)", 12},
        {"sum1 f", 10},
        {"sum1 x", 4},
        {"sum2 (sum0, x)", 8},
        {"sum3 (sum0, x, 2)", 10},
        {"sum2(2,3)", 5},
        {"sum3(2,3,4)", 9},
        {"sum4(2,3,4,5)", 14},
        {"sum5(2,3,4,5,6)", 20},
        {"sum6(2,3,4,5,6,7)", 27},
        {"sum7(2,3,4,5,6,7,8)", 35}
    };
    int i;
    x = 2;
    f = 5;
    for (i = 0; i < (int)(sizeof(cases) / sizeof(test_case)); ++i) {
        int err;
        tinyexpr.te_expr *ex = tinyexpr.te_compile(cases[i].expr, lookup,
            (int)(sizeof(lookup) / sizeof(tinyexpr.te_variable)), &err);
        lok(!!ex, cases[i].expr);
        if (ex) {
            lfequal(tinyexpr.te_eval(ex), cases[i].answer, cases[i].expr);
            tinyexpr.te_free(ex);
        }
    }
}

static double clo0(void *context) {
    if (context) return *((double *)context) + 6;
    return 6;
}
static double clo1(void *context, double a) {
    if (context) return *((double *)context) + a * 2;
    return a * 2;
}
static double clo2(void *context, double a, double b) {
    if (context) return *((double *)context) + a + b;
    return a + b;
}
static double cell(void *context, double a) {
    double *c = context;
    return c[(int)a];
}

static void test_closure(void) {
    double extra;
    double c[] = {5, 6, 7, 8, 9};
    tinyexpr.te_variable lookup[] = {
        {"c0", (const void *)clo0, tinyexpr.TE_CLOSURE0, &extra},
        {"c1", (const void *)clo1, tinyexpr.TE_CLOSURE1, &extra},
        {"c2", (const void *)clo2, tinyexpr.TE_CLOSURE2, &extra},
        {"cell", (const void *)cell, tinyexpr.TE_CLOSURE1, c}
    };
    test_case cases[] = {
        {"c0", 6},
        {"c1 4", 8},
        {"c2 (10, 20)", 30}
    };
    test_case cases2[] = {
        {"cell 0", 5},
        {"cell 1", 6},
        {"cell 0 + cell 1", 11},
        {"cell 1 * cell 3 + cell 4", 57}
    };
    int i;
    for (i = 0; i < (int)(sizeof(cases) / sizeof(test_case)); ++i) {
        int err;
        tinyexpr.te_expr *ex = tinyexpr.te_compile(cases[i].expr, lookup,
            (int)(sizeof(lookup) / sizeof(tinyexpr.te_variable)), &err);
        lok(!!ex, cases[i].expr);
        if (ex) {
            extra = 0;
            lfequal(tinyexpr.te_eval(ex), cases[i].answer + extra, cases[i].expr);
            extra = 10;
            lfequal(tinyexpr.te_eval(ex), cases[i].answer + extra, cases[i].expr);
            tinyexpr.te_free(ex);
        }
    }
    for (i = 0; i < (int)(sizeof(cases2) / sizeof(test_case)); ++i) {
        int err;
        tinyexpr.te_expr *ex = tinyexpr.te_compile(cases2[i].expr, lookup,
            (int)(sizeof(lookup) / sizeof(tinyexpr.te_variable)), &err);
        lok(!!ex, cases2[i].expr);
        if (ex) {
            lfequal(tinyexpr.te_eval(ex), cases2[i].answer, cases2[i].expr);
            tinyexpr.te_free(ex);
        }
    }
}

static void test_optimize(void) {
    test_case cases[] = {
        {"5+5", 10},
        {"pow(2,2)", 4},
        {"sqrt 100", 10},
        {"pi * 2", 6.2832}
    };
    int i;
    for (i = 0; i < (int)(sizeof(cases) / sizeof(test_case)); ++i) {
        int err;
        tinyexpr.te_expr *ex = tinyexpr.te_compile(cases[i].expr, 0, 0, &err);
        lok(!!ex, cases[i].expr);
        if (ex) {
            lfequal(tinyexpr.te_value(ex), cases[i].answer, cases[i].expr);
            lfequal(tinyexpr.te_eval(ex), cases[i].answer, cases[i].expr);
            tinyexpr.te_free(ex);
        }
    }
}

static void test_pow(void) {
    test_equ cases[] = {
        {"2^3^4", "(2^3)^4"},
        {"-2^2", "(-2)^2"},
        {"(-2)^2", "4"},
        {"--2^2", "2^2"},
        {"---2^2", "(-2)^2"},
        {"-2^2", "4"},
        {"2^1.1^1.2^1.3", "((2^1.1)^1.2)^1.3"},
        {"-a^b", "(-a)^b"},
        {"-a^-b", "(-a)^(-b)"},
        {"1^0", "1"},
        {"(1)^0", "1"},
        {"(-1)^0", "1"},
        {"(-5)^0", "1"},
        {"(!0)^0", "1"},
        {"(!!5)^0", "1"},
        {"2^-3^4", "(2^(-3))^4"},
        {"-2^-3^-4", "((-2)^(-3))^(-4)"}
    };
    double a = 2, b = 3;
    tinyexpr.te_variable lookup[] = {
        {"a", &a, 0, 0},
        {"b", &b, 0, 0}
    };
    int i;
    for (i = 0; i < (int)(sizeof(cases) / sizeof(test_equ)); ++i) {
        tinyexpr.te_expr *ex1 = tinyexpr.te_compile(cases[i].expr1, lookup, 2, 0);
        tinyexpr.te_expr *ex2 = tinyexpr.te_compile(cases[i].expr2, lookup, 2, 0);
        lok(!!ex1, cases[i].expr1);
        lok(!!ex2, cases[i].expr2);
        if (ex1 && ex2) {
            double r1 = tinyexpr.te_eval(ex1);
            double r2 = tinyexpr.te_eval(ex2);
            lfequal(r1, r2, cases[i].expr1);
        }
        tinyexpr.te_free(ex1);
        tinyexpr.te_free(ex2);
    }
}

static void test_combinatorics(void) {
    test_case cases[] = {
        {"fac(0)", 1},
        {"fac(0.2)", 1},
        {"fac(1)", 1},
        {"fac(2)", 2},
        {"fac(3)", 6},
        {"fac(4.8)", 24},
        {"fac(10)", 3628800},
        {"ncr(0,0)", 1},
        {"ncr(10,1)", 10},
        {"ncr(10,0)", 1},
        {"ncr(10,10)", 1},
        {"ncr(16,7)", 11440},
        {"ncr(16,9)", 11440},
        {"ncr(100,95)", 75287520},
        {"npr(0,0)", 1},
        {"npr(10,1)", 10},
        {"npr(10,0)", 1},
        {"npr(10,10)", 3628800},
        {"npr(20,5)", 1860480},
        {"npr(100,4)", 94109400}
    };
    int i;
    for (i = 0; i < (int)(sizeof(cases) / sizeof(test_case)); ++i) {
        int err;
        const double ev = tinyexpr.te_interp(cases[i].expr, &err);
        lok(!err, cases[i].expr);
        lfequal(ev, cases[i].answer, cases[i].expr);
    }
}

static void test_logic(void) {
    test_case cases[] = {
        {"1 && 1", 1},
        {"1 && 0", 0},
        {"0 && 1", 0},
        {"0 && 0", 0},
        {"1 || 1", 1},
        {"1 || 0", 1},
        {"0 || 1", 1},
        {"0 || 0", 0},
        {"!0", 1},
        {"!1", 0},
        {"!2", 0},
        {"!-2", 0},
        {"-!2", 0},
        {"!!0", 0},
        {"!!1", 1},
        {"!!2", 1},
        {"!!-2", 1},
        {"!-!2", 1},
        {"-!!2", -1},
        {"--!!2", 1},
        {"1 < 2", 1},
        {"2 < 2", 0},
        {"2 <= 2", 1},
        {"2 > 1", 1},
        {"2 > 2", 0},
        {"2 >= 2", 1},
        {"2 > -2", 1},
        {"-2 < 2", 1},
        {"0 == 0", 1},
        {"0 != 0", 0},
        {"2 == 2", 1},
        {"2 != 2", 0},
        {"2 == 3", 0},
        {"2 != 3", 1},
        {"2 == 2.0001", 0},
        {"2 != 2.0001", 1},
        {"1 < 2 && 2 < 3", 1},
        {"1 < 2 && 3 < 2", 0},
        {"2 < 1 && 2 < 3", 0},
        {"2 < 1 && 3 < 2", 0},
        {"1 < 2 || 2 < 3", 1},
        {"1 < 2 || 3 < 2", 1},
        {"2 < 1 || 2 < 3", 1},
        {"2 < 1 || 3 < 2", 0},
        {"0 == 1 < 2", 0},
        {"1 == 1 < 2", 1},
        {"1 != 2 > 3", 1},
        {"1 || 0 && 0", 1},
        {"1 || 1 && 0", 1},
        {"0 && 0 || 1", 1},
        {"!1 == 0", 1},
        {"1 < 1+1", 1},
        {"1 < 1*2", 1},
        {"1 < 2/2", 0},
        {"1 < 2^2", 1},
        {"5+5 < 4+10", 1},
        {"5+(5 < 4)+10", 15},
        {"5+(5 < 4+10)", 6},
        {"(5+5 < 4)+10", 10},
        {"5+!(5 < 4)+10", 16},
        {"5+!(5 < 4+10)", 5},
        {"!(5+5 < 4)+10", 11},
        {"!0^2", 1},
        {"!0^-1", 1},
        {"-!0^2", 1}
    };
    int i;
    for (i = 0; i < (int)(sizeof(cases) / sizeof(test_case)); ++i) {
        int err;
        const double ev = tinyexpr.te_interp(cases[i].expr, &err);
        lok(!err, cases[i].expr);
        lfequal(ev, cases[i].answer, cases[i].expr);
    }
}

static void test_depth(void) {
    int depths[] = {50, 100, 400, 1000};
    int i;
    for (i = 0; i < (int)(sizeof(depths) / sizeof(int)); ++i) {
        const int depth = depths[i];
        const int ok = depth < 500;
        int j, err;
        char *expr = runtime.malloc(depth * 2 + 2);
        double r;
        if (!expr) {
            lok(0, "malloc nested parens");
            continue;
        }
        runtime.memset(expr, '(', depth);
        expr[depth] = '1';
        runtime.memset(expr + depth + 1, ')', depth);
        expr[depth * 2 + 1] = '\0';
        r = tinyexpr.te_interp(expr, &err);
        if (ok)
            lok(err == 0 && r == 1.0, "nested parens");
        else
            lok(err != 0 && r != r, "nested parens overflow");
        runtime.free(expr);

        expr = runtime.malloc(depth * 4 + 2);
        if (!expr) {
            lok(0, "malloc nested sin");
            continue;
        }
        for (j = 0; j < depth; ++j)
            runtime.memcpy(expr + j * 4, "sin ", 4);
        expr[depth * 4] = '1';
        expr[depth * 4 + 1] = '\0';
        r = tinyexpr.te_interp(expr, &err);
        if (ok)
            lok(err == 0, "nested sin");
        else
            lok(err != 0 && r != r, "nested sin overflow");
        runtime.free(expr);
    }
}

static unsigned long long number_rand_state = 1;

static unsigned long number_rand(unsigned long modulus) {
    number_rand_state = number_rand_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (unsigned long)((number_rand_state >> 33) % modulus);
}

static void test_number(void) {
    int i;
    const char *digits = "0123456789";
    const char *hex = "0123456789abcdefABCDEF";
    for (i = 0; i < 200; ++i) {
        char expr[64];
        char *p = expr;
        int j;
        if (number_rand(8) == 0) {
            const int ndigits = 1 + (int)number_rand(8);
            p += runtime.sprintf(p, "0x");
            for (j = 0; j < ndigits; ++j)
                p += runtime.sprintf(p, "%c", hex[number_rand(22)]);
        } else {
            const int int_digits = (int)number_rand(11);
            const int frac_digits = int_digits ? (int)number_rand(11) : 1 + (int)number_rand(10);
            for (j = 0; j < int_digits; ++j)
                p += runtime.sprintf(p, "%c", digits[number_rand(10)]);
            if (frac_digits || number_rand(2)) {
                p += runtime.sprintf(p, ".");
                for (j = 0; j < frac_digits; ++j)
                    p += runtime.sprintf(p, "%c", digits[number_rand(10)]);
            }
            if (number_rand(2)) {
                p += runtime.sprintf(p, "%c", "eE"[number_rand(2)]);
                if (number_rand(2)) p += runtime.sprintf(p, "%c", "+-"[number_rand(2)]);
                p += runtime.sprintf(p, "%lu", number_rand(320));
            }
        }
        {
            const double expected = strtod(expr, 0);
            int err;
            const double got = tinyexpr.te_interp(expr, &err);
            const int ok = err == 0 && (got == expected ||
                fabs(got - expected) <= 1e-14 * fabs(expected) + 1e-304);
            lok(ok, expr);
        }
    }
    {
        const char *partial[] = {"1e", "1e+", "0x", "."};
        int j;
        for (j = 0; j < (int)(sizeof(partial) / sizeof(partial[0])); ++j) {
            int err;
            const double r = tinyexpr.te_interp(partial[j], &err);
            lok(r != r, partial[j]);
            lok(err != 0, partial[j]);
        }
    }
}

int main(void) {
    test_results();
    test_syntax();
    test_nans();
    test_infs();
    test_variables();
    test_functions();
    test_dynamic();
    test_closure();
    test_optimize();
    test_pow();
    test_combinatorics();
    test_logic();
    test_depth();
    test_number();

    if (lfails == 0)
        runtime.printf("ALL TESTS PASSED (%d/%d)\n", ltests, ltests);
    else
        runtime.printf("SOME TESTS FAILED (%d/%d)\n", ltests - lfails, ltests);
    return lfails != 0;
}
