// expect: 16
// sizeof struct: two int members = 4+4 padded to 8-byte alignment = 8?
// Actually struct P{int;int;} → offsets 0,4 total 8 rounded to 8.
// Struct { int; long; } → 0, 8, total 16.
package main;
struct M { int a; long b; };
int main() {
    return sizeof(struct M);
}
