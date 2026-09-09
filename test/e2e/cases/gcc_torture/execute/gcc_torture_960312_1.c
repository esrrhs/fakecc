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
struct S
{
  int *sp, fc, *sc, a[2];
};
void f(struct S *x)
{
  int *t = x->sc;
  int t1 = t[0];
  int t2 = t[1];
  int t3 = t[2];
  int a0 = x->a[0];
  int a1 = x->a[1];
  
  t[2] = t1;
  t[0] = a1;
  x->a[1] = a0;
  x->a[0] = t3;
  x->fc = t2;
  x->sp = t;
}
int main(void)
{
  struct S s;
  static int sc[3] = {2, 3, 4};
  s.sc = sc;
  s.a[0] = 10;
  s.a[1] = 11;
  f (&s);
  if (s.sp[2] != 2)
    abort ();
  exit (0);
}

