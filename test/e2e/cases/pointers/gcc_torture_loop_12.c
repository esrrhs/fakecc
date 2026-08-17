// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/loop-12.c
package main;

/* Checks that pure functions are not treated as const.  */

char *p;

static int 
is_end_of_statement (void)
{
  return *p == '\n' || *p == ';' || *p == '!';
}

void foo (void)
{
  /* The is_end_of_statement call was moved out of the loop at one stage,
     resulting in an endless loop.  */
  while (!is_end_of_statement ())
    p++;
}

int
main (void)
{
  p = "abc\n";
  foo ();
  return 0;
}