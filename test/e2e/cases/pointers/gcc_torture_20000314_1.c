// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000314-1.c
package main;

int sub(int a, int b) {
    return a - b;
}

int main() {
    int (*fn)(int, int) = sub;
    if (fn(10, 4) != 6) return 1;
    return 0;
}
