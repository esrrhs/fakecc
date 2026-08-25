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

extern void abort(void);
extern void exit(int);
struct baz {
  int a, b, c, d, e;
};
void bar(struct baz *x, int f, int g, int h, int i, int j)
{
  if (x->a != 1 || x->b != 2 || x->c != 3 || x->d != 4 || x->e != 5 ||
      f != 6 || g != 7 || h != 8 || i != 9 || j != 10)
    abort();
}
void foo(struct baz x, char **y)
{
  bar(&x,6,7,8,9,10);
}
int main()
{
  struct baz x;
  x.a = 1;
  x.b = 2;
  x.c = 3;
  x.d = 4;
  x.e = 5;
  foo(x,(char **)0);
  exit(0);
}
