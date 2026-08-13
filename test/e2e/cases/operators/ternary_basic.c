// expect: 12
package main;
int main() {
    int a = 1;
    int b = 0;
    /* True branch picked when cond nonzero, false branch when zero.
     * a=1 → 7, b=0 → 5, sum = 12. */
    return (a ? 7 : 9) + (b ? 3 : 5);
}
