// expect: 0
// Ported from GCC C-Torture suite: gcc.c-torture/execute/960321-1.c
package main;

int calc(int a, int b, int c, int d, int e, int f, int g, int h) {
    return a * 1 + b * 2 + c * 3 + d * 4 + e * 5 + f * 6 + g * 7 + h * 8;
}

int main() {
    int res = calc(1, 1, 1, 1, 1, 1, 1, 1);
    if (res != 36) return 1;
    return 0;
}
