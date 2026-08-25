/* PR target/92140 */

// expect: 0
package main;

char c;
int v;

void f1 (void) { v += c != 0; }
void f2 (void) { v -= c != 0; }
void f3 (void) { v += c == 0; }
void f4 (void) { v -= c == 0; }
void f5 (void) { v += (c != 0) - 26; }
void f6 (void) { v -= (c != 0) - 26; }
void f7 (void) { v += (c == 0) - 26; }
void f8 (void) { v -= (c == 0) - 26; }
void f9 (void) { v += (c != 0) + 42; }
void f10 (void) { v -= (c != 0) + 42; }
void f11 (void) { v += (c == 0) + 42; }
void f12 (void) { v -= (c == 0) + 42; }
void f13 (int z) { v += (c == 0) + z; }
void f14 (int z) { v -= (c == 0) + z; }
unsigned int f15 (unsigned int n) { return n ? 2 : 1; }

int
main (void)
{
  int i;
  for (i = 0; i < 2; i++)
    {
      v = 15;
      if (i == 1)
	c = 37;
      f1 ();
      if (v != 15 + i)
	__builtin_abort ();
      f2 ();
      if (v != 15)
	__builtin_abort ();
      f3 ();
      if (v != 16 - i)
	__builtin_abort ();
      f4 ();
      if (v != 15)
	__builtin_abort ();
      f5 ();
      if (v != 15 + i - 26)
	__builtin_abort ();
      f6 ();
      if (v != 15)
	__builtin_abort ();
      f7 ();
      if (v != 16 - i - 26)
	__builtin_abort ();
      f8 ();
      if (v != 15)
	__builtin_abort ();
      f9 ();
      if (v != 15 + i + 42)
	__builtin_abort ();
      f10 ();
      if (v != 15)
	__builtin_abort ();
      f11 ();
      if (v != 16 - i + 42)
	__builtin_abort ();
      f12 ();
      if (v != 15)
	__builtin_abort ();
      f13 (173);
      if (v != 16 - i + 173)
	__builtin_abort ();
      f14 (173);
      if (v != 15)
	__builtin_abort ();
      f13 (-35);
      if (v != 16 - i - 35)
	__builtin_abort ();
      f14 (-35);
      if (v != 15)
	__builtin_abort ();
    }
  if (f15 (0) != 1 || f15 (1) != 2 || f15 (371) != 2)
    __builtin_abort ();
  return 0;
}
