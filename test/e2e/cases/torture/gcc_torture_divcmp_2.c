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
int test1(int x)
{
  return x/10 == 2;
}
int test2(int x)
{
  return x/10 == 0;
}
int test3(int x)
{
  return x/10 == -2;
}
int test4(int x)
{
  return x/-10 == 2;
}
int test5(int x)
{
  return x/-10 == 0;
}
int test6(int x)
{
  return x/-10 == -2;
}
int main()
{
  if (test1(19) != 0)
    abort ();
  if (test1(20) != 1)
    abort ();
  if (test1(29) != 1)
    abort ();
  if (test1(30) != 0)
    abort ();
  if (test2(-10) != 0)
    abort ();
  if (test2(-9) != 1)
    abort ();
  if (test2(9) != 1)
    abort ();
  if (test2(10) != 0)
    abort ();
  if (test3(-30) != 0)
    abort ();
  if (test3(-29) != 1)
    abort ();
  if (test3(-20) != 1)
    abort ();
  if (test3(-19) != 0)
    abort ();
  if (test4(-30) != 0)
    abort ();
  if (test4(-29) != 1)
    abort ();
  if (test4(-20) != 1)
    abort ();
  if (test4(-19) != 0)
    abort ();
  if (test5(-10) != 0)
    abort ();
  if (test5(-9) != 1)
    abort ();
  if (test5(9) != 1)
    abort ();
  if (test5(10) != 0)
    abort ();
  if (test6(19) != 0)
    abort ();
  if (test6(20) != 1)
    abort ();
  if (test6(29) != 1)
    abort ();
  if (test6(30) != 0)
    abort ();
  return 0;
}
