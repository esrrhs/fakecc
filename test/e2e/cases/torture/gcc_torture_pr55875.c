// expect: 0
package main;

extern void exit(int);
extern void abort(void);

int a[251];

__attribute__ ((noinline))
int t(int i)
{
  if (i==0)
    exit(0);
  if (i>255)
    abort ();
  return 0;
}

int main(void)
{
  unsigned int i;
  for (i=0;;i++)
    {
      a[i]=t((unsigned char)(i+5));
    }
  return 0;
}
