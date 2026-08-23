/* PR tree-optimization/86492 */

// expect: 0
package main;

union U
{
  unsigned int r;
  struct S
  {
    unsigned int a:12;
    unsigned int b:4;
    unsigned int c:16;
  } f;
};

__attribute__((noipa)) unsigned int
foo (unsigned int x)
{
  union U u;
  u.r = 0;
  u.f.c = x;
  u.f.b = 0xe;
  return u.r;
}

int
main (void)
{
  union U u;
  u.r = foo (0x72);
  if (u.f.a != 0 || u.f.b != 0xe || u.f.c != 0x72)
    __builtin_abort ();
  return 0;
}
