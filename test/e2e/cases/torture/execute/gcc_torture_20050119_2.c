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

typedef enum { A, B, C, D } E;
struct S {
  E  a;
  E  b;
  E  c;
  E  d;
};
extern void abort (void);
extern void exit (int);
E
foo (struct S *s)
{
  if (s->a != s->b)
    abort ();
  if (s->c != C)
    abort ();
  return s->d;
}
int main(void)
{
  struct S s[2];
  s[0].a = B;
  s[0].b = B;
  s[0].c = C;
  s[0].d = D;
  s[1].a = D;
  s[1].b = C;
  s[1].c = B;
  s[1].d = A;
  if (foo (s) != D)
    abort ();
  exit (0);
}

