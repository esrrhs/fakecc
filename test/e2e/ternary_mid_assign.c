// expect: 8
package main;
int main() {
    /* The middle operand of ?: is a full expr — assignment allowed.
     * a == 1 picks the then-branch, which assigns b = 8. */
    int a = 1;
    int b = 0;
    int r = (a ? (b = 8) : 9);
    return r;
}
