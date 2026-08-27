// expect: 0
package main;

/* Port of gcc.c-torture/execute/builtins/fputs.c (+ fputs-lib.c + lib/main.c).
 * Original abort checks and stdout writes are kept. */

int inside_main = 0;

extern int vprintf (const char *, __builtin_va_list);
typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
extern void __fakecc_va_copy(void *dst, void *src);
typedef struct _IO_FILE FILE;
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
extern void abort (void);
extern size_t strlen(const char *);

int i;

__attribute__ ((__noinline__))
int
fputs(const char *string, FILE *stream)
{
  size_t n = strlen(string);
  size_t r;
  r = fwrite (string, 1, n, stream);
  return n > r ? -1 : 0;
}

int
fputs_unlocked(const char *string, FILE *stream)
{
  return fputs (string, stream);
}

void
main_test(void)
{
  FILE *s_array[] = {stdout, ((void*)0)}, **s_ptr = s_array;
  const char *const s1 = "hello world";

  fputs ("", *s_ptr);
  fputs ("\n", *s_ptr);
  fputs ("bye", *s_ptr);
  fputs (s1, *s_ptr);
  fputs (s1+5, *s_ptr);
  fputs (s1+10, *s_ptr);
  fputs (s1+11, *s_ptr);

  fputs ("", *s_ptr++);
  if (s_ptr != s_array+1 || *s_ptr != 0)
    abort();

  s_ptr = s_array;
  fputs ("\n", *s_ptr++);
  if (s_ptr != s_array+1 || *s_ptr != 0)
    abort();

  s_ptr = s_array;
  fputs ("hello\n", *s_ptr++);
  if (s_ptr != s_array+1 || *s_ptr != 0)
    abort();

  s_ptr = s_array;
  __builtin_fputs ("", *s_ptr);
  __builtin_fputc ('\n', *s_ptr);
  __builtin_fwrite ("hello\n", 1, 6, *s_ptr);
  fputs_unlocked ("", *s_ptr);
  __builtin_fputs_unlocked ("", *s_ptr);

  s_ptr = s_array;
  fputs (i++ ? "f" : "x", *s_ptr++);
  if (s_ptr != s_array+1 || *s_ptr != 0 || i != 1)
    abort();
  fputs (--i ? "\n" : "\n", *--s_ptr);
  if (s_ptr != s_array || i != 0)
    abort();
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
