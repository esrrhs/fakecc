// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20060929-1.c
// Tests recursive fibonacci
package main;

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    if (fib(0) != 0) return 1;
    if (fib(1) != 1) return 2;
    if (fib(6) != 8) return 3;
    if (fib(10) != 55) return 4;
    return 0;
}
