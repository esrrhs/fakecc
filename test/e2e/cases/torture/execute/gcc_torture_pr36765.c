// expect: 0
package main;

extern void *malloc(unsigned long);

int __attribute__((noinline))
foo(int i)
{
  int *p = malloc(4 * sizeof(int));
  *p = 0;
  p[i] = 1;
  return *p;
}

extern void abort(void);

int main(void)
{
  if (foo(0) != 1)
    abort ();
  return 0;
}
