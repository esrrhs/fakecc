// expect: 0
package main;

extern void abort (void);
extern void exit (int);

struct S {
  short int i : 12;
  char c1 : 1;
  char c2 : 1;
  char c3 : 1;
  char c4 : 1;
} __attribute__((scalar_storage_order("big-endian")));

int main (void)
{
  struct S s0 = { 341, 1, 1, 1, 1 };
  char *p = (char *) &s0;

  if (*p != 21)
    abort ();

  return 0;
}
