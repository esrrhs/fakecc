// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/andor-1.c
// Tests short-circuit evaluation of && and ||
package main;

int side_effect;

int set_flag(int v) {
    side_effect = 1;
    return v;
}

int main() {
    side_effect = 0;
    /* 0 && set_flag(1): set_flag should NOT be called */
    if (0 && set_flag(1)) return 1;
    if (side_effect != 0) return 2;

    side_effect = 0;
    /* 1 || set_flag(1): set_flag should NOT be called */
    if (!(1 || set_flag(1))) return 3;
    if (side_effect != 0) return 4;

    return 0;
}
