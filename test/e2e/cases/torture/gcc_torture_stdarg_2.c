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

void foo(int v, va_list ap) {
    switch (v) {
    case 5:
        foo_arg = va_arg(ap, int);
        foo_arg += (int)va_arg(ap, double);
        foo_arg += (int)va_arg(ap, long long);
        break;
    default:
        abort();
    }
}

void f5(int v, ...) {
    va_list ap;
    va_start(ap, v);
    foo(v, ap);
    va_end(ap);
}

int main(void) {
    f5(5, 1, 19.0, 18LL);
    if (foo_arg != 38) abort();
    return 0;
}
