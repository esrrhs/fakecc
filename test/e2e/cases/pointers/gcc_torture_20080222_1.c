// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20080222-1.c
package main;

struct container
{
  unsigned char data[1];
};

unsigned char space[6] = {1, 2, 3, 4, 5, 6};

int
foo (struct container *p)
{
  return p->data[4];
}

int
main ()
{
  if (foo ((struct container *) space) != 5)
    return 1;
  return 0;
}