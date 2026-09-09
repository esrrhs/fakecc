// expect: 0
package main;

/* PR middle-end/70903 */

extern void abort (void);

typedef unsigned char V8 __attribute__ ((vector_size (32)));
typedef unsigned int V32 __attribute__ ((vector_size (32)));
typedef unsigned long long V64 __attribute__ ((vector_size (32)));

static V32
foo (V64 x)
{
  V64 y = (V64)(V8){((V8)(V64){65535, x[0]})[1]};
  return (V32){y[0], 255};
}

int
main (void)
{
  V32 x = foo ((V64){});
  if (x[1] != 255)
    abort ();
  return 0;
}
