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
int test3(int x)
{
  return (x + (-2147483647 - 1)) ^ 0x1234;
}
int test4(int x)
{
  return (x ^ 0x1234) + (-2147483647 - 1);
}
int test5(int x)
{
  return (x - (-2147483647 - 1)) ^ 0x1234;
}
int test6(int x)
{
  return (x ^ 0x1234) - (-2147483647 - 1);
}
int test9(int x)
{
  int y = (-2147483647 - 1);
  int z = 0x1234;
  return (x + y) ^ z;
}
int test10(int x)
{
  int y = 0x1234;
  int z = (-2147483647 - 1);
  return (x ^ y) + z;
}
int test11(int x)
{
  int y = (-2147483647 - 1);
  int z = 0x1234;
  return (x - y) ^ z;
}
int test12(int x)
{
  int y = 0x1234;
  int z = (-2147483647 - 1);
  return (x ^ y) - z;
}
void test(int a, int b)
{
  if (test3(a) != b)
    abort();
  if (test4(a) != b)
    abort();
  if (test5(a) != b)
    abort();
  if (test6(a) != b)
    abort();
  if (test9(a) != b)
    abort();
  if (test10(a) != b)
    abort();
  if (test11(a) != b)
    abort();
  if (test12(a) != b)
    abort();
}
int main()
{
  test(0x00000000,0x80001234);
  test(0x00001234,0x80000000);
  test(0x80000000,0x00001234);
  test(0x80001234,0x00000000);
  test(0x7fffffff,0xffffedcb);
  test(0xffffffff,0x7fffedcb);
  return 0;
}

