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
int test1(int x)
{
  return x/-10 == 2;
}
int test2(int x)
{
  return x/-10 == 0;
}
int test3(int x)
{
  return x/-10 != 2;
}
int test4(int x)
{
  return x/-10 != 0;
}
int test5(int x)
{
  return x/-10 < 2;
}
int test6(int x)
{
  return x/-10 < 0;
}
int test7(int x)
{
  return x/-10 <= 2;
}
int test8(int x)
{
  return x/-10 <= 0;
}
int test9(int x)
{
  return x/-10 > 2;
}
int test10(int x)
{
  return x/-10 > 0;
}
int test11(int x)
{
  return x/-10 >= 2;
}
int test12(int x)
{
  return x/-10 >= 0;
}
int main()
{
  if (test1(-30) != 0)
    abort ();
  if (test1(-29) != 1)
    abort ();
  if (test1(-20) != 1)
    abort ();
  if (test1(-19) != 0)
    abort ();
  if (test2(0) != 1)
    abort ();
  if (test2(9) != 1)
    abort ();
  if (test2(10) != 0)
    abort ();
  if (test2(-1) != 1)
    abort ();
  if (test2(-9) != 1)
    abort ();
  if (test2(-10) != 0)
    abort ();
  if (test3(-30) != 1)
    abort ();
  if (test3(-29) != 0)
    abort ();
  if (test3(-20) != 0)
    abort ();
  if (test3(-19) != 1)
    abort ();
  if (test4(0) != 0)
    abort ();
  if (test4(9) != 0)
    abort ();
  if (test4(10) != 1)
    abort ();
  if (test4(-1) != 0)
    abort ();
  if (test4(-9) != 0)
    abort ();
  if (test4(-10) != 1)
    abort ();
  if (test5(-30) != 0)
    abort ();
  if (test5(-29) != 0)
    abort ();
  if (test5(-20) != 0)
    abort ();
  if (test5(-19) != 1)
    abort ();
  if (test6(0) != 0)
    abort ();
  if (test6(9) != 0)
    abort ();
  if (test6(10) != 1)
    abort ();
  if (test6(-1) != 0)
    abort ();
  if (test6(-9) != 0)
    abort ();
  if (test6(-10) != 0)
    abort ();
  if (test7(-30) != 0)
    abort ();
  if (test7(-29) != 1)
    abort ();
  if (test7(-20) != 1)
    abort ();
  if (test7(-19) != 1)
    abort ();
  if (test8(0) != 1)
    abort ();
  if (test8(9) != 1)
    abort ();
  if (test8(10) != 1)
    abort ();
  if (test8(-1) != 1)
    abort ();
  if (test8(-9) != 1)
    abort ();
  if (test8(-10) != 0)
    abort ();
  if (test9(-30) != 1)
    abort ();
  if (test9(-29) != 0)
    abort ();
  if (test9(-20) != 0)
    abort ();
  if (test9(-19) != 0)
    abort ();
  if (test10(0) != 0)
    abort ();
  if (test10(9) != 0)
    abort ();
  if (test10(10) != 0)
    abort ();
  if (test10(-1) != 0)
    abort ();
  if (test10(-9) != 0)
    abort ();
  if (test10(-10) != 1)
    abort ();
  if (test11(-30) != 1)
    abort ();
  if (test11(-29) != 1)
    abort ();
  if (test11(-20) != 1)
    abort ();
  if (test11(-19) != 0)
    abort ();
  if (test12(0) != 1)
    abort ();
  if (test12(9) != 1)
    abort ();
  if (test12(10) != 0)
    abort ();
  if (test12(-1) != 1)
    abort ();
  if (test12(-9) != 1)
    abort ();
  if (test12(-10) != 1)
    abort ();
  return 0;
}

