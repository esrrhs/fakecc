// expect: 0
// `_Alignof` operator: verify it returns the natural alignment of several
// types.  fakecc uses width-based alignment (int→4, double→8, pointer→8,
// struct→8).  Returns 0 on success or a failing sentinel for the first
// mismatch.
package main;
struct S { int a; int b; };
int main() {
    if (_Alignof(int) != 4) return 1;
    if (_Alignof(double) != 8) return 2;
    if (_Alignof(char) != 1) return 3;
    if (_Alignof(int *) != 8) return 4;
    if (_Alignof(struct S) != 4) return 5;
    return 0;
}
