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
    if (v == 0x4008) {
        if (va_arg(*pap, long long) != 14LL) abort();
        if (va_arg(*pap, long double) != 131.0L) abort();
        if (va_arg(*pap, int) != 17) abort();
    }
}

void f7(int v, ...) {
    va_list ap;
    va_start(ap, v);
    pap = &ap;
    bar(v);
    va_end(ap);
}

int main(void) {
    f7(0x4008, 14LL, 131.0L, 17);
    return 0;
}
