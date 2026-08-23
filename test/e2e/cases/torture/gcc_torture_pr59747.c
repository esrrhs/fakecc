// expect: 0
package main;

extern void abort(void);
extern void exit(int);

int a[6], c = 1, d;
short e;

int fn1(int p)
{
  return a[p];
}

int main(void)
{
  a[0] = 1;
  if (c)
    e--;
  d = e;
  long long f = e;
  if (fn1 ((f >> 56) & 1) != 0)
    abort ();
  exit (0);
}
