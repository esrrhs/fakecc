// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/loop-15.c
package main;

/* Bombed with a segfault on powerpc-linux.  doloop.c generated wrong
   loop count.  */

void
foo (unsigned long *start, unsigned long *end)
{
  unsigned long *temp = end - 1;

  while (end > start)
    *end-- = *temp--;
}

int
main (void)
{
  unsigned long a[5];
  int start, end, k;

  for (start = 0; start < 5; start++)
    for (end = 0; end < 5; end++)
      {
	for (k = 0; k < 5; k++)
	  a[k] = k;

	foo (a + start, a + end);

	for (k = 0; k <= start; k++)
	  if (a[k] != k)
	    return 1;

	for (k = start + 1; k <= end; k++)
	  if (a[k] != k - 1)
	    return 1;

	for (k = end + 1; k < 5; k++)
	  if (a[k] != k)
	    return 1;
      }

  return 0;
}