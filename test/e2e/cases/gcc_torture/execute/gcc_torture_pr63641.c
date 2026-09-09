// expect: 0
package main;

/* PR tree-optimization/63641 */

extern void abort (void);

int
foo (unsigned char b)
{
  if (0x0 <= b && b <= 0x8)
    goto lab;
  if (b == 0x0b)
    goto lab;
  if (0x0e <= b && b <= 0x1a)
    goto lab;
  if (0x1c <= b && b <= 0x1f)
    goto lab;
  return 0;
lab:
  return 1;
}

int
bar (unsigned char b)
{
  if (0x0 <= b && b <= 0x8)
    goto lab;
  if (b == 0x0b)
    goto lab;
  if (0x0e <= b && b <= 0x1a)
    goto lab;
  if (0x3c <= b && b <= 0x3f)
    goto lab;
  return 0;
lab:
  return 1;
}

char tab1[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1 };
char tab2[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };

int
main (void)
{
  int i;
  for (i = 0; i < 256; i++)
    if (foo (i) != (i < 32 ? tab1[i] : 0))
      abort ();
  for (i = 0; i < 256; i++)
    if (bar (i) != (i < 64 ? tab2[i] : 0))
      abort ();
  return 0;
}
