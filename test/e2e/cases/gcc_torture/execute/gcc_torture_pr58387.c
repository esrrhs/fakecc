// expect: 0
package main;

extern void abort(void);

int a = -1; 

int main (void)
{
  int b = a == 0 ? 0 : -a;
  if (b < 1)
    abort ();
  return 0;
}
