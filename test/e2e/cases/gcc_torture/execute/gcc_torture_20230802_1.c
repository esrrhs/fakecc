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

long long foo(unsigned int x)
{
  int y = x;
  y = ~y;
  return ((long long) x) & y;
}

long long foo_v(volatile unsigned int x)
{
  volatile int y = x;
  y = ~y;
  return ((long long) x) & y;
}

long long bar(unsigned int x)
{
  int y = x;
  y = ~y;
  return ((long long) x) ^ y;
}

long long bar_v(volatile unsigned int x)
{
  volatile int y = x;
  y = ~y;
  return ((long long) x) ^ y;
}

long long baz(unsigned int x)
{
  int y = x;
  y = ~y;
  return y ^ ((long long) x);
}

long long baz_v(volatile unsigned int x)
{
  volatile int y = x;
  y = ~y;
  return y ^ ((long long) x);
}
int main()
{
  for(int t = -1; t <= 1; t++)
    {
      if (foo(t) != foo_v(t))
        abort();
      if (bar(t) != bar_v(t))
        abort();
      if (baz(t) != baz_v(t))
        abort();
    }
}

