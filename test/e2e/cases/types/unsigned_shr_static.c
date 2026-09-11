// expect: 0
// Unsigned right shift in a static initializer is logical, not arithmetic.
package main;
unsigned x = (~0u) >> 1;
unsigned long long y = (~0ULL) >> 1;
int main(void) {
    if (x != 0x7fffffffU) return 1;
    if (y != 0x7fffffffffffffffULL) return 2;
    return 0;
}
