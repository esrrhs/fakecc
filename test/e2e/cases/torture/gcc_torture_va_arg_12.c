// expect: 0
package main;

extern void abort(void);
extern void exit(int);
extern unsigned long strlen(const char *);
void abort(void);

typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} __va_list_tag;
typedef __va_list_tag __builtin_va_list[1];
typedef __builtin_va_list va_list;

int to_hex(unsigned int a) {
    static char hex[] = "0123456789abcdef";
    if (a > 15) abort();
    return hex[a];
}

void fap(int i, char* format, va_list ap) {
    if (strlen(format) != 16 - i) abort();
    while (*format) {
        char c = *format++;
        char h = (char)to_hex(va_arg(ap, int));
        if (c != h) abort();
    }
}

void f0(char* format, ...) {
    va_list ap;
    va_start(ap, format);
    fap(0, format, ap);
    va_end(ap);
}

int main(void) {
    char *f = "0123456789abcdef";
    f0(f+0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    exit(0);
    return 0;
}
