// expect: 1
// Unsigned wrap: 4000000000u + 1000000000u == 705032704u (mod 2^32).
package main;
int main() {
    unsigned int x = 4000000000;
    unsigned int y = x + 1000000000;
    if (y == 705032704) { return 1; }
    return 0;
}
