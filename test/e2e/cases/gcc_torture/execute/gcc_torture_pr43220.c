// expect: 0
package main;

void *volatile p;

int
main (void)
{
  int n = 0;
lab:;
    {
      int x[1000 + 1];
      x[0] = 1;
      x[1000] = 2;
      p = x;
      n++;
    }

    {
      int x[1000 + 1];
      x[0] = 1;
      x[1000] = 2;
      p = x;
      n++;
    }

  if (n < 1000000)
    goto lab;

  return 0;
}
