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

extern void abort (void);
struct S
{
  int n1, n2, n3, n4;
};
__attribute__((noinline)) struct S
foo (int x, int y, int z)
{
  if (x != 10 || y != 9 || z != 8)
    abort ();
  struct S s = { 1, 2, 3, 4 };
  return s;
}
__attribute__((noinline)) void **
bar (void **u, int *v)
{
  void **w = u;
  int *s = v, x, y, z;
  void **p, **q;
  static void *l[] = { &&lab1, &&lab1, &&lab2, &&lab3, &&lab4 };
  if (!u)
    return l;
  q = *w++;
  goto *q;
lab2:
  p = q;
  q = *w++;
  x = s[2];
  y = s[1];
  z = s[0];
  s -= 1;
  struct S r = foo (x, y, z);
  s[3] = r.n1;
  s[2] = r.n2;
  s[1] = r.n3;
  s[0] = r.n4;
  goto *q;
lab3:
  p = q;
  q = *w++;
  s += 1;
  s[0] = 23;
lab1:
  goto *q;
lab4:
  return 0;
}
int
main (void)
{
  void **u = bar ((void **) 0, (int *) 0);
  void *t[] = { u[2], u[4] };
  int s[] = { 7, 8, 9, 10, 11, 12 };
  if (bar (t, &s[1]) != (void **) 0
      || s[0] != 4 || s[1] != 3 || s[2] != 2 || s[3] != 1
      || s[4] != 11 || s[5] != 12)
    abort ();
  return 0;
}
