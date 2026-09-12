// expect: 5
// Bitfield width may be an imported enum constant.
package main;
import flags;
struct S { unsigned x : flags.FLAG_C; };
int main(void) {
    struct S s;
    s.x = 5;
    return (int)s.x;
}
