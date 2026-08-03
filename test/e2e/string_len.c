// expect: 5
// strlen over a string literal.
package main;
int slen(char *s) {
    int n = 0;
    while (s[n] != 0) { n = n + 1; }
    return n;
}
int main() {
    return slen("hello");
}
