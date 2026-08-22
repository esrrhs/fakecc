// runtime.strtod / runtime.strtold: hex float literals with more than 16 hex
// digits must round like glibc, not drop low digits into the exponent.
// expect: 0
package main;
import runtime;
int main() {
    /* Host glibc parses the matching literals in this file (compiled by
     * gcc-built fakecc); runtime.strto* must land on the same value. */
    if (runtime.strtod("0x1.0p0", 0) != 0x1.0p0) return 1;
    if (runtime.strtod("0x1.fffffffffffffp0", 0) != 0x1.fffffffffffffp0) return 2;

    if (runtime.strtold("0x123456789abcdef123456789p0", 0)
        != 0x123456789abcdef123456789p0L) return 3;

    if (runtime.strtold("0x1.0000000000000008p0", 0)
        != 0x1.0000000000000008p0L) return 4;

    char *end;
    runtime.strtod("0x1.2p0xyz", &end);
    if (end[0] != 'x') return 5;

    return 0;
}
