// expect: 1
// String literal has a non-zero address at runtime.
package main;
int main() {
    char *s = "test";
    long a = (long)s;
    if (a == 0) { return 0; }
    return 1;
}
