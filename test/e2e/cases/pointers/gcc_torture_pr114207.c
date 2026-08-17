// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr114207.c
package main;

struct S {
    int a, b;
};

void foo (struct S *s) {
    struct S ss = (struct S) {
        .a = s->b,
        .b = s->a
    };
    *s = ss;
}

int main() {
  struct S s = {6, 12};
  foo(&s);
  if (s.a != 12 || s.b != 6)
    return 1;
  return 0;
}