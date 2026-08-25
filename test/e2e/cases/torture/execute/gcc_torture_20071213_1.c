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
void h(int x, va_list ap)
{
  switch (x)
    {
    case 1:
      if (va_arg(ap, int) != 3 || va_arg(ap, int) != 4)
 abort ();
      return;
    case 5:
      if (va_arg(ap, int) != 9 || va_arg(ap, int) != 10)
 abort ();
      return;
    default:
      abort ();
    }
}
void f1(int i, long long int j, ...)
{
  va_list ap;
  va_start(ap, j);
  h (i, ap);
  if (i != 1 || j != 2)
    abort ();
  va_end(ap);
}
void f2(int i, int j, int k, long long int l, ...)
{
  va_list ap;
  va_start(ap, l);
  h (i, ap);
  if (i != 5 || j != 6 || k != 7 || l != 8)
    abort ();
  va_end(ap);
}
int main()
{
  f1 (1, 2, 3, 4);
  f2 (5, 6, 7, 8, 9, 10);
  return 0;
}

