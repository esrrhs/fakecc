// expect: 0
package main;

/* Ported from GCC C-Torture: gcc.c-torture/execute/pr80692.c
 * _Decimal64 signed-zero compare: -0.DD == 0.DD and -0.DD == -0.DD. */

int main() {
    _Decimal64 d64 = -0.DD;

    if (d64 != 0.DD)
        __builtin_abort();

    if (d64 != -0.DD)
        __builtin_abort();

    return 0;
}
