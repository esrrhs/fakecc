// expect: 0
package main;

extern void abort (void);
extern int strcmp (const char *, const char *);

char *sf (char *s, char *s0)
{
  while (*--s == '9')
    if (s == s0)
      {
	*s = '0';
	break;
      }
  ++*s++;
  return s;
}

int main (void)
{
  char s[] = "999999";
  char *x = sf (s + 2, s);
  if (x != s + 1 || strcmp (s, "199999") != 0)
    abort ();
  return 0;
}
