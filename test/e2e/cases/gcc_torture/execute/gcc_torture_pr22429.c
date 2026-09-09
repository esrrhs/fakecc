// expect: 0
package main;

extern void abort(void);

enum { N = 1 << (sizeof(int) * 8 - 2) };

int f(int n)
{
  if (-N <= n && n <= N-1)
    return 1;
  return 0;
}

int main (void)
{
  if (f (N))
    abort ();
  return 0;
}
