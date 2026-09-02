// expect: 1
// skip_difftest: gcc needs -std=c23 for `enum : bool`; FakeCC accepts bool here only.
package main;

enum Tiny : bool { X };

int main(void) {
    return sizeof(enum Tiny);
}
