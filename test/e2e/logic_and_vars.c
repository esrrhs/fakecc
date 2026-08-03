// expect: 1
package main;
int main() {
    int a = 10;
    int b = 20;
    /* Both nonzero → 1. Also exercises non-constant operands. */
    return (a && b);
}
