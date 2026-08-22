// expect: 0
package main;

extern void abort(void);
extern void exit(int);
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
va_list *pap;

void foo(int v, va_list ap) {
    switch (v) {
    case 5: foo_arg = va_arg(ap, int); break;
    default: abort();
    }
}

void bar(int v) {
    if (v == 0x4006) {
        double d1 = va_arg(gap, double);
        long l = va_arg(gap, long);
        if (d1 != 17.0 || l != 129L) abort();
    } else if (v == 0x4008) {
        long long ll = va_arg(*pap, long long);
        long double ld = va_arg(*pap, long double);
        int i = va_arg(*pap, int);
        if (ll != 14LL || ld != 131.0L || i != 17) abort();
    }
    bar_arg = v;
}

void f0(int i, ...) {
    va_list ap;
    va_start(ap, i);
    bar(i);
    x = va_arg(ap, int);
    va_end(ap);
}

void f5(int v, ...) {
    va_list ap;
    va_start(ap, v);
    foo(v, ap);
    va_end(ap);
}

int main(void) {
    f0(1);
    f5(5, 128);
    if (foo_arg != 128 || x != 1) abort();
    return 0;
}
