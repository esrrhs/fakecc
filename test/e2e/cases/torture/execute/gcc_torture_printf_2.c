// expect: 0
package main;

extern void abort(void);
extern int strcmp(const char *, const char *);
extern void *freopen(const char *, const char *, void *);
extern void *fopen(const char *, const char *);
extern int fclose(void *);
extern int remove(const char *);
extern int fscanf(void *, const char *, ...);
extern int printf(const char *, ...);
extern void perror(const char *);
extern void *stdout;

void
write_file (void)
{
  printf ("1");
  printf ("%c", '2');
  printf ("%c%c", '3', '4');
  printf ("%s", "5");
  printf ("%s%s", "6", "7");
  printf ("%i", 8);
  printf ("%.1s\n", "9x");
}

int main (void)
{
  char tmpfname[64] = "/tmp/printf_2_test.txt";
  void *f = freopen (tmpfname, "w", stdout);
  if (!f)
    {
      return 1;
    }

  write_file ();
  fclose (f);

  f = fopen (tmpfname, "r");
  if (!f)
    {
      remove (tmpfname);
      return 1;
    }

  char buf[12] = "";
  if (1 != fscanf (f, "%s", buf))
    {
      fclose (f);
      remove (tmpfname);
      return 1;
    }

  fclose (f);
  remove (tmpfname);

  if (strcmp (buf, "123456789"))
    abort ();

  return 0;
}
