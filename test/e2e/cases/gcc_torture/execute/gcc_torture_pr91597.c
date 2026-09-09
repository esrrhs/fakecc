/* PR tree-optimization/91597 */

// expect: 0
package main;

enum E { A, B, C };
struct __attribute__((aligned (4))) S { enum E e; };

enum E
foo (struct S *o)
{
  if (((long) (o) & 1) == 0)
    return o->e;
  else
    return A;
}

int
bar (struct S *o)
{
  return foo (o) == B || foo (o) == C;
}

static inline void
baz (struct S *o, int d)
{
  if (!bar (o))
    __builtin_abort ();
  if (d > 2) return;
  baz (o, d + 1);
}

void
qux (struct S *o)
{
  switch (o->e)
    {
    case A: return;
    case B: baz (o, 0); break;
    case C: baz (o, 0); break;
    }
}

struct S s = { C };

int
main (void)
{
  qux (&s);
  return 0;
}
