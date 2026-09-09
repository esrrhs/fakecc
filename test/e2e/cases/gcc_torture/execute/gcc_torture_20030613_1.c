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
struct CS {
  long x;
  long y;
};
static struct CS CCID(struct CS x)
{
  struct CS a;
  a.x = x.x;
  a.y = x.y;
  return a;
}
static struct CS CPOW(struct CS x, int y)
{
  struct CS a;
  a = x;
  while (--y > 0)
    a=CCID(a);
  return a;
}
static int c5p(struct CS x)
{
  struct CS a,b;
  a = CPOW (x, 2);
  b = CCID( CPOW(a,2) );
  return (b.x == b.y);
}
int main(void)
{
  struct CS x;
  x.x = -7;
  x.y = -7;
  if (!c5p(x))
    abort();
  return 0;
}

