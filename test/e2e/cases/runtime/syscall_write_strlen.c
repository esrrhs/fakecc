// expect: 5
// strlen via loop over a string literal, then confirm write returns the
// same length.
package main;
int strlen(char *s) {
    int n = 0;
    while (s[n] != 0) { n = n + 1; }
    return n;
}
int main() {
    char *msg = "hello";
    int n = strlen(msg);
    long w = __syscall(1, 1, msg, n);
    if ((int)w != n) { return 99; }
    return n;
}
