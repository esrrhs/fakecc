// expect: 0
// scalar_storage_order("big-endian") must bswap 64-bit scalars, not only 16/32.
package main;
struct S {
    unsigned long long x;
} __attribute__((scalar_storage_order("big-endian")));
int main(void) {
    struct S s;
    s.x = 0x0102030405060708ULL;
    unsigned char *p = (unsigned char *)&s;
    if (p[0] != 1) return 1;
    if (p[1] != 2) return 2;
    if (p[2] != 3) return 3;
    if (p[3] != 4) return 4;
    if (p[4] != 5) return 5;
    if (p[5] != 6) return 6;
    if (p[6] != 7) return 7;
    if (p[7] != 8) return 8;
    if (s.x != 0x0102030405060708ULL) return 9;
    return 0;
}
