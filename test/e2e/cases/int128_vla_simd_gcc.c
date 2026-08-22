// expect: 0
package main;

int vla_computed_outer(int n) {
    int a[n];
    void *t;
    a[0] = 42;
    t = &&after;
    {
        int inner[n];
        inner[0] = 1;
        t = &&after;
        goto *t;
    }
after:
    return a[0] == 42 ? 0 : 1;
}

typedef int v4si __attribute__((vector_size(16)));

int vec_div(void) {
    v4si a;
    v4si b;
    v4si c;
    int fail = 0;
    a[0] = 8; a[1] = 15; a[2] = -16; a[3] = 9;
    b[0] = 2; b[1] = 4; b[2] = 2; b[3] = 3;
    c = a / b;
    if (c[0] != 4) fail = fail + 1;
    if (c[1] != 3) fail = fail + 1;
    if (c[2] != -8) fail = fail + 1;
    if (c[3] != 3) fail = fail + 1;
    return fail;
}

int i128_float(void) {
    int fail = 0;
    __int128 a = 1;
    if ((double)a != 1.0) fail = fail + 1;
    a = -1;
    if ((double)a != -1.0) fail = fail + 1;
    a = ((__int128)1 << 53) + 1;
    if ((double)a != 9007199254740992.0) fail = fail + 1;
    a = ((__int128)1 << 100);
    if ((__int128)(double)a != a) fail = fail + 1;
    if ((__int128)1.9 != 1) fail = fail + 1;
    if ((__int128)(-1.9) != -1) fail = fail + 1;
    unsigned __int128 u = ~(unsigned __int128)0;
    if ((double)u < 3.0e38) fail = fail + 1;
    a = (__int128)(0x1p64);
    if ((a >> 64) != 1) fail = fail + 1;
    if ((int)a != 0) fail = fail + 1;
    if ((float)((__int128)3) != 3.0f) fail = fail + 1;
    if ((__int128)3.7f != 3) fail = fail + 1;
    a = 42;
    if ((__int128)(long double)a != 42) fail = fail + 1;
    return fail;
}

int main(void) {
    int fail = 0;
    fail = fail + vla_computed_outer(16);
    {
        void *p = &&once;
        int n = 8;
        int a[n];
        a[0] = 1;
        goto *p;
    once:
        if (a[0] != 1) fail = fail + 10;
    }
    fail = fail + vec_div();
    fail = fail + i128_float();
    return fail;
}
