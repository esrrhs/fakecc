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

unsigned int
foo (unsigned long long x)
{
  unsigned int u;
  if (x == 0)
    return 0;
  u = (unsigned int) (x >> 32);
  return u;
}
unsigned long long
bar (unsigned short x)
{
  return (unsigned long long) x << 32;
}
extern void abort (void);
int
main (void)
{
  if (sizeof (long long) != 8)
    return 0;
  if (foo (0) != 0)
    abort ();
  if (foo (0xffffffffULL) != 0)
    abort ();
  if (foo (0x25ff00ff00ULL) != 0x25)
    abort ();
  if (bar (0) != 0)
    abort ();
  if (bar (0x25) != 0x2500000000ULL)
    abort ();
  return 0;
}
