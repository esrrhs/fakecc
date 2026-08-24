/* PR middle-end/98366 */

// expect: 0
package main;

typedef struct S { int a, b, c : 7, d : 8, e : 17; } S;
const S f[] = { {0, 3, 4, 2, 0} };

int
main (void)
{
  S cmp = {0, 3, 4, 2, 0};
  if (__builtin_memcmp (f, &cmp, sizeof (S)))
    __builtin_abort ();
  return 0;
}
