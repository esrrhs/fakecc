// expect: 0
package main;

extern void* memcpy(void*, const void*, unsigned long);
extern void* memset(void*, int, unsigned long);
extern int memcmp(const void*, const void*, unsigned long);
extern void* memmove(void*, const void*, unsigned long);
extern int strcmp(const char*, const char*);
extern int strncmp(const char*, const char*, unsigned long);
extern unsigned long strlen(const char*);
extern char* strcpy(char*, const char*);
extern char* strncpy(char*, const char*, unsigned long);
extern char* strchr(const char*, int);
extern char* strrchr(const char*, int);
extern char* strcat(char*, const char*);
extern char* strncat(char*, const char*, unsigned long);
extern char* strstr(const char*, const char*);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int puts(const char*);
extern int putchar(int);
extern void* malloc(unsigned long);
extern void free(void*);
extern void* calloc(unsigned long, unsigned long);
extern void* realloc(void*, unsigned long);
extern int abs(int);
extern long labs(long);
extern int atoi(const char*);
extern long atol(const char*);
extern double atof(const char*);
extern double sqrt(double);
extern double fabs(double);
extern double pow(double, double);
extern double ceil(double);
extern double floor(double);
extern void exit(int);
extern void abort(void);
extern int rand(void);
extern void srand(unsigned int);

struct U { char c[16]; };
struct V { char c[16]; };
struct S { unsigned int a : 3, b : 8, c : 21; struct U d; unsigned int e; struct V f; unsigned int g : 5, h : 27; };
struct T { unsigned int a : 16, b : 8, c : 8; struct U d; unsigned int e; struct V f; unsigned int g : 5, h : 27; };
 void foo (struct S *p)
{
  p->b = 231;
  p->c = 42;
  p->d = (struct U) { "abcdefghijklmno" };
  p->e = 0xdeadbeef;
  p->f = (struct V) { "ABCDEFGHIJKLMNO" };
}

 void bar (struct S *p)
{
  p->b = 231;
  p->c = 42;
  p->d = (struct U) { "abcdefghijklmno" };
  p->e = 0xdeadbeef;
  p->f = (struct V) { "ABCDEFGHIJKLMNO" };
  p->g = 12;
}

 void baz (struct T *p)
{
  p->c = 42;
  p->d = (struct U) { "abcdefghijklmno" };
  p->e = 0xdeadbeef;
  p->f = (struct V) { "ABCDEFGHIJKLMNO" };
  p->g = 12;
}
int main()
{
  if (8 != 8 || 4 != 4)
    return 0;
  struct S s = {};
  struct T t = {};
  foo (&s);
  if (s.a != 0 || s.b != 231 || s.c != 42
      || memcmp (&s.d.c, "abcdefghijklmno", 16) || s.e != 0xdeadbeef
      || memcmp (&s.f.c, "ABCDEFGHIJKLMNO", 16) || s.g != 0 || s.h != 0)
    abort();
  memset (&s, 0, sizeof (s));
  s.a = 7;
  s.g = 31;
  s.h = (1U << 27) - 1;
  foo (&s);
  if (s.a != 7 || s.b != 231 || s.c != 42
      || memcmp (&s.d.c, "abcdefghijklmno", 16) || s.e != 0xdeadbeef
      || memcmp (&s.f.c, "ABCDEFGHIJKLMNO", 16) || s.g != 31 || s.h != (1U << 27) - 1)
    abort();
  memset (&s, 0, sizeof (s));
  bar (&s);
  if (s.a != 0 || s.b != 231 || s.c != 42
      || memcmp (&s.d.c, "abcdefghijklmno", 16) || s.e != 0xdeadbeef
      || memcmp (&s.f.c, "ABCDEFGHIJKLMNO", 16) || s.g != 12 || s.h != 0)
    abort();
  memset (&s, 0, sizeof (s));
  s.a = 7;
  s.g = 31;
  s.h = (1U << 27) - 1;
  bar (&s);
  if (s.a != 7 || s.b != 231 || s.c != 42
      || memcmp (&s.d.c, "abcdefghijklmno", 16) || s.e != 0xdeadbeef
      || memcmp (&s.f.c, "ABCDEFGHIJKLMNO", 16) || s.g != 12 || s.h != (1U << 27) - 1)
    abort();
  baz (&t);
  if (t.a != 0 || t.b != 0 || t.c != 42
      || memcmp (&t.d.c, "abcdefghijklmno", 16) || t.e != 0xdeadbeef
      || memcmp (&t.f.c, "ABCDEFGHIJKLMNO", 16) || t.g != 12 || t.h != 0)
    abort();
  memset (&s, 0, sizeof (s));
  t.a = 7;
  t.b = 255;
  t.g = 31;
  t.h = (1U << 27) - 1;
  baz (&t);
  if (t.a != 7 || t.b != 255 || t.c != 42
      || memcmp (&t.d.c, "abcdefghijklmno", 16) || t.e != 0xdeadbeef
      || memcmp (&t.f.c, "ABCDEFGHIJKLMNO", 16) || t.g != 12 || t.h != (1U << 27) - 1)
    abort();
  return 0;
}

