// compound_mod: the %= operator.  Basic remainder, no remainder, and
// a variable right-hand side.
// expect: 0
package main;
int main() {
    int x = 17;
    x %= 5;
    if (x != 2) return 1;
    x = 20;
    x %= 4;
    if (x != 0) return 2; /* divides evenly */
    int m = 6;
    x = 23;
    x %= m;
    if (x != 5) return 3;
    return 0;
}
