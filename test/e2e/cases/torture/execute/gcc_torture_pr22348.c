// expect: 0
package main;

extern void abort(void);

void f(int i)
{
  if (i>4 + 3 * 16)
    abort();
}

int main(void)
{
  unsigned int buflen, i;
  buflen = 4 + 3 * 16;
  for (i = 4; i < buflen; i+= 3)
    f(i);
  return 0;
}
