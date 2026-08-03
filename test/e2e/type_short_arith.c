// expect: 232
// 3000 % 256 == 184.  But we want to exercise short arithmetic — return
// the low byte of a short sum: (1000 + 2000) & 0xff = 3000 mod 256 = 184.
// Actually exit codes only take low 8 bits; short(3000) mod 256 = 184.
// Adjust: test 200+32=232 which fits in char range so we can verify directly.
package main;
int main() {
    short a = 200;
    short b = 32;
    short s = a + b;
    return s;
}
