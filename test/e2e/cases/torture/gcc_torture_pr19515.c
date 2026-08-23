/* PR 19515 */

// expect: 0
package main;

typedef union {
      char a2[8];
}aun;

extern void abort(void);

int main(void)
{
  aun a = {{0}};

  if (a.a2[2] != 0)
    abort ();
  return 0;
}
