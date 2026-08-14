// ge: the >= comparison operator.  Pin the true-strict case, the
// equal case (greater OR equal), the false case, and a signed-negative
// case.
// expect: 0
package main;
int main() {
    if (!(5 >= 3)) return 1;
    if (!(5 >= 5)) return 2; /* equal: satisfies >= */
    if (3 >= 5) return 3;
    if (-1 >= 0) return 4;
    return 0;
}
