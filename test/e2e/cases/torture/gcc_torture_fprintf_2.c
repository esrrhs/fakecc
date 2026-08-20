// expect: 0
package main;

extern void* fopen(const char*, const char*);
extern int fclose(void*);
extern int fprintf(void*, const char*, ...);
extern int fscanf(void*, const char*, ...);
extern char* tmpnam(char*);
extern int remove(const char*);
extern void perror(const char*);
extern int strcmp(const char*, const char*);
extern void abort(void);

typedef void FILE;

int main (void)
{
  char *tmpfname = tmpnam (0);
  FILE *f = fopen (tmpfname, "w");
  if (!f)
    {
      perror ("fopen for writing");
      return 1;
    }

  fprintf (f, "1");
  fprintf (f, "%c", '2');
  fprintf (f, "%c%c", '3', '4');
  fprintf (f, "%s", "5");
  fprintf (f, "%s%s", "6", "7");
  fprintf (f, "%i", 8);
  fprintf (f, "%.1s\n", "9x");
  fclose (f);

  f = fopen (tmpfname, "r");
  if (!f)
    {
      perror ("fopen for reading");
      remove (tmpfname);
      return 1;
    }

  char buf[12] = "";
  if (1 != fscanf (f, "%s", buf))
    {
      perror ("fscanf");
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
