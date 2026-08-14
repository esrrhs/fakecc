// compound_div: the /= operator.  Exact division, truncated division,
// and division by a variable divisor.
// expect: 0
package main;
int main() {
    int x = 20;
    x /= 4;
    if (x != 5) return 1;
    x = 17;
    x /= 5;
    if (x != 3) return 2; /* truncates toward zero */
    int d = 3;
    x = 10;
    x /= d;
    if (x != 3) return 3;
    return 0;
}
