// expect: 0
// Unsigned usual-arithmetic conversions in constant expressions: 1u < -1
// is true (1 compared with UINT_MAX), including in array bounds and enums.
package main;
int x = 1u < -1;
int y = 1u > -1;
int a[1u < -1 ? 10 : 20];
enum { N = 1u < -1 };
int main(void) {
    if (x != 1) return 1;
    if (y != 0) return 2;
    if ((int)(sizeof(a) / sizeof(a[0])) != 10) return 3;
    if (N != 1) return 4;
    return 0;
}
