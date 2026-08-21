// expect: 0
/* Wide constants via __int128 shifts/casts (GCC gnu99 does not type
 * unsuffixed digit strings wider than 64 bits as __int128; it warns and
 * keeps a narrow type). */
package main;

int main(void) {
    int fail = 0;

    unsigned __int128 h = (unsigned __int128)1 << 64;
    if ((int)(h >> 64) != 1) fail = fail + 1;
    if ((int)h != 0) fail = fail + 1;

    __int128 d = (__int128)1 << 64;
    if ((int)(d >> 64) != 1) fail = fail + 1;
    if ((int)d != 0) fail = fail + 1;

    unsigned __int128 x = ((unsigned __int128)0xFF) | ((unsigned __int128)1 << 64);
    if ((int)x != 0xFF) fail = fail + 1;
    if ((int)(x >> 64) != 1) fail = fail + 1;

    if (sizeof(0x8000000000000000) != 8) fail = fail + 1;
    if (sizeof(0xFFFFFFFFFFFFFFFFULL) != 8) fail = fail + 1;

    static __int128 g;
    g = ((__int128)1 << 64) | 1;
    if ((int)g != 1) fail = fail + 1;
    if ((int)(g >> 64) != 1) fail = fail + 1;

    return fail;
}
