// compound_sub: the -= operator.  Basic subtraction, subtraction to a
// negative result, and subtraction from zero.
// expect: 0
package main;
int main() {
    int x = 10;
    x -= 3;
    if (x != 7) return 1;
    x -= 10;
    if (x != -3) return 2;
    int y = 5;
    y -= 5;
    if (y != 0) return 3;
    return 0;
}
