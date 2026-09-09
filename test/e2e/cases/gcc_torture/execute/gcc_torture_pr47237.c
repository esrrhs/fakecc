// expect: 0
package main;

/* Port of gcc.c-torture/execute/pr47237.c
 * INTEGER_ARG macro expanded. Original abort checks kept.
 * Requires GNU __builtin_apply / __builtin_apply_args. */

extern void abort(void);

static void foo(int arg)
{
  if (arg != 5)
    abort();
}

static void bar(int arg)
{
  foo(arg);
  __builtin_apply(foo, __builtin_apply_args(), 16);
}

int main(void)
{
  bar(5);

  return 0;
}
