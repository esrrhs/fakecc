// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/920202-1.c
package main;

static int rule_text_needs_stack_pop = 0;
static int input_stack_pos = 1;

int
f (void)
{
  rule_text_needs_stack_pop = 1;

  if (input_stack_pos <= 0)
    return 1;
  else
    return 0;
}

int
main (void)
{
  f ();
  return 0;
}