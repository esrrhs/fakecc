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

typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef int wchar_t;
void abort (void);
void exit (int);
 static int dummy(int x)
{
  int y;
  y = (long) (x * 4711.3);
  return y;
}
int getval (void);
int f2(double x)
{
  unsigned short s;
  int a, b, c, d, e, f, g, h, i, j;
  a = getval ();
  b = getval ();
  c = getval ();
  d = getval ();
  e = getval ();
  f = getval ();
  g = getval ();
  h = getval ();
  i = getval ();
  j = getval ();
  s = x;
  return a + b + c + d + e + f + g + h + i + j + s;
}
int x = 1;
int getval(void)
{
  return x++;
}
char buf[10];
void f()
{
  char ar[200000/2];
  int a, b, c, d, e, f, g, h, i, j, k;
  a = getval ();
  b = getval ();
  c = getval ();
  d = getval ();
  e = getval ();
  f = getval ();
  g = getval ();
  h = getval ();
  i = getval ();
  j = getval ();
  k = f2 (17.0);
  sprintf (buf, "%d\n", a + b + c + d + e + f + g + h + i + j + k);
  if (a + b + c + d + e + f + g + h + i + j + k != 227)
    abort ();
}
int main(void)
{
  f ();
  exit (0);
}

