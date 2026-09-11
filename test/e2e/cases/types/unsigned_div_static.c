// expect: 0
// Unsigned division in a static initializer: 1u / -1 → 1 / UINT_MAX → 0.
package main;
unsigned x = 1u / -1;
int main(void) {
    if (x != 0) return 1;
    return 0;
}
