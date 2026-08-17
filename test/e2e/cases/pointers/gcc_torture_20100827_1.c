// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20100827-1.c
package main;

int 
foo (char *p)
{
  int h = 0;
  do
    {
      if (*p == '\0')
	break;
      ++h;
      if (p == 0)
	return 1;
      ++p;
    }
  while (1);
  return h;
}
int main()
{
  if (foo("a") != 1)
    return 1;
  return 0;
}