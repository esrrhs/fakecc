// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr23047.c
// Tests signed vs unsigned comparison edge cases
package main;

int main() {
    signed int si = -1;
    unsigned int ui = 1U;
    /* signed -1 vs unsigned 1: when both promoted to unsigned, -1 > 1 */
    if ((unsigned int)si < ui) return 1;  /* (unsigned)-1 = 0xFFFFFFFF > 1 */
    if (si > 0) return 2;                 /* signed -1 is not > 0 */
    return 0;
}
