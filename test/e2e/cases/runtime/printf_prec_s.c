// expect: 0
// %.Ns must not strlen() past the precision (the buffer need not be NUL
// terminated).  %zu must consume a full size_t, not unsigned int.
package main;
import runtime;
int main(void) {
    char buf[64];
    char raw[4];
    raw[0] = 'a'; raw[1] = 'b'; raw[2] = 'c'; raw[3] = 'd';
    if (runtime.sprintf(buf, "%.3s", raw) != 3) return 1;
    if (buf[0] != 'a' || buf[1] != 'b' || buf[2] != 'c' || buf[3] != 0) return 2;

    unsigned long zu = 3000000000ul;
    if (runtime.sprintf(buf, "%zu", zu) != 10) return 3;
    if (runtime.strcmp(buf, "3000000000") != 0) return 4;

    /* Huge %f precision must not smash the 512-byte staging buffer. */
    if (runtime.sprintf(buf, "%.8f", 1.0) < 3) return 5;
    if (runtime.sprintf(buf, "%f", 1.0) < 3) return 6;
    return 0;
}
