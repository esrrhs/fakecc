/* PR target/60072 */

// expect: 0
package main;

int c = 1;

static int *foo(int *p)
{
  return p;
}

int main(void)
{
  *foo(&c) = 2;
  return c - 2;
}
