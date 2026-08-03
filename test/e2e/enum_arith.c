// expect: 3
package main;
enum Num { ZERO, ONE, TWO, THREE };
int main() {
    /* Enum constants in arithmetic: THREE == 3. */
    return THREE + ZERO;
}
