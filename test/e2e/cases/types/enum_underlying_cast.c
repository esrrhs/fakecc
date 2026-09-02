// expect: 1
// skip_difftest: gcc needs -std=c23 for `enum : bool`; FakeCC accepts bool here only.
package main;

enum Bit : bool { ZERO, ONE };

int main(void) {
    enum Bit b = (enum Bit)1;
    return b;
}
