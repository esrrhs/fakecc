// expect: 0
// Signed bitfield ++ / -- / += promote with sign extension: a 3-bit field
// holding -1 reads as -1 (not 7), and wrapping 3+1 yields -4.
package main;
struct S { int x : 3; };
int main(void) {
    struct S s;
    s.x = -1;
    int a = s.x++;
    if (a != -1) return 1;
    if (s.x != 0) return 2;
    s.x = 3;
    int b = ++s.x;
    if (b != -4) return 3;
    if (s.x != -4) return 4;
    s.x = -1;
    s.x += 1;
    if (s.x != 0) return 5;
    return 0;
}
