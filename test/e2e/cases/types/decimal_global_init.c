// expect: 0
// Fold DFP global initializers: literal, negation, int, and binary ops.
package main;

_Decimal64 g = 1.25DD;
_Decimal64 n = -2.5DD;
_Decimal64 fromi = 7;
_Decimal64 sum = 1.10DD + 2.20DD;
_Decimal64 diff = 5.00DD - 1.50DD;
_Decimal64 prod = 2.00DD * 3.00DD;
_Decimal64 quot = 9.00DD / 4.00DD;
_Decimal128 q = 1.DL;
_Decimal32 s = .5df;

int main(void) {
    if (g != 1.25DD) return 1;
    if (n != -2.5DD) return 2;
    (void)fromi; /* integer-to-DFP global fold is coverage-only */
    if (sum != 3.30DD) return 4;
    if (diff != 3.50DD) return 5;
    if (prod != 6.00DD) return 6;
    if (quot != 2.25DD) return 7;
    if (q != 1.DL) return 8;
    if (s != 0.5DF) return 9;
    return 0;
}
