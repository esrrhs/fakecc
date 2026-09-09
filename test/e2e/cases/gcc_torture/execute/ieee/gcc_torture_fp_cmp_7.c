// expect: 0
package main;

extern void abort(void);

void link_error (void)
{
  abort ();
}

void foo(double x)
{
  if (x > __builtin_inf())
    link_error ();
}

int main ()
{
  foo (1.0);
  return 0;
}
