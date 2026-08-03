// expect: 55
// Long arithmetic — verify 64-bit ops path is used.
package main;
int main() {
    long x = 100;
    long y = 200;
    long z = x + y - 245;
    return z;
}
