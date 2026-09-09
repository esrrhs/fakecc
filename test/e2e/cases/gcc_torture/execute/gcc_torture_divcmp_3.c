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
int test1(char x)
{
  return x/100 == 3;
}
int test1u(unsigned char x)
{
  return x/100 == 3;
}
int test2(char x)
{
  return x/100 != 3;
}
int test2u(unsigned char x)
{
  return x/100 != 3;
}
int test3(char x)
{
  return x/100 < 3;
}
int test3u(unsigned char x)
{
  return x/100 < 3;
}
int test4(char x)
{
  return x/100 <= 3;
}
int test4u(unsigned char x)
{
  return x/100 <= 3;
}
int test5(char x)
{
  return x/100 > 3;
}
int test5u(unsigned char x)
{
  return x/100 > 3;
}
int test6(char x)
{
  return x/100 >= 3;
}
int test6u(unsigned char x)
{
  return x/100 >= 3;
}
int main()
{
  int c;
  for (c=-128; c<256; c++)
  {
    if (test1(c) != 0)
      abort ();
    if (test1u(c) != 0)
      abort ();
    if (test2(c) != 1)
      abort ();
    if (test2u(c) != 1)
      abort ();
    if (test3(c) != 1)
      abort ();
    if (test3u(c) != 1)
      abort ();
    if (test4(c) != 1)
      abort ();
    if (test4u(c) != 1)
      abort ();
    if (test5(c) != 0)
      abort ();
    if (test5u(c) != 0)
      abort ();
    if (test6(c) != 0)
      abort ();
    if (test6u(c) != 0)
      abort ();
  }
  return 0;
}
