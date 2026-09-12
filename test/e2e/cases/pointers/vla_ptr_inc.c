// expect: 0
// Pointer increment over a VLA array type strides by the current bound,
// matching p+1.
package main;
int main(void) {
    int n = 3;
    int a[2][n];
    int i;
    for (i = 0; i < n; i++) {
        a[0][i] = 10 + i;
        a[1][i] = 20 + i;
    }
    int (*p)[n] = a;
    p++;
    if ((*p)[0] != 20) return 1;
    p = a;
    p = p + 1;
    if ((*p)[0] != 20) return 2;
    return 0;
}
