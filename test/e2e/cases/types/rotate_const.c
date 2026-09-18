// expect: 0
// (x<<n)|(x>>(32-n)) should lower to IR_ROL and const-fold.
package main;

int main(void) {
    unsigned a = (1u << 1) | (1u >> 31);
    unsigned b = (0x80000000u >> 1) | (0x80000000u << 31);
    if (a != 2u) return 1;
    if (b != 0x40000000u) return 2;
    unsigned x = 1u;
    unsigned n = 1u;
    unsigned r = (x << n) | (x >> (32u - n));
    if (r != 2u) return 3;
    return 0;
}
