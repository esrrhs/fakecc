// expect: 0
package main;

/* PR middle-end/21850 */

extern void abort (void);

typedef int V2SI __attribute__ ((vector_size (8)));

int
main (void)
{
  if (((int)(long long)(V2SI){ 2, 2 }) != 2)
    abort ();
  return 0;
}
