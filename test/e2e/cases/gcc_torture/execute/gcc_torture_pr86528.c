// expect: 0
package main;

/* PR middle-end/86528 */

extern void abort (void);
extern void *memcpy (void *, const void *, unsigned long);
extern char *strcpy (char *, const char *);
extern int strcmp (const char *, const char *);
extern unsigned long strlen (const char *);

void
test (char *data, unsigned long len)
{
  static char const appended[] = "/./";
  char *buf = __builtin_alloca (len + sizeof appended);
  memcpy (buf, data, len);
  strcpy (buf + len, &appended[data[len - 1] == '/']);
  if (strcmp (buf, "test1234/./"))
    abort ();
}

int
main (void)
{
  char *arg = "test1234/";
  test (arg, strlen (arg));
  return 0;
}
