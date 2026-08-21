// expect: 0
/* Unsuffixed constants larger than 64 bits are `__int128` / `unsigned __int128`
 * (GCC), not truncated through `unsigned long long`. */
package main;

int main(void) {
    int fail = 0;

    /* 2^64: hex, does not fit unsigned long long → unsigned __int128 */
    if (sizeof(0x1p0) != sizeof(double)) fail = fail + 1;
    if (sizeof(0x10000000000000000) != 16) fail = fail + 1;
    unsigned __int128 h = 0x10000000000000000;
    if ((int)(h >> 64) != 1) fail = fail + 1;
    if ((int)h != 0) fail = fail + 1;

    /* 2^64: decimal fits signed __int128 */
    if (sizeof(18446744073709551616) != 16) fail = fail + 1;
    __int128 d = 18446744073709551616;
    if ((int)(d >> 64) != 1) fail = fail + 1;
    if ((int)d != 0) fail = fail + 1;

    /* 2^64 + 0xFF */
    unsigned __int128 x = 0x100000000000000FF;
    if ((int)x != 0xFF) fail = fail + 1;
    if ((int)(x >> 64) != 1) fail = fail + 1;

    /* Still a 64-bit unsigned long long */
    if (sizeof(0x8000000000000000) != 8) fail = fail + 1;
    if (sizeof(0xFFFFFFFFFFFFFFFFULL) != 8) fail = fail + 1;

    /* Static initializer */
    static __int128 g = 0x10000000000000001;
    if ((int)g != 1) fail = fail + 1;
    if ((int)(g >> 64) != 1) fail = fail + 1;

    return fail;
}
