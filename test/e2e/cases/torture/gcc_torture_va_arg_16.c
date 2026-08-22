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

void f(long double a, ...) {
    va_list args;
    va_start(args, a);
    long double d;
    d = va_arg(args, long double);
    if (d != 1.0L) abort();
    d = va_arg(args, long double);
    if (d != 2.0L) abort();
    va_end(args);
}

int main(void) {
    long double a = 3.14L;
    f(a, 1.0L, 2.0L);
    exit(0);
    return 0;
}
