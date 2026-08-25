// expect: 0
package main;

extern void abort(void);

int foo = 0;
void *bar = 0;
unsigned int baz = 100;

void *pure_alloc (void)
{
  void *res;

  while (1)
    {
      res = (void *) ((((unsigned int)((char *)bar + foo))) & ~1);
      foo += 2;
      if (foo < baz)
        return res;
      foo = 0;
    }
}

int main (void)
{
  pure_alloc ();
  if (!foo)
    abort ();
  return 0;
}
