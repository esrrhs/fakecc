// expect: 0
// scalar_storage_order("little-endian") is a no-op on SysV x86-64.
package main;
struct S {
    unsigned x;
} __attribute__((scalar_storage_order("little-endian")));
int main(void) {
    struct S s;
    s.x = 0x01020304u;
    unsigned char *p = (unsigned char *)&s;
    if (p[0] != 4) return 1;
    if (p[1] != 3) return 2;
    if (p[2] != 2) return 3;
    if (p[3] != 1) return 4;
    if (s.x != 0x01020304u) return 5;
    return 0;
}
