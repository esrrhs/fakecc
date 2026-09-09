/* PR target/49186 */

// expect: 0
package main;

extern void abort(void);

int
main (void)
{
  int x;
  unsigned long long uv = 0x1000000001ULL;

  x = (uv < 0x80) ? 1 : ((uv < 0x800) ? 2 : 3);
  if (x != 3)
    abort ();

  return 0;
}
