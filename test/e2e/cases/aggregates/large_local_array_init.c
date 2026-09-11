// expect: 0
// Local arrays bigger than STRUCT_COPY_MEMCPY_THRESHOLD are zeroed with
// memset and only the explicit initializers are stored.  Check that a short
// string / sparse list still leaves the tail zero and hits the right slots.
package main;
int main() {
    char s[128] = "A";
    int a[40] = {1, 2, [10] = 9};
    if (s[0] != 'A') return 1;
    if (s[1] != 0) return 2;
    if (s[127] != 0) return 3;
    if (a[0] != 1) return 4;
    if (a[1] != 2) return 5;
    if (a[2] != 0) return 6;
    if (a[10] != 9) return 7;
    if (a[39] != 0) return 8;
    return 0;
}
