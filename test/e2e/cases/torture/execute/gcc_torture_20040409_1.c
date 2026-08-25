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

extern void abort ();
int test1(int x)
{
  return x ^ (-2147483647 - 1);
}
unsigned int test1u(unsigned int x)
{
  return x ^ (unsigned int)(-2147483647 - 1);
}
unsigned int test2u(unsigned int x)
{
  return x + (unsigned int)(-2147483647 - 1);
}
unsigned int test3u(unsigned int x)
{
  return x - (unsigned int)(-2147483647 - 1);
}
int test4(int x)
{
  int y = (-2147483647 - 1);
  return x ^ y;
}
unsigned int test4u(unsigned int x)
{
  unsigned int y = (unsigned int)(-2147483647 - 1);
  return x ^ y;
}
unsigned int test5u(unsigned int x)
{
  unsigned int y = (unsigned int)(-2147483647 - 1);
  return x + y;
}
unsigned int test6u(unsigned int x)
{
  unsigned int y = (unsigned int)(-2147483647 - 1);
  return x - y;
}
void test(int a, int b)
{
  if (test1(a) != b)
    abort();
  if (test4(a) != b)
    abort();
}
void testu(unsigned int a, unsigned int b)
{
  if (test1u(a) != b)
    abort();
  if (test2u(a) != b)
    abort();
  if (test3u(a) != b)
    abort();
  if (test4u(a) != b)
    abort();
  if (test5u(a) != b)
    abort();
  if (test6u(a) != b)
    abort();
}
int main()
{
  test(0x00000000,0x80000000);
  test(0x80000000,0x00000000);
  test(0x12345678,0x92345678);
  test(0x92345678,0x12345678);
  test(0x7fffffff,0xffffffff);
  test(0xffffffff,0x7fffffff);
  testu(0x00000000,0x80000000);
  testu(0x80000000,0x00000000);
  testu(0x12345678,0x92345678);
  testu(0x92345678,0x12345678);
  testu(0x7fffffff,0xffffffff);
  testu(0xffffffff,0x7fffffff);
  return 0;
}

