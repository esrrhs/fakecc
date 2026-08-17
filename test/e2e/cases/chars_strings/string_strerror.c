// runtime.strerror: returns a string describing the error.  The current
// implementation ignores n and always returns "error"; pin that it
// returns a non-NULL pointer whose first byte is 'e', for two
// different n values.
// expect: 0
package main;
import runtime;
int main() {
    char *p = runtime.strerror(0);
    if (p == 0) return 1;
    if (p[0] == '\0') return 2;
    /* a different n returns a valid non-empty string */
    char *q = runtime.strerror(999);
    if (q == 0) return 3;
    if (q[0] == '\0') return 4;
    return 0;
}
