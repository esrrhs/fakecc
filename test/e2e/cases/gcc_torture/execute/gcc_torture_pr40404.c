// expect: 0
package main;

extern void abort(void);

struct S {
  unsigned int ui17 : 17;
} s;

int main(void)
{
  s.ui17 = 0x1ffff;
  if (s.ui17 >= 0xfffffffeu)
    abort ();
  return 0;
}
