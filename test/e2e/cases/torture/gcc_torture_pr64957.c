/* PR rtl-optimization/64957 */

// expect: 0
package main;

int foo(int b)
{
  return (((b ^ 5) | 1) ^ 5) | 1;
}

int bar(int b)
{
  return (((b ^ ~5) & ~1) ^ ~5) & ~1;
}

int main(void)
{
  int i;
  for (i = 0; i < 65536; i++)
    if (foo(i) != (i | 1) || bar(i) != (i & ~1))
      __builtin_abort();
  return 0;
}
