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

void f(int a, int b, ...) {
    va_list args;
    va_start(args, b);
    long double d;
    d = va_arg(args, long double);
    if (d != 1.0L) abort();
    d = va_arg(args, long double);
    if (d != 2.0L) abort();
    d = va_arg(args, long double);
    if (d != 3.0L) abort();
    va_end(args);
}

int main(void) {
    f(1, 2, 1.0L, 2.0L, 3.0L);
    exit(0);
    return 0;
}
