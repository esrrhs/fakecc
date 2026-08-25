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

void abort (void);
void exit (int);
struct xx
 {
   int a;
   struct xx *b;
   short c;
 };
int f1 (struct xx *);
void f2 (void);
int foo(struct xx *p, int b, int c, int d)
{
  int a;
  for (;;)
    {
      a = f1(p);
      if (a)
 return (0);
      if (b)
 continue;
      p->c = d;
      if (p->a)
 f2 ();
      if (c)
 f2 ();
      d = p->c;
      switch (a)
 {
 case 1:
   if (p->b)
     f2 ();
   if (c)
     f2 ();
 default:
   break;
 }
    }
  return d;
}
int main(void)
{
  struct xx s = {0, &s, 23};
  if (foo (&s, 0, 0, 0) != 0 || s.a != 0 || s.b != &s || s.c != 0)
    abort ();
  exit (0);
}
int f1(struct xx *p)
{
  static int beenhere = 0;
  if (beenhere++ > 1)
    abort ();
  return beenhere > 1;
}
void f2(void)
{
  abort ();
}

