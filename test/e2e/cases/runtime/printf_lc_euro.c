// expect: 0
// skip_difftest
// `%lc` of U+20AC is UTF-8 e2 82 ac; `%ls` of NULL is "(null)".
// glibc `%lc` is locale-dependent (C locale cannot encode U+20AC).
package main;
import runtime;
int main(void) {
    char buf[16];
    int *p;
    buf[0] = 'Z';
    buf[1] = 'Z';
    buf[2] = 'Z';
    if (runtime.sprintf(buf, "%lc", 0x20ac) != 3) return 1;
    if ((unsigned char)buf[0] != 0xe2) return 2;
    if ((unsigned char)buf[1] != 0x82) return 3;
    if ((unsigned char)buf[2] != 0xac) return 4;
    p = 0;
    if (runtime.sprintf(buf, "%ls", p) != 6) return 5;
    if (buf[0] != '(' || buf[5] != ')') return 6;
    return 0;
}
