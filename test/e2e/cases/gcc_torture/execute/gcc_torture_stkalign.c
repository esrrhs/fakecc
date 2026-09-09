// expect: 0
package main;

/* Check that stack alignment is not affected by variables not placed
   on the stack.  */

extern void abort(void);

enum { ALIGNMENT = 64 };

unsigned test(unsigned n, unsigned p)
{
  static struct { char __attribute__((__aligned__(ALIGNMENT))) c; } s;
  unsigned x;

  if (__alignof__(s) != ALIGNMENT) abort();
  x = (unsigned)(unsigned long)&x;

  return n ? test(n - 1, x) : (x ^ p);
}

unsigned test2(unsigned n, unsigned p)
{
  static struct { char c; } s;
  unsigned x;

  if (__alignof__(s) == ALIGNMENT) abort();
  x = (unsigned)(unsigned long)&x;

  return n ? test2(n - 1, x) : (x ^ p);
}

int main (int argc, char *argv[])
{
  unsigned int x, y;

  x = test(argc, 0);
  x |= test(argc + 1, 0);
  x |= test(argc + 2, 0);

  y = test2(argc, 0);
  y |= test2(argc + 1, 0);
  y |= test2(argc + 2, 0);

  return (x & (ALIGNMENT - 1)) == 0 && (y & (ALIGNMENT - 1)) != 0 ? 1 : 0;
}
