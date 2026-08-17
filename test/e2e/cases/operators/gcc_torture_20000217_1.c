// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000217-1.c
package main;

int check(unsigned int a, unsigned int b) {
    if (a < b) return 1;
    return 0;
}

int main() {
    unsigned int x = 0x80000000U;
    unsigned int y = 1U;
    if (check(x, y) != 0) return 1;
    if (check(y, x) != 1) return 2;
    return 0;
}
