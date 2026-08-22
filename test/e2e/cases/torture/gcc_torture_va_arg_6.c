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

void f(int n, ...) {
    va_list args;
    va_start(args, n);
    int i; long long ll; long double ld; double d;
    i = va_arg(args, int); if (i != 10) abort();
    ll = va_arg(args, long long); if (ll != 10000000000LL) abort();
    i = va_arg(args, int); if (i != 11) abort();
    ld = va_arg(args, long double); if (ld != 3.14L) abort();
    i = va_arg(args, int); if (i != 12) abort();
    i = va_arg(args, int); if (i != 13) abort();
    ll = va_arg(args, long long); if (ll != 20000000000LL) abort();
    i = va_arg(args, int); if (i != 14) abort();
    d = va_arg(args, double); if (d != 2.72) abort();
    va_end(args);
}

int main(void) {
    f(4, 10, 10000000000LL, 11, 3.14L, 12, 13, 20000000000LL, 14, 2.72);
    exit(0);
    return 0;
}
