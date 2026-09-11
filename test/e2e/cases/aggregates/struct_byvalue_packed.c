// expect: 42
// Packed `{char; int}` is MEMORY-class (unaligned int).  Passed by value on
// the stack eightbytes, not as a GP pointer.
package main;
struct __attribute__((packed)) P { char c; int x; };
int f(struct P p) { return p.x; }
int main(void) {
    struct P p;
    p.c = 1;
    p.x = 42;
    return f(p);
}
