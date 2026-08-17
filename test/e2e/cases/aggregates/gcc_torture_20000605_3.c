// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000605-3.c
package main;

struct F { int x; int y; };

int main()
{
  int timeout = 0;
  int x = 0;
  while (1)
    {
      const struct F i = { x++, };
      if (i.x > 0)
	break;
      if (++timeout > 5)
	goto die;
    }
  return 0;
 die:
  return 1;
}