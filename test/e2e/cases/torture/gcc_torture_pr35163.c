// expect: 0
package main;

extern void abort(void);

int main(void)
{
  signed char a = -30;
  signed char b = -31;
  if (a > (unsigned short)b)
    abort ();
  return 0;
}
