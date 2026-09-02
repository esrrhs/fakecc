// expect: 1
// skip_difftest: gcc needs -std=c23 for `enum : bool`; FakeCC accepts bool here only.
package main;

enum Flag : bool { OFF = 0, ON = 1 };

int main(void) {
    enum Flag f = ON;
    if (f != ON) return 2;
    if (f != 1) return 3;
    return f;
}
