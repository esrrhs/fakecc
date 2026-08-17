// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20030714-1.c
package main;

int max(int a, int b) { return a > b ? a : b; }
int min(int a, int b) { return a < b ? a : b; }

int main() {
    if (max(3, 5) != 5) return 1;
    if (max(-1, -2) != -1) return 2;
    if (min(3, 5) != 3) return 3;
    if (min(-1, -2) != -2) return 4;
    return 0;
}
