// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20100323-1.c
// Tests enum usage in switch
package main;

typedef int direction;

int move(direction d) {
    switch (d) {
    case 0: return 10;  /* NORTH */
    case 1: return 20;  /* SOUTH */
    case 2: return 30;  /* EAST */
    case 3: return 40;  /* WEST */
    }
    return -1;
}

int main() {
    if (move(0) != 10) return 1;
    if (move(1) != 20) return 2;
    if (move(2) != 30) return 3;
    if (move(3) != 40) return 4;
    return 0;
}
