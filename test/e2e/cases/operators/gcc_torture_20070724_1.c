// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20070724-1.c
package main;

static unsigned char magic[] = "\235";
static unsigned char value = '\235';

int main()
{
  if (value != magic[0])
    return 1;
  return 0;
}