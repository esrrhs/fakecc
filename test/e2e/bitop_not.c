// expect: 0
package main;
int main() {
    /* ~3 == -4; -4 + 4 == 0. Verifies bitwise NOT without depending on
     * the sign of the raw exit code (exit codes are mod 256). */
    int x = ~3;
    return x + 4;
}
