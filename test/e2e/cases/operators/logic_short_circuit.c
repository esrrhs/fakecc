// expect: 0
package main;
int main() {
    int x = 0;
    /* 0 && <anything> must short-circuit: the assignment to x is skipped. */
    int r = (0 && (x = 5));
    return x + r;
}
