// expect: 0
// IEEE 754 decimal floating types: sizes, signed zero, and exact decimal arith.
package main;

int main(void) {
    if (sizeof(_Decimal32) != 4) return 1;
    if (sizeof(_Decimal64) != 8) return 2;
    if (sizeof(_Decimal128) != 16) return 3;
    if (sizeof(0.DF) != 4) return 4;
    if (sizeof(0.DD) != 8) return 5;
    if (sizeof(0.DL) != 16) return 6;

    _Decimal64 z = 0.DD;
    _Decimal64 nz = -0.DD;
    if (z != nz) return 7;
    if (nz != 0.DD) return 8;
    if (z != -0.DD) return 9;

    _Decimal32 s = -0.DF;
    if (s != 0.DF) return 10;

    _Decimal64 a = 1.10DD;
    _Decimal64 b = 2.20DD;
    _Decimal64 c = a + b;
    if (c != 3.30DD) return 11;
    if (c == 3.299DD) return 12;
    if (!(c > 3.00DD)) return 13;
    if (!(c >= 3.30DD)) return 14;
    if (!(1.10DD < 2.20DD)) return 15;
    if (1.10DD == 2.20DD) return 16;

    _Decimal64 d = 5.00DD - 1.50DD;
    if (d != 3.50DD) return 17;

    _Decimal64 e = 2.00DD * 3.00DD;
    if (e != 6.00DD) return 18;

    _Decimal64 f = 9.00DD / 4.00DD;
    if (f != 2.25DD) return 19;

    _Decimal32 x = 1.25DF;
    _Decimal64 y = x;
    if (y != 1.25DD) return 20;

    _Decimal64 fromi = 7;
    if (fromi != 7.DD) return 21;

    int k = (int)3.75DD;
    if (k != 3) return 22;

    _Decimal128 q = 1.DL;
    _Decimal128 r = 2.DL;
    if (q + r != 3.DL) return 23;
    if (q == r) return 24;

    if (!c) return 25;
    if (!!z) return 26;

    _Decimal64 g = 1.10DD;
    g += 2.20DD;
    if (g != 3.30DD) return 27;

    return 0;
}
