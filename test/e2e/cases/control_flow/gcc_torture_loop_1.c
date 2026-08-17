// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/loop-1.c
package main;

int main() {
    int i, sum = 0;
    for (i = 1; i <= 100; i++)
        sum += i;
    if (sum != 5050) return 1;
    return 0;
}
