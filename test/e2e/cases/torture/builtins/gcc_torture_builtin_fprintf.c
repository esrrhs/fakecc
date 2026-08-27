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
typedef long fpos_t;
extern void abort (void);

__attribute__ ((__noinline__))
int
fprintf (FILE *fp, const char *string, ...)
{
  va_list ap;
  int r;
  va_start (ap, string);
  r = vfprintf (fp, string, ap);
  va_end (ap);
  return r;
}
__attribute__ ((__noinline__))
int
fprintf_unlocked (FILE *fp, const char *string, ...)
{
  va_list ap;
  int r;
  va_start (ap, string);
  r = vfprintf (fp, string, ap);
  va_end (ap);
  return r;
}
extern int fprintf_unlocked (FILE *, const char *, ...);
extern void abort(void);
void
main_test (void)
{
  FILE *s_array[] = {stdout, ((void*)0)}, **s_ptr = s_array;
  const char *const s1 = "hello world";
  const char *const s2[] = { s1, 0 }, *const*s3;
  fprintf (*s_ptr, "");
  fprintf (*s_ptr, "%s", "");
  fprintf (*s_ptr, "%s", "hello");
  fprintf (*s_ptr, "%s", "\n");
  fprintf (*s_ptr, "%s", *s2);
  s3 = s2;
  fprintf (*s_ptr, "%s", *s3++);
  if (s3 != s2+1 || *s3 != 0)
    abort();
  s3 = s2;
  fprintf (*s_ptr++, "%s", *s3++);
  if (s3 != s2+1 || *s3 != 0 || s_ptr != s_array+1 || *s_ptr != 0)
    abort();
  s_ptr = s_array;
  fprintf (*s_ptr, "%c", '\n');
  fprintf (*s_ptr, "%c", **s2);
  s3 = s2;
  fprintf (*s_ptr, "%c", **s3++);
  if (s3 != s2+1 || *s3 != 0)
    abort();
  s3 = s2;
  fprintf (*s_ptr++, "%c", **s3++);
  if (s3 != s2+1 || *s3 != 0 || s_ptr != s_array+1 || *s_ptr != 0)
    abort();
  s_ptr = s_array;
  fprintf (*s_ptr++, "hello world");
  if (s_ptr != s_array+1 || *s_ptr != 0)
    abort();
  s_ptr = s_array;
  fprintf (*s_ptr, "\n");
  __builtin_fprintf (*s_ptr, "%s", "hello world\n");
  fprintf_unlocked (*s_ptr, "");
  __builtin_fprintf_unlocked (*s_ptr, "");
  fprintf_unlocked (*s_ptr, "%s", "");
  __builtin_fprintf_unlocked (*s_ptr, "%s", "");
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
