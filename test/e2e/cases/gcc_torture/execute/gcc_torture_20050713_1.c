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
  int a, b, c;
};
int
foo2 (struct S x, struct S y)
{
  if (x.a != 3 || x.b != 4 || x.c != 5)
    abort ();
  if (y.a != 6 || y.b != 7 || y.c != 8)
    abort ();
  return 0;
}
int
foo3 (struct S x, struct S y, struct S z)
{
  foo2 (x, y);
  if (z.a != 9 || z.b != 10 || z.c != 11)
    abort ();
  return 0;
}
int
bar2 (struct S x, struct S y)
{
  return foo2 (y, x);
}
int
bar3 (struct S x, struct S y, struct S z)
{
  return foo3 (y, x, z);
}
int
baz3 (struct S x, struct S y, struct S z)
{
  return foo3 (y, z, x);
}
int
main (void)
{
  struct S a = { 3, 4, 5 }, b = { 6, 7, 8 }, c = { 9, 10, 11 };
  bar2 (b, a);
  bar3 (b, a, c);
  baz3 (c, a, b);
  return 0;
}
