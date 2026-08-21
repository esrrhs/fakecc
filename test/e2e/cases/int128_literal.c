// expect: 0
/* GCC gnu99 on LP64: integer constants wrap at 64 bits; a magnitude that does
 * not fit in unsigned long long keeps the low 64 bits and is typed from that
 * wrapped value (so 2^64 is `int` 0).  Decimal values in (LLONG_MAX,
 * ULLONG_MAX] are `__int128`.  Hex/octal never become `__int128`.  A true
 * 2^64 value is written with a shift. */
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

    /* 2^64 decimal wraps the same way. */
    if (sizeof(18446744073709551616) != 4) fail = fail + 1;
    __int128 d = 18446744073709551616;
    if ((int)d != 0) fail = fail + 1;
    if ((int)(d >> 64) != 0) fail = fail + 1;

    /* 2^64 + 0xFF wraps to 0xFF. */
    unsigned __int128 x = 0x100000000000000FF;
    if ((int)x != 0xFF) fail = fail + 1;
    if ((int)(x >> 64) != 0) fail = fail + 1;

    if (sizeof(0x8000000000000000) != 8) fail = fail + 1;
    if (sizeof(0xFFFFFFFFFFFFFFFFULL) != 8) fail = fail + 1;

    /* Decimal 2^63 does not fit long long → GCC `__int128`. */
    if (sizeof(9223372036854775808) != 16) fail = fail + 1;

    /* Actual 2^64, the way GCC writes it. */
    unsigned __int128 w = (unsigned __int128)1 << 64;
    if ((int)(w >> 64) != 1) fail = fail + 1;
    if ((int)w != 0) fail = fail + 1;

    static __int128 g = 0x10000000000000001;
    if ((int)g != 1) fail = fail + 1;
    if ((int)(g >> 64) != 0) fail = fail + 1;

    return fail;
}
