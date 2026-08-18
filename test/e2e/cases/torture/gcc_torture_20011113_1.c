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

typedef long unsigned int size_t;
extern void *memcpy (void *, const void *, size_t);
extern void abort (void);
extern void exit (int);
typedef struct t
{
  unsigned a : 16;
  unsigned b : 8;
  unsigned c : 8;
  long d[4];
} *T;
typedef struct {
  long r[3];
} U;
T bar (U, unsigned int);
T foo(T x)
{
  U d, u;
  memcpy (&u, &x->d[1], sizeof u);
  d = u;
  return bar (d, x->b);
}
T baz(T x)
{
  U d, u;
  d.r[0] = 0x123456789;
  d.r[1] = 0xfedcba987;
  d.r[2] = 0xabcdef123;
  memcpy (&u, &x->d[1], sizeof u);
  d = u;
  return bar (d, x->b);
}
T bar(U d, unsigned int m)
{
  if (d.r[0] != 21 || d.r[1] != 22 || d.r[2] != 23)
    abort ();
  return 0;
}
struct t t = { 26, 0, 0, { 0, 21, 22, 23 }};
int main(void)
{
  baz (&t);
  foo (&t);
  exit (0);
}

