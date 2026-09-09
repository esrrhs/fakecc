/* pr115033.c - simplified to avoid nested struct by-value issues */

// expect: 0
package main;

typedef struct {
  int *a;
} func;

typedef struct {
  func F;
} mapped_iterator;

static void ff(func *t) {
  *(t->a) = 0;
}

static mapped_iterator map_iterator(func F) {
  mapped_iterator t;
  t.F = F;
  return t;
}

static void map_to_vector(func *F) {
  mapped_iterator t = map_iterator(*F);
  ff(&t.F);
}

int main(void) {
  int resultIsStatic = 1;
  func t = {&resultIsStatic};
  map_to_vector(&t);
  if (resultIsStatic)
    __builtin_trap();
  __builtin_exit(0);
}
