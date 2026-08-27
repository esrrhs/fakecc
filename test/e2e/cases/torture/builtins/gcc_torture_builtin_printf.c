// expect: 0
package main;

int inside_main = 0;

extern int vprintf (const char *, __builtin_va_list);
typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
extern void __fakecc_va_copy(void *dst, void *src);
typedef struct FILE FILE;
extern FILE *stderr;
extern FILE *stdin;
extern FILE *stdout;
extern int fprintf(FILE *f, const char *fmt, ...);
extern int vfprintf(FILE *f, const char *fmt, va_list ap);
extern int printf(const char *fmt, ...);
extern int sprintf(char *buf, const char *fmt, ...);
extern int snprintf(char *buf, size_t n, const char *fmt, ...);
extern int fputs(const char *s, FILE *f);
extern int fputc(int c, FILE *f);
extern int fflush(FILE *f);
extern int puts(const char *s);
extern int putchar(int c);
extern FILE *fopen(const char *p, const char *m);
extern int fclose(FILE *f);
extern size_t fwrite(const void *p, size_t n, size_t m, FILE *f);
extern size_t fread(void *p, size_t n, size_t m, FILE *f);
extern void perror(const char *s);
extern int fileno(FILE *f);
extern int fseek(FILE *f, long off, int whence);
extern long ftell(FILE *f);
typedef long fpos_t;
extern void abort (void);

__attribute__ ((__noinline__))
int
printf (const char *string, ...)
{
  va_list ap;
  int r;
  va_start (ap, string);
  r = vprintf (string, ap);
  va_end (ap);
  return r;
}
__attribute__ ((__noinline__))
int
printf_unlocked (const char *string, ...)
{
  va_list ap;
  int r;
  va_start (ap, string);
  r = vprintf (string, ap);
  va_end (ap);
  return r;
}
extern int printf (const char *, ...);
extern int printf_unlocked (const char *, ...);
extern void abort(void);
void
main_test (void)
{
  const char *const s1 = "hello world";
  const char *const s2[] = { s1, 0 }, *const*s3;
  printf ("%s\n", "hello");
  printf ("%s\n", *s2);
  s3 = s2;
  printf ("%s\n", *s3++);
  if (s3 != s2+1 || *s3 != 0)
    abort();
  printf ("%c", '\n');
  printf ("%c", **s2);
  s3 = s2;
  printf ("%c", **s3++);
  if (s3 != s2+1 || *s3 != 0)
    abort();
  printf ("");
  printf ("%s", "");
  printf ("\n");
  printf ("%s", "\n");
  printf ("hello world\n");
  printf ("%s", "hello world\n");
  __builtin_printf ("%s\n", "hello");
  __builtin_putchar ('\n');
  __builtin_puts ("hello");
  printf_unlocked ("");
  __builtin_printf_unlocked ("");
  printf_unlocked ("%s", "");
  __builtin_printf_unlocked ("%s", "");
}

int main (void)
{
  inside_main = 1;
  main_test ();
  inside_main = 0;
  return 0;
}

void link_error (void)
{
  abort ();
}
