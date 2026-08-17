// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20010520-1.c
package main;

static unsigned int expr_hash_table_size = 1;

int
main ()
{
  int del = 1;
  unsigned int i = 0;

  if (i < expr_hash_table_size && del)
    return 0;
  return 1;
}