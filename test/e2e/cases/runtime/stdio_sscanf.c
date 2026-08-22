// runtime.sscanf / runtime.fscanf: %n, length modifiers, %x/%ld, assignment
// suppression.  Must match glibc conversion and assignment counts.
// expect: 0
package main;
import runtime;
int main() {
    int a = 0, n = -1, r;
    long L = 0;
    unsigned int u = 0;
    long long ll = 0;
    char buf[16];
    short sh = 0;

    r = runtime.sscanf("12 34", "%d%n", &a, &n);
    if (r != 1) return 1;
    if (a != 12) return 2;
    if (n != 2) return 3;

    r = runtime.sscanf("ff 3000000000", "%x %ld", &u, &L);
    if (r != 2) return 4;
    if (u != 255u) return 5;
    if (L != 3000000000L) return 6;

    r = runtime.sscanf("hello", "%*s%n", &n);
    if (r != 0) return 7;
    if (n != 5) return 8;

    r = runtime.sscanf("7abc", "%d%s", &a, buf);
    if (r != 2) return 9;
    if (a != 7) return 10;
    if (buf[0] != 'a' || buf[1] != 'b' || buf[2] != 'c') return 11;

    r = runtime.sscanf("-42", "%ld", &L);
    if (r != 1 || L != -42L) return 12;

    r = runtime.sscanf("1234567890123", "%lld", &ll);
    if (r != 1 || ll != 1234567890123LL) return 13;

    r = runtime.sscanf("z", "%c", buf);
    if (r != 1 || buf[0] != 'z') return 14;

    r = runtime.sscanf("0xff", "%i", &a);
    if (r != 1 || a != 255) return 15;

    r = runtime.sscanf("077", "%i", &a);
    if (r != 1 || a != 63) return 16;

    r = runtime.sscanf("9", "%hd", &sh);
    if (r != 1 || sh != 9) return 17;

    return 0;
}
