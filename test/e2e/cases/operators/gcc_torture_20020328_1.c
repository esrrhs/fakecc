// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020328-1.c
package main;

int b = 0;

void func (void) { }

int
testit(int x)
{
  if (x != 20)
    return 1;

    return 0;}

int
main()

{
  int a = 0;

  if (b)
    func();

  /* simplify_and_const_int would incorrectly omit the mask in
     the line below.  */
  testit ((a + 23) & 0xfffffffc);
  return 0;
}