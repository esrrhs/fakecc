package main;

extern void abort(void);

typedef long __attribute__((vector_size (16 * sizeof (long)))) v16di;

int main (void)
{
  v16di v = {};
  asm goto ("" : : : : L1);
L2:
  asm goto ("" : : : : L1);
L0:
  asm goto ("" : : : : L2);
  v = (v16di){ -1 };
  asm goto ("" : : : : L0);
L1:
  asm goto ("" : : : : L0);
  if (v[3])
    abort ();
  return 0;
}
