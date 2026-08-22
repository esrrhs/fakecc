// expect: 0
package main;

extern void abort(void);
void abort(void);

typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} __va_list_tag;
typedef __va_list_tag __builtin_va_list[1];
typedef __builtin_va_list va_list;

int foo_arg, bar_arg;
long x;
double d;
va_list gap;

void foo(int v, va_list ap) {
    switch (v) {
    case 5:
        foo_arg = va_arg(ap, int);
        foo_arg += (int)va_arg(ap, double);
        foo_arg += (int)va_arg(ap, long long);
        break;
    case 8:
        foo_arg = (int)va_arg(ap, long long);
        foo_arg += (int)va_arg(ap, double);
        break;
    case 11:
        foo_arg = va_arg(ap, int);
        foo_arg += (int)va_arg(ap, long double);
        break;
    default:
        abort();
    }
}

void bar(int v) {
    va_list copy;
    va_copy(copy, gap);
    if (v == 0x4002) {
        if (va_arg(copy, int) != 13 || va_arg(copy, double) != -14.0)
            abort();
    }
    bar_arg = v;
    va_end(copy);
}

void f1(int i, ...) {
    va_start(gap, i);
    x = va_arg(gap, long);
    va_end(gap);
}

void f2(int i, ...) {
    va_start(gap, i);
    bar(i);
    va_end(gap);
}

void f5(int v, ...) {
    va_list ap;
    va_start(ap, v);
    foo(v, ap);
    va_end(ap);
}

int main(void) {
    f1(1, 79L);
    if (x != 79L) abort();
    f2(0x4002, 13, -14.0);
    if (bar_arg != 0x4002) abort();
    f5(5, 1, 19.0, 18LL);
    if (foo_arg != 38) abort();
    return 0;
}
