// expect: 0
/* Integer literal typing on LP64: hex/octal wrap at 64 bits; decimal uses
 * the full 128-bit range (int → long → long long → __int128 → unsigned
 * __int128).  A true 2^64 value is most naturally written as a decimal
 * literal (no shift needed). */
package main;

int main(void) {
    int fail = 0;

    if (sizeof(0x1p0) != sizeof(double)) fail = fail + 1;

    /* 2^64 hex wraps to 0, typed as int. */
    if (sizeof(0x10000000000000000) != 4) fail = fail + 1;
    if ((int)0x10000000000000000 != 0) fail = fail + 1;
    unsigned __int128 h = 0x10000000000000000;
    if ((int)h != 0) fail = fail + 1;
    if ((int)(h >> 64) != 0) fail = fail + 1;

    /* 2^64 decimal is a full __int128 (lo=0, hi=1). */
    if (sizeof(18446744073709551616) != 16) fail = fail + 1;
    __int128 d = 18446744073709551616;
    if ((int)d != 0) fail = fail + 1;
    if ((int)(d >> 64) != 1) fail = fail + 1;

    /* 2^64 + 0xFF wraps to 0xFF (hex). */
    unsigned __int128 x = 0x100000000000000FF;
    if ((int)x != 0xFF) fail = fail + 1;
    if ((int)(x >> 64) != 0) fail = fail + 1;

    if (sizeof(0x8000000000000000) != 8) fail = fail + 1;
    if (sizeof(0xFFFFFFFFFFFFFFFFULL) != 8) fail = fail + 1;

    /* Decimal 2^63 does not fit long long → __int128. */
    if (sizeof(9223372036854775808) != 16) fail = fail + 1;

    /* Decimal 2^64 + 0xFF is a full __int128 (lo=0xFF, hi=1). */
    unsigned __int128 dx = 18446744073709551616 + 0xFF;
    if ((int)dx != 0xFF) fail = fail + 1;
    if ((int)(dx >> 64) != 1) fail = fail + 1;

    /* Static initializer with decimal 2^64 + 1. */
    static __int128 g = 18446744073709551617;
    if ((int)g != 1) fail = fail + 1;
    if ((int)(g >> 64) != 1) fail = fail + 1;

    return fail;
}
