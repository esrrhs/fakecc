// gt: the > comparison operator.  Pin the true case, the false case
// (a not greater than b), equality (not greater), and a signed-negative
// case where -1 is NOT greater than 0.
// expect: 0
package main;
int main() {
    if (!(5 > 3)) return 1;
    if (3 > 5) return 2;
    if (5 > 5) return 3; /* equal: not greater */
    if (-1 > 0) return 4; /* signed comparison */
    return 0;
}
