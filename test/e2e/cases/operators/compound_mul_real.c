// compound_mul: the *= operator.  (The file named compound_mul.c actually
// tests <<=, so *= lives here.)  Pin the basic product, a self-product
// (x *= x), and a product whose result must survive a later read.
// expect: 0
package main;
int main() {
    int x = 5;
    x *= 3;          /* 15 */
    if (x != 15) return 1;

    int y = 7;
    y *= 1;          /* identity */
    if (y != 7) return 2;

    int z = 4;
    z *= z;          /* 16 */
    if (z != 16) return 3;

    /* result is a real value, not a discarded temporary */
    int w = 2;
    w *= 10;         /* 20 */
    if (w + 1 != 21) return 4;

    return 0;
}
