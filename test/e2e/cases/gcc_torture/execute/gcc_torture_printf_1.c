// expect: 0
package main;

extern void abort (void);
extern int printf (const char *, ...);

int
main (void)
{
  printf ("hello");
  if (printf ("hello") != 5) abort ();

  printf ("hello\n");
  if (printf ("hello\n") != 6) abort ();

  printf ("a");
  if (printf ("a") != 1) abort ();

  printf ("");
  if (printf ("") != 0) abort ();

  printf ("%s", "hello");
  if (printf ("%s", "hello") != 5) abort ();

  printf ("%s", "hello\n");
  if (printf ("%s", "hello\n") != 6) abort ();

  printf ("%s", "a");
  if (printf ("%s", "a") != 1) abort ();

  printf ("%s", "");
  if (printf ("%s", "") != 0) abort ();

  printf ("%c", 'x');
  if (printf ("%c", 'x') != 1) abort ();

  printf ("%s\n", "hello\n");
  if (printf ("%s\n", "hello\n") != 7) abort ();

  printf ("%d\n", 0);
  if (printf ("%d\n", 0) != 2) abort ();

  return 0;
}
