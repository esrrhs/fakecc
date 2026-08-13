// expect: 7
// MEMORY-class struct (>16 bytes): SysV passes eightbytes on the stack.
package main;
struct Big { long a; long b; long c; };
long sum3(struct Big s) { return s.a + s.b + s.c; }
int main(void) {
    struct Big s;
    s.a = 1; s.b = 2; s.c = 4;
    return (int)sum3(s);
}
