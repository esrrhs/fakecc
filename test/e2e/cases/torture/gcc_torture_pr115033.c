// expect: 0
package main;

extern void exit(int);
extern void abort(void);
void abort(void);

typedef struct func
{
  int *a;
}func;

void ff(struct func *t)
{
  *(t->a) = 0;
}

typedef struct mapped_iterator {
  func F;
}mapped_iterator;

mapped_iterator map_iterator(func F) {
  mapped_iterator t = {F};
  return t;
}

void map_to_vector(func *F) {
  mapped_iterator t = map_iterator(*F);
  ff(&t.F);
}

int main() {
  int resultIsStatic = 1;
  func t ={&resultIsStatic};
  map_to_vector(&t);

  if (resultIsStatic)
    abort();
  exit(0);
  return 0;
}
