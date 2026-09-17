// expect: 0
// `%lc` converts a wchar (int) code point to UTF-8.
package main;
import runtime;
int main(void) {
    char buf[8];
    buf[0] = 'Z';
    buf[1] = 'Z';
    buf[2] = 'Z';
    if (runtime.sprintf(buf, "%lc", 'A') != 1) return 1;
    if (buf[0] != 'A' || buf[1] != 0) return 2;
    return 0;
}
