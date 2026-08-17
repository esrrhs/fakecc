// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20080522-1.c
package main;

/* This testcase is to make sure we have i in referenced vars and that we
   properly compute aliasing for the loads and stores.  */

static int i;
static int *p = &i;

int 
foo(int *q)
{
  *p = 1;
  *q = 2;
  return *p;
}

int 
bar(int *q)
{
  *q = 2;
  *p = 1;
  return *q;
}

int main()
{
  int j = 0;

  if (foo(&i) != 2)
    return 1;
  if (bar(&i) != 1)
    return 1;
  if (foo(&j) != 1)
    return 1;
  if (j != 2)
    return 1;
  if (bar(&j) != 2)
    return 1;
  if (j != 2)
    return 1;

  return 0;
}