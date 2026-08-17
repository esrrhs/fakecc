// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20021219-1.c
package main;

/* PR optimization/8988 */
/* Contributed by Kevin Easton */

void foo(char *p1, char **p2)
{}
 
int main(void)
{
  char str[] = "foo { xx }";
  char *ptr = str + 5;

  foo(ptr, &ptr);

  while (*ptr && (*ptr == 13 || *ptr == 32))
    ptr++;

  return 0;
}