// str.strerror: returns a string describing the error.  The current
// implementation ignores n and always returns "error"; pin that it
// returns a non-NULL pointer whose first byte is 'e', for two
// different n values.
// expect: 0
package main;
import str;
int main() {
    char *p = str.strerror(0);
    if (p == 0) return 1;
    if (p[0] != 'e') return 2;
    /* a different n returns the same constant pointer */
    char *q = str.strerror(999);
    if (q == 0) return 3;
    if (q[0] != 'e') return 4;
    return 0;
}
