// expect: 0
// __builtin_choose_expr: nonzero constant selects the second argument, zero
// selects the third.  The unselected branch is discarded, so types need not
// match.  sizeof of a constant is a valid condition.
package main;

int main(void) {
    if (__builtin_choose_expr(1, 10, 20) != 10) return 1;
    if (__builtin_choose_expr(0, 10, 20) != 20) return 2;
    if (__builtin_choose_expr(1 + 1, 7, 8) != 7) return 3;
    if (__builtin_choose_expr(1 - 1, 7, 8) != 8) return 4;
    if (__builtin_choose_expr(sizeof(int), 1, 0) != 1) return 5;
    if (__builtin_choose_expr(sizeof(char) - 1, 1, 0) != 0) return 6;
    if (sizeof(__builtin_choose_expr(1, (char)1, 1000)) != 1) return 7;
    if (sizeof(__builtin_choose_expr(0, (char)1, 1000)) != sizeof(int)) return 8;
    return 0;
}
