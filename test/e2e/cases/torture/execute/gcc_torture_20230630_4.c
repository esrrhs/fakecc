// expect: 0
package main;

extern void abort (void);
extern void exit (int);

struct S {
  int i : 24;
  char c1 : 1;
  char c2 : 1;
  char c3 : 1;
  char c4 : 1;
  char c5 : 1;
  char c6 : 1;
  char c7 : 1;
  char c8 : 1;
} __attribute__((scalar_storage_order("big-endian")));

int main (void)
{
  struct S s0 = { 1193046, 1, 1, 1, 1, 1, 1, 1, 1 };
  char *p = (char *) &s0;

  if (*p != 18)
    abort ();

  return 0;
}
