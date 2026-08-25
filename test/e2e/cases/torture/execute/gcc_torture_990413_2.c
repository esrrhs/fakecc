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
extern void* memchr(const void*, int, unsigned long);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int fprintf(void*, const char*, ...);
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
extern int isprint(int);
extern void *stdin;
extern void *stdout;
extern void *stderr;
extern int open(const char*, int, ...);
extern int close(int);
extern long read(int, void*, unsigned long);
extern int unlink(const char*);
extern char* tmpnam(char*);

extern void abort (void);
static inline double minus_zero(void)
{
  union { double __d; int __i[2]; } __x;
  __x.__i[0] = 0x0;
  __x.__i[1] = 0x80000000;
  return __x.__d;
}
static inline long double __atan2l(long double __y, long double __x)
{
  register long double __value;
  __asm __volatile__ ("fpatan\n\t"
        : "=t" (__value)
        : "0" (__x), "u" (__y)
        : "st(1)");
  return __value;
}
static inline long double __sqrtl(long double __x)
{
  register long double __result;
  __asm __volatile__ ("fsqrt" : "=t" (__result) : "0" (__x));
  return __result;
}
static inline double asin(double __x)
{
  return __atan2l (__x, __sqrtl (1.0 - __x * __x));
}
int main(void)
{
  double x;
  x = minus_zero();
  x = asin (x);
  if (x != 0.0)
    abort ();
  return 0;
}
