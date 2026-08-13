// expect: 9
package main;
int main() {
    int a = 0;
    int b = 1;
    /* Ternary result feeds into a variable and into an assignment RHS. */
    int r = (a ? 12 : 9);
    r = (b ? 9 : 12);
    return r;
}
