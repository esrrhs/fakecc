// expect: 0
/* TRUNC-hidden bitwise immediates in mem-RMW (pr29797 / pr58943). */
package main;

unsigned bar(void) { return 32768; }
unsigned x[1] = { 2 };

int main(void) {
    unsigned nStyle = bar();
    if (nStyle & 32768)
        nStyle |= 65536;
    if (nStyle != (32768 | 65536))
        return 1;
    x[0] |= 128u;
    if (x[0] != 130)
        return 2;
    return 0;
}
