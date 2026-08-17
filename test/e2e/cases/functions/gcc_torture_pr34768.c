// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr34768.c
// Tests nested function calls and return value propagation
package main;

int double_val(int x) { return x * 2; }
int add_one(int x)    { return x + 1; }
int square(int x)     { return x * x; }

int main() {
    /* square(add_one(double_val(3))) = square(add_one(6)) = square(7) = 49 */
    if (square(add_one(double_val(3))) != 49) return 1;
    return 0;
}
