// expect: 0
package main;
int main() {
    /* The unevaluated branch must NOT be evaluated — mirrors && / ||.
     * Here the false-branch has (x = 5) which must be skipped. */
    int x = 0;
    int r = (1 ? 0 : (x = 5));
    return x + r;
}
