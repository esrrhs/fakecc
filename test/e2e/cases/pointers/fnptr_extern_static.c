// expect: 0
// link: -lm
// Extern libm function pointers in a file-scope table must go through a
// real PLT slot.  They are referenced only from .data, never as a direct
// call or `&fn` in code — that used to patch the slot to the ELF entry
// stub, so calling the pointer re-entered `_start`.
package main;

extern double asin(double x);
extern double acos(double x);

typedef double (*fn_d)(double);

static double ident(double x) { return x; }

static const fn_d table[] = {
    ident,
    asin,
    acos
};

int main(void) {
    double a = table[0](0.5);
    double b = table[1](0.5);
    double c = table[2](0.5);
    if (a < 0.49 || a > 0.51) return 1;
    if (b < 0.52 || b > 0.53) return 2;
    if (c < 1.04 || c > 1.05) return 3;
    return 0;
}
