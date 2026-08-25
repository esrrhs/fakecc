/* struct-ini-1.c */

// expect: 0
package main;

struct S {
  char f1;
  int f2[2];
};

static struct S object = {'X', 8, 9};

int main(void) {
  if (object.f1 != 'X' || object.f2[0] != 8 || object.f2[1] != 9)
    __builtin_abort();
  return 0;
}
