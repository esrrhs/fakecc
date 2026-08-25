/* zero-struct-1.c */

// expect: 0
package main;

struct g {};

static char y[3];
static char *f = &y[0];
static char *ff = &y[0];

static void h(void) {
  struct g t;
  *((struct g *)(f++)) = *((struct g *)(ff++));
  *((struct g *)(f++)) = (struct g){};
  t = *((struct g *)(ff++));
}

int main(void) {
  h();
  if (f != &y[2])
    __builtin_abort();
  if (ff != &y[2])
    __builtin_abort();
  return 0;
}
