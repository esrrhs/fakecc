// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/921215-1.c
// Tests do-while loop with break
package main;

int main() {
    int i = 0, count = 0;
    do {
        count++;
        i++;
        if (i == 5) break;
    } while (i < 10);
    if (count != 5) return 1;
    if (i != 5) return 2;
    return 0;
}
