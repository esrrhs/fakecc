/* PR tree-optimization/98727 */

// expect: 0
package main;

long int foo(long int x, long int y) {
  long int z = (unsigned long) x * y;
  if (x != z / y)
    return -1;
  return z;
}

int main(void) {
  if (foo (4, 24) != 96
      || foo (124, 126) != 124L * 126
      || foo (0x3fffffffffffffffl, 17) != -1)
    __builtin_abort ();
  return 0;
}
