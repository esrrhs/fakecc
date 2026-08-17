// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20070212-1.c
package main;

struct f
{
  int i;
};

int g(int i, int c, struct f *ff, int *p)
{
  int *t;
  if (c)
   t = &i;
  else
   t = &ff->i;
  *p = 0;
  return *t;
}

int main()
{
  struct f f;
  f.i = 1;
  if (g(5, 0, &f, &f.i) != 0)
    return 1;
  return 0;
}