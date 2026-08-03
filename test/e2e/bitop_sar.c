// expect: 0
package main;
int main() {
    /* Arithmetic right shift of a signed negative value preserves the sign:
     * -8 >> 1 == -4.  Verify via -4 + 4 == 0 (exit codes are mod 256, so
     * we can't observe -4 directly). */
    int x = -8 >> 1;
    return x + 4;
}
