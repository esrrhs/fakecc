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

void va_double(int n, ...) {
    va_list args;
    va_start(args, n);
    if (va_arg(args, double) != 3.141592) abort();
    if (va_arg(args, double) != 2.71827) abort();
    if (va_arg(args, double) != 2.2360679) abort();
    if (va_arg(args, double) != 2.1474836) abort();
    va_end(args);
}

void va_long_double(int n, ...) {
    va_list args;
    va_start(args, n);
    if (va_arg(args, long double) != 3.141592L) abort();
    if (va_arg(args, long double) != 2.71827L) abort();
    if (va_arg(args, long double) != 2.2360679L) abort();
    if (va_arg(args, long double) != 2.1474836L) abort();
    va_end(args);
}

int main(void) {
    va_double(4, 3.141592, 2.71827, 2.2360679, 2.1474836);
    va_long_double(4, 3.141592L, 2.71827L, 2.2360679L, 2.1474836L);
    exit(0);
    return 0;
}
