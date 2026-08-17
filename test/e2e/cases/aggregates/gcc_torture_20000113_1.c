// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000113-1.c
package main;

struct bar {
    int a;
    int b;
};

struct foo {
    int c;
    struct bar d;
};

int main() {
    struct foo f;
    f.d.b = 99;
    if (f.d.b != 99) return 1;
    return 0;
}
