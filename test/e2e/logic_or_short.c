// expect: 1
package main;
int main() {
    int x = 0;
    /* 1 || <anything> must short-circuit: the assignment to x is skipped. */
    int r = (1 || (x = 5));
    return r - x;
}
