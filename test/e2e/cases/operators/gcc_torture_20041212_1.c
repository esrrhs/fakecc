// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20041212-1.c
package main;

/* A function pointer compared with a void pointer should not be canonicalized.
   See PR middle-end/17564.  */

void *f (void) ;
void *
f (void)
{
  return f;
}
int
main (void)
{
  if (f () != f)
    return 1;
  return 0;
}