// expect: 0
package main;

extern void abort (void);
extern void *stdout;
extern int vfprintf (void *, const char *, va_list);

void
inner (int x, ...)
{
  va_list ap, ap2;
  va_start (ap, x);
  va_start (ap2, x);

  switch (x)
    {
    case 0:
      vfprintf (stdout, "hello", ap);
      if (vfprintf (stdout, "hello", ap2) != 5)
        abort ();
      break;
    case 1:
      vfprintf (stdout, "hello\n", ap);
      if (vfprintf (stdout, "hello\n", ap2) != 6)
        abort ();
      break;
    case 2:
      vfprintf (stdout, "a", ap);
      if (vfprintf (stdout, "a", ap2) != 1)
        abort ();
      break;
    case 3:
      vfprintf (stdout, "", ap);
      if (vfprintf (stdout, "", ap2) != 0)
        abort ();
      break;
    case 4:
      vfprintf (stdout, "%s", ap);
      if (vfprintf (stdout, "%s", ap2) != 5)
        abort ();
      break;
    case 5:
      vfprintf (stdout, "%s", ap);
      if (vfprintf (stdout, "%s", ap2) != 6)
        abort ();
      break;
    case 6:
      vfprintf (stdout, "%s", ap);
      if (vfprintf (stdout, "%s", ap2) != 1)
        abort ();
      break;
    case 7:
      vfprintf (stdout, "%s", ap);
      if (vfprintf (stdout, "%s", ap2) != 0)
        abort ();
      break;
    case 8:
      vfprintf (stdout, "%c", ap);
      if (vfprintf (stdout, "%c", ap2) != 1)
        abort ();
      break;
    case 9:
      vfprintf (stdout, "%s\n", ap);
      if (vfprintf (stdout, "%s\n", ap2) != 7)
        abort ();
      break;
    case 10:
      vfprintf (stdout, "%d\n", ap);
      if (vfprintf (stdout, "%d\n", ap2) != 2)
        abort ();
      break;
    default:
      abort ();
    }

  va_end (ap);
  va_end (ap2);
}

int
main (void)
{
  inner (0);
  inner (1);
  inner (2);
  inner (3);
  inner (4, "hello");
  inner (5, "hello\n");
  inner (6, "a");
  inner (7, "");
  inner (8, 'x');
  inner (9, "hello\n");
  inner (10, 0);
  return 0;
}
