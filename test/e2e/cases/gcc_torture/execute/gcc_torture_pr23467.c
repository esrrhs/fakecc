// expect: 0
package main;

/* { dg-skip-if "small alignment" { pdp11-*-* } } */

extern void abort (void);
extern void exit (int);

struct s1
{
  int __attribute__ ((aligned (8))) a;
};

struct
{
  char c;
  struct s1 m;
} v;

int
main (void)
{
  if ((int)(long)&v.m & 7)
    abort ();
  exit (0);
}
