package main;

import runtime;

/* Test Win64 calling convention & ABI features */

/* 1. Long double width */
int test_long_double(void) {
    if (sizeof(long double) != 8) return 1;
    long double a = 1.25;
    long double b = 2.5;
    long double c = a + b;
    if ((double)c != 3.75) return 2;
    return 0;
}

/* 2. Positional integer and float arguments (RCX, RDX, R8, R9 / XMM0-3) */
/* Slot 0: int (RCX)
 * Slot 1: double (XMM1)
 * Slot 2: int (R8)
 * Slot 3: float (XMM3)
 */
double mix4(int a, double b, int c, float d) {
    return (double)a + b + (double)c + (double)d;
}

int test_mix4(void) {
    double res = mix4(10, 20.5, 30, 40.5f);
    if (res != 101.0) return 1;
    return 0;
}

/* 3. Stack arguments (> 4 args) */
double many_args(int a, double b, int c, double d, int e, double f, int g, double h) {
    return (double)a + b + (double)c + d + (double)e + f + (double)g + h;
}

int test_many_args(void) {
    double res = many_args(1, 2.0, 3, 4.0, 5, 6.0, 7, 8.0);
    if (res != 36.0) return 1;
    return 0;
}

/* 4. Aggregates: 1, 2, 4, 8 bytes passed/returned by value */
struct S1 { char a; };
struct S2 { short a; };
struct S4 { int a; };
struct S8 { int a; int b; };

struct S1 ret_s1(char x) { struct S1 s; s.a = x; return s; }
struct S2 ret_s2(short x) { struct S2 s; s.a = x; return s; }
struct S4 ret_s4(int x) { struct S4 s; s.a = x; return s; }
struct S8 ret_s8(int a, int b) { struct S8 s; s.a = a; s.b = b; return s; }

int pass_small(struct S1 s1, struct S2 s2, struct S4 s4, struct S8 s8) {
    return s1.a + s2.a + s4.a + s8.a + s8.b;
}

int test_small_structs(void) {
    if (sizeof(struct S1) != 1 || sizeof(struct S2) != 2 || sizeof(struct S4) != 4 || sizeof(struct S8) != 8)
        return 1;
    struct S1 s1 = ret_s1(10);
    struct S2 s2 = ret_s2(20);
    struct S4 s4 = ret_s4(30);
    struct S8 s8 = ret_s8(40, 50);
    if (s1.a != 10 || s2.a != 20 || s4.a != 30 || s8.a != 40 || s8.b != 50)
        return 2;
    if (pass_small(s1, s2, s4, s8) != 150)
        return 3;
    return 0;
}

/* 5. Aggregates: sizes != 1, 2, 4, 8 passed by reference & returned via sret */
struct S12 { int a; int b; int c; };
struct S16 { long long a; long long b; };
struct S24 { long long a; long long b; long long c; };

struct S12 ret_s12(int a, int b, int c) {
    struct S12 s; s.a = a; s.b = b; s.c = c;
    return s;
}

struct S16 ret_s16(long long a, long long b) {
    struct S16 s; s.a = a; s.b = b;
    return s;
}

struct S24 ret_s24(long long a, long long b, long long c) {
    struct S24 s; s.a = a; s.b = b; s.c = c;
    return s;
}

long long pass_large(struct S12 s12, struct S16 s16, struct S24 s24) {
    return s12.a + s12.b + s12.c + s16.a + s16.b + s24.a + s24.b + s24.c;
}

int test_large_structs(void) {
    if (sizeof(struct S12) != 12 || sizeof(struct S16) != 16 || sizeof(struct S24) != 24)
        return 1;
    struct S12 s12 = ret_s12(1, 2, 3);
    struct S16 s16 = ret_s16(10, 20);
    struct S24 s24 = ret_s24(100, 200, 300);
    if (s12.a != 1 || s12.b != 2 || s12.c != 3) return 2;
    if (s16.a != 10 || s16.b != 20) return 3;
    if (s24.a != 100 || s24.b != 200 || s24.c != 300) return 4;
    long long sum = pass_large(s12, s16, s24);
    if (sum != 636) return 5;
    return 0;
}

/* 6. Callee-saved RSI and RDI */
int clobber_callee(int n) {
    int sum = 0;
    int i = 0;
    while (i < n) {
        sum += i;
        i++;
    }
    return sum;
}

