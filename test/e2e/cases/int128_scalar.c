// expect: 0
package main;

__int128 add128(__int128 a, __int128 b) { return a + b; }

int main(void) {
    int fail = 0;
    __int128 a = 1;
    a = a << 64;
    a = a + 5;
    if ((int)a != 5) fail = fail + 1;
    if ((int)(a >> 64) != 1) fail = fail + 1;

    __int128 b = a + a;
    if ((int)b != 10) fail = fail + 1;
    if ((int)(b >> 64) != 2) fail = fail + 1;

    __int128 c = -a;
    if ((int)c != -5) fail = fail + 1;
    if ((int)(c >> 64) != -2) fail = fail + 1;

    unsigned __int128 u = (unsigned __int128)1;
    u = u << 64;
    u = u * (unsigned __int128)2;
    if ((int)(u >> 64) != 2) fail = fail + 1;
    if ((int)u != 0) fail = fail + 1;

    unsigned __int128 p = (unsigned __int128)0x100000000ULL;
    p = p * p;
    if ((int)(p >> 64) != 1) fail = fail + 1;
    if ((int)p != 0) fail = fail + 1;

    __int128 d = (__int128)100;
    if ((int)(d / 7) != 14) fail = fail + 1;
    if ((int)(d % 7) != 2) fail = fail + 1;
    if ((int)((-d) / 7) != -14) fail = fail + 1;
    if ((int)((-d) % 7) != -2) fail = fail + 1;

    if (!(a != b)) fail = fail + 1;
    if (!(a < b)) fail = fail + 1;
    if (b < a) fail = fail + 1;

    unsigned __int128 ones = ~(unsigned __int128)0;
    if ((int)(ones >> 64) != -1) fail = fail + 1;
    if ((int)ones != -1) fail = fail + 1;

    __int128 x = 10;
    x += 3;
    if ((int)x != 13) fail = fail + 1;
    x = x + 1;
    if ((int)x != 14) fail = fail + 1;

    __int128 y = add128(a, a);
    if ((int)y != 10) fail = fail + 1;
    if ((int)(y >> 64) != 2) fail = fail + 1;

    return fail;
}
