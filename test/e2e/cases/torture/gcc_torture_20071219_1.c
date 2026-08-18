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
extern void *memset (void *s, int c, long unsigned int n);
struct S
{
  char s[25];
};
struct S *p;
void  foo (struct S *x, int set)
{
  int i;
  for (i = 0; i < sizeof (x->s); ++i)
    if (x->s[i] != 0)
      abort ();
    else if (set)
      x->s[i] = set;
  p = x;
}
void  test1 (void)
{
  struct S a;
  memset (&a.s, '\0', sizeof (a.s));
  foo (&a, 0);
  struct S b = a;
  foo (&b, 1);
  b = a;
  b = b;
  foo (&b, 0);
}
void  test2 (void)
{
  struct S a;
  memset (&a.s, '\0', sizeof (a.s));
  foo (&a, 0);
  struct S b = a;
  foo (&b, 1);
  b = a;
  b = *p;
  foo (&b, 0);
}
void  test3 (void)
{
  struct S a;
  memset (&a.s, '\0', sizeof (a.s));
  foo (&a, 0);
  struct S b = a;
  foo (&b, 1);
  *p = a;
  *p = b;
  foo (&b, 0);
}
int main(void)
{
  test1 ();
  test2 ();
  test3 ();
  return 0;
}