int test_callee_saved(void) {
    long long v1 = 0x123456789ABCDEF0LL;
    long long v2 = 0x0FEDCBA987654321LL;
    int dummy = clobber_callee(100);
    if (dummy != 4950) return 1;
    if (v1 != 0x123456789ABCDEF0LL) return 2;
    if (v2 != 0x0FEDCBA987654321LL) return 3;
    return 0;
}

/* 7. Variadic functions, va_list, va_start, va_arg, va_copy */
double sum_var_doubles(int count, ...) {
    va_list ap;
    __builtin_va_start(ap, count);
    double sum = 0.0;
    int i = 0;
    while (i < count) {
        sum += __builtin_va_arg(ap, double);
        i++;
    }
    __builtin_va_end(ap);
    return sum;
}

int sum_var_mix(int count, ...) {
    va_list ap;
    __builtin_va_start(ap, count);
    va_list ap2;
    __builtin_va_copy(ap2, ap);

    int sum1 = 0;
    int i = 0;
    while (i < count) {
        sum1 += __builtin_va_arg(ap, int);
        i++;
    }
    __builtin_va_end(ap);

    int sum2 = 0;
    i = 0;
    while (i < count) {
        sum2 += __builtin_va_arg(ap2, int);
        i++;
    }
    __builtin_va_end(ap2);

    if (sum1 != sum2) return -1;
    return sum1;
}

int test_variadic(void) {
    if (sizeof(va_list) != 8) return 1;
    double dsum = sum_var_doubles(5, 1.5, 2.5, 3.5, 4.5, 5.5);
    if (dsum != 17.5) return 2;

    int isum = sum_var_mix(6, 10, 20, 30, 40, 50, 60);
    if (isum != 210) return 3;

    return 0;
}

/* 8. Variadic with structs and mixed types */
int sum_var_struct(int count, ...) {
    va_list ap;
    __builtin_va_start(ap, count);
    int sum = 0;
    struct S4 s4 = __builtin_va_arg(ap, struct S4);
    sum += s4.a;
    struct S12 s12 = __builtin_va_arg(ap, struct S12);
    sum += s12.a + s12.b + s12.c;
    int extra = __builtin_va_arg(ap, int);
    sum += extra;
    __builtin_va_end(ap);
    return sum;
}

int test_variadic_structs(void) {
    struct S4 s4; s4.a = 100;
    struct S12 s12; s12.a = 200; s12.b = 300; s12.c = 400;
    int res = sum_var_struct(3, s4, s12, 500);
    if (res != 1500) return 1;
    return 0;
}

/* 9. Native Win64 Static TLS */
__thread int tls_var1 = 12345;
__thread long tls_var2 = 0x1122334455667788LL;
__thread char tls_buf[16];
__thread const char *tls_ptr = "win64_tls";

void tls_helper(int delta) {
    tls_var1 += delta;
}

int test_win64_tls(void) {
    if (tls_var1 != 12345) return 1;
    if (tls_var2 != 0x1122334455667788LL) return 2;
    if (tls_ptr[0] != 'w' || tls_ptr[8] != 's') return 3;

    tls_buf[0] = 7;
    tls_buf[15] = 42;
    if (tls_buf[0] != 7 || tls_buf[15] != 42) return 4;

    tls_helper(55);
    if (tls_var1 != 12400) return 5;

    return 0;
}

int main(int argc, char **argv) {
    /* Verify argc >= 1 and argv[0] is non-NULL */
    if (argc < 1 || argv == 0 || argv[0] == 0) {
        return 99;
    }

    int rc;
    rc = test_long_double();
    if (rc != 0) return 10 + rc;
    rc = test_mix4();
    if (rc != 0) return 20 + rc;
    rc = test_many_args();
    if (rc != 0) return 30 + rc;
    rc = test_small_structs();
    if (rc != 0) return 40 + rc;
    rc = test_large_structs();
    if (rc != 0) return 50 + rc;
    rc = test_callee_saved();
    if (rc != 0) return 60 + rc;
    rc = test_variadic();
    if (rc != 0) return 70 + rc;
    rc = test_variadic_structs();
    if (rc != 0) return 80 + rc;
    rc = test_win64_tls();
    if (rc != 0) return 90 + rc;

    runtime.printf("ALL WIN64 ABI TESTS PASSED\n");
    return 0;
}
