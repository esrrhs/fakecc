// expect: 0
package main;

extern void abort(void);
extern int strcmp(const char *, const char *);
extern char *tmpnam(char *);
extern void *freopen(const char *, const char *, void *);
extern void *fopen(const char *, const char *);
extern int fclose(void *);
extern int remove(const char *);
extern int fscanf(void *, const char *, ...);
extern void perror(const char *);
extern void *stdout;
extern int vfprintf(void *, const char *, __builtin_va_list);

void
user_print (const char *fmt, ...)
{
  __builtin_va_list va;
  __builtin_va_start (va, fmt);
  vfprintf (stdout, fmt, va);
  __builtin_va_end (va);
}

int main (void)
{
  char tmpfname[64] = "/tmp/user_printf_test.txt";
  void *f = freopen (tmpfname, "w", stdout);
  if (!f)
    {
      return 1;
    }

  user_print ("1");
  user_print ("%c", '2');
  user_print ("%c%c", '3', '4');
  user_print ("%s", "5");
  user_print ("%s%s", "6", "7");
  user_print ("%i", 8);
  user_print ("%.1s\n", "9x");

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
