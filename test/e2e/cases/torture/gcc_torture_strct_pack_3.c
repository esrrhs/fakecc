/* strct-pack-3.c */

// expect: 0
package main;

typedef struct {
  short i __attribute__((aligned(2), packed));
  int f[2] __attribute__((aligned(2), packed));
} A;

static int f(A *ap) {
  short i, j = 1;
  i = ap->f[1];
  i += ap->f[j];
  for (j = 0; j < 2; j++)
    i += ap->f[j];
  return i;
}

int main(void) {
  A a;
  a.f[0] = 100;
  a.f[1] = 13;
  if (f(&a) != 139)
    __builtin_abort();
  return 0;
}
