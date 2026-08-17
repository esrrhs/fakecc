// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20030125-1.c
package main;

int main() {
    int val = 100;
    int *p = &val;
    int **pp = &p;
    if (**pp != 100) return 1;
    **pp = 200;
    if (val != 200) return 2;
    return 0;
}
