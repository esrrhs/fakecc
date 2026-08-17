// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000801-2.c
package main;

int bar(void);
int baz(void);

struct foo {
  struct foo *next;
};

struct foo *test(struct foo *node)
{
  while (node) {
    if (bar() && !baz())
      break;
    node = node->next;
  }
  return node;
}

int bar (void)
{
  return 0;
}

int baz (void)
{
  return 0;
}

int main(void)
{
  struct foo a, b, *c;

  a.next = &b;
  b.next = (struct foo *)0;
  c = test(&a);
  if (c)
    return 1;
  return 0;
}