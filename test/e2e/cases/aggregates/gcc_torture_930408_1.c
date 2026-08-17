// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/930408-1.c
package main;

typedef enum foo E;
enum foo { e0, e1 };

struct {
  E eval;
} s;

int
p(void)
{
  return 1;

    return 0;}

void
f(void)
{
  switch (s.eval)
    {
    case e0:
      p();
    }
}

int
main(void)
{
  s.eval = e1;
  f();
  return 0;
}