// expect: 0
// Qualified `pkg.CONST` resolves imported enum constants (anonymous and
// tagged) in expressions, static initializers, array bounds, local enums,
// calls, and case / case-range labels.
package main;
import flags;

int g = flags.FLAG_A;
int tbl[] = { flags.FLAG_A, flags.FLAG_B, flags.FLAG_C };
enum { LOCAL = flags.FLAG_B };

int id(int x) { return x; }

int main(void) {
    if (g != 1) return 1;
    if (flags.FLAG_B != 2) return 2;
    if (flags.COLOR_RED != 10) return 3;
    if (flags.COLOR_BLUE != 20) return 4;

    int a[flags.FLAG_C];
    if ((int)(sizeof(a) / sizeof(a[0])) != 4) return 5;

    int x = flags.COLOR_BLUE;
    switch (x) {
    case flags.COLOR_RED:
        return 6;
    case flags.COLOR_BLUE:
        break;
    default:
        return 7;
    }

    if (flags.FLAG_NEG != -5) return 8;
    if (flags.FLAG_NEXT != -4) return 9;
    if (flags.TOKEN_A != 100) return 10;
    if (flags.TOKEN_B != 101) return 11;
    if ((flags.FLAG_A | flags.FLAG_B) != 3) return 12;
    if (flags.FLAG_A + flags.FLAG_C != 5) return 13;
    if (id(flags.COLOR_RED) != 10) return 14;
    if (LOCAL != 2) return 15;
    if (tbl[0] != 1 || tbl[1] != 2 || tbl[2] != 4) return 16;

    switch (3) {
    case flags.FLAG_A ... flags.FLAG_C:
        break;
    default:
        return 17;
    }

    if (flags.flag_one() != 1) return 18;
    return 0;
}
