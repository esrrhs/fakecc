// expect: 0
// Casts in constant initializers and array bounds must truncate: (char)256 → 0.
package main;
int x = (char)256;
int a[1 + (char)256];
int main(void) {
    if (x != 0) return 1;
    if ((int)(sizeof(a) / sizeof(a[0])) != 1) return 2;
    return 0;
}
