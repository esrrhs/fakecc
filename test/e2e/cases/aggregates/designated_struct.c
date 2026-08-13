// expect: 0
// Designated struct initializer with fields given out of order: `struct S s
// = {.y = 5, .x = 1}`.  Verifies each field lands at the right offset
// regardless of source order.  Returns 0 on success or a failing sentinel.
package main;
struct S { int x; int y; };
int main() {
    struct S s = {.y = 5, .x = 1};
    if (s.x != 1) return 1;
    if (s.y != 5) return 2;
    return 0;
}
