// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/bitfld-10.c
package main;

/* PR tree-optimization/102622 */
/* Wrong code introduced due to phi-opt
   introducing undefined signed interger overflow
   with one bit signed integer negation. */

struct f{signed t:1;};
int g(struct f *a, int t) ;
int g(struct f *a, int t)
{
    if (t)
      a->t = -1;
    else
      a->t = 0;
    int t1 = a->t;
    if (t1) return 1;
    return t1;
}

int main(void)
{
    struct f a;
    if (!g(&a, 1))  return 1;
    return 0;
}