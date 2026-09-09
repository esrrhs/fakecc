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

struct D { int d1; struct D *d2; };
struct C { struct D c1; long c2, c3, c4, c5, c6; };
struct A { struct A *a1; struct C *a2; };
struct B { struct C b1; struct A *b2; };
extern void abort (void);
extern void exit (int);
void
foo (struct B *x, struct B *y)
{
  if (x->b2 == 0)
    {
      struct A *a;
      x->b2 = a = y->b2;
      y->b2 = 0;
      for (; a; a = a->a1)
 a->a2 = &x->b1;
    }
  if (y->b2 != 0)
    abort ();
  if (x->b1.c3 == -1)
    {
      x->b1.c3 = y->b1.c3;
      x->b1.c4 = y->b1.c4;
      y->b1.c3 = -1;
      y->b1.c4 = 0;
    }
  if (y->b1.c3 != -1)
    abort ();
}
struct B x, y;
int main ()
{
  y.b1.c1.d1 = 6;
  y.b1.c3 = 145;
  y.b1.c4 = 2448;
  x.b1.c3 = -1;
  foo (&x, &y);
  exit (0);
}
