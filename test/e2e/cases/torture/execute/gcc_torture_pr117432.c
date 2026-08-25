/* PR ipa/117432 */
// expect: 0
package main;

extern void abort(void);

long long r;

__attribute__((noipa)) void
baz (int tag, ...)
{
  va_list ap;
  va_start (ap, tag);
  if (!r)
    r = va_arg (ap, long long);
  else
    r = va_arg (ap, int);
  va_end (ap);
}

void
foo (void)
{
  baz (1, -1, 0);
}

void
bar (void)
{
  baz (1, -1LL, 0);
}

__attribute__((noipa)) void
qux (...)
{
  va_list ap;
  __builtin_va_start (ap, 0);
  if (!r)
    r = va_arg (ap, long long);
  else
    r = va_arg (ap, int);
  va_end (ap);
}

void
corge (void)
{
  qux (-2, 0);
}

void
fred (void)
{
  qux (-2LL, 0);
}

void foo(void);
void bar(void);
void corge(void);
void fred(void);

int
main ()
{
  bar ();
  if (r != -1LL)
    abort ();
  foo ();
  if (r != -1)
    abort ();
  r = 0;
  fred ();
  if (r != -2LL)
    abort ();
  corge ();
  if (r != -2)
    abort ();
}
