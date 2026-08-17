// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/mayalias-1.c
// Tests that aliased pointer writes are visible through another pointer
package main;

int main() {
    int x = 42;
    int *p = &x;
    int *q = &x;
    *p = 100;
    if (*q != 100) return 1;
    *q = 200;
    if (x != 200) return 2;
    return 0;
}
