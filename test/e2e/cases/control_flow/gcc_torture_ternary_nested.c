// expect: 0
// Ported from GCC C-Torture suite: gcc.c-torture/execute/920721-1.c
package main;

int f(int a, int b) {
    return a > 0 ? (b > 0 ? a + b : a - b) : (b > 0 ? -a + b : -a - b);
}

int main() {
    if (f(1, 2) != 3) return 1;
    if (f(1, -2) != 3) return 2;
    if (f(-1, 2) != 3) return 3;
    if (f(-1, -2) != 3) return 4;
    return 0;
}
