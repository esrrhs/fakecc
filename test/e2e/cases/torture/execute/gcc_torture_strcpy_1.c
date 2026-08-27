// expect: 0
package main;

/* Copyright (C) 2002  Free Software Foundation.

   Test strcpy with various combinations of pointer alignments and lengths to
   make sure any optimizations in the library are correct.  */

extern void abort (void);
extern void exit (int);
extern char *strcpy (char *, const char *);

static union {
  char buf[97];
  long long align_int;
  long double align_fp;
} u1, u2;

int
main (void)
{
  int off1, off2, len, i;
  char *p, *q, c;

  for (off1 = 0; off1 < 8; off1++)
    for (off2 = 0; off2 < 8; off2++)
      for (len = 1; len < 80; len++)
	{
	  for (i = 0, c = 'A'; i < 97; i++, c++)
	    {
	      u1.buf[i] = 'a';
	      if (c >= 'A' + 31)
		c = 'A';
	      u2.buf[i] = c;
	    }
	  u2.buf[off2 + len] = '\0';

	  p = strcpy (u1.buf + off1, u2.buf + off2);
	  if (p != u1.buf + off1)
	    abort ();

	  q = u1.buf;
	  for (i = 0; i < off1; i++, q++)
	    if (*q != 'a')
	      abort ();

	  for (i = 0, c = 'A' + off2; i < len; i++, q++, c++)
	    {
	      if (c >= 'A' + 31)
		c = 'A';
	      if (*q != c)
		abort ();
	    }

	  if (*q++ != '\0')
	    abort ();
	  for (i = 0; i < 8; i++, q++)
	    if (*q != 'a')
	      abort ();
	}

  exit (0);
}
