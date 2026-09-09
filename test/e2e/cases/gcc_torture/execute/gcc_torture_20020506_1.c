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
void test1 (signed char c, int set);
void test2 (unsigned char c, int set);
void test3 (short s, int set);
void test4 (unsigned short s, int set);
void test5 (int i, int set);
void test6 (unsigned int i, int set);
void test7 (long long l, int set);
void test8 (unsigned long long l, int set);
void test1(signed char c, int set)
{
  if ((c & (127 +1)) == 0)
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
  if ((c & (127 +1)) != 0)
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((c & (127 +1)) == (127 +1))
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((c & (127 +1)) != (127 +1))
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
}
void test2(unsigned char c, int set)
{
  if ((c & (127 +1)) == 0)
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
  if ((c & (127 +1)) != 0)
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((c & (127 +1)) == (127 +1))
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((c & (127 +1)) != (127 +1))
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
}
void test3(short s, int set)
{
  if ((s & (32767 +1)) == 0)
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
  if ((s & (32767 +1)) != 0)
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((s & (32767 +1)) == (32767 +1))
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((s & (32767 +1)) != (32767 +1))
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
}
void test4(unsigned short s, int set)
{
  if ((s & (32767 +1)) == 0)
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
  if ((s & (32767 +1)) != 0)
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((s & (32767 +1)) == (32767 +1))
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((s & (32767 +1)) != (32767 +1))
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
}
void test5(int i, int set)
{
  if ((i & (2147483647 +1U)) == 0)
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
  if ((i & (2147483647 +1U)) != 0)
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((i & (2147483647 +1U)) == (2147483647 +1U))
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((i & (2147483647 +1U)) != (2147483647 +1U))
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
}
void test6(unsigned int i, int set)
{
  if ((i & (2147483647 +1U)) == 0)
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
  if ((i & (2147483647 +1U)) != 0)
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((i & (2147483647 +1U)) == (2147483647 +1U))
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((i & (2147483647 +1U)) != (2147483647 +1U))
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
}
void test7(long long l, int set)
{
  if ((l & (0x7fffffffffffffffLL +1ULL)) == 0)
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
  if ((l & (0x7fffffffffffffffLL +1ULL)) != 0)
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((l & (0x7fffffffffffffffLL +1ULL)) == (0x7fffffffffffffffLL +1ULL))
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((l & (0x7fffffffffffffffLL +1ULL)) != (0x7fffffffffffffffLL +1ULL))
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
}
void test8(unsigned long long l, int set)
{
  if ((l & (0x7fffffffffffffffLL +1ULL)) == 0)
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
  if ((l & (0x7fffffffffffffffLL +1ULL)) != 0)
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((l & (0x7fffffffffffffffLL +1ULL)) == (0x7fffffffffffffffLL +1ULL))
    {
      if (!set) abort ();
    }
  else
    if (set) abort ();
  if ((l & (0x7fffffffffffffffLL +1ULL)) != (0x7fffffffffffffffLL +1ULL))
    {
      if (set) abort ();
    }
  else
    if (!set) abort ();
}
int main()
{
  test1 (0, 0);
  test1 (127, 0);
  test1 ((-128), 1);
  test1 (255, 1);
  test2 (0, 0);
  test2 (127, 0);
  test2 ((-128), 1);
  test2 (255, 1);
  test3 (0, 0);
  test3 (32767, 0);
  test3 ((-32768), 1);
  test3 (65535, 1);
  test4 (0, 0);
  test4 (32767, 0);
  test4 ((-32768), 1);
  test4 (65535, 1);
  test5 (0, 0);
  test5 (2147483647, 0);
  test5 ((-2147483647 - 1), 1);
  test5 (4294967295U, 1);
  test6 (0, 0);
  test6 (2147483647, 0);
  test6 ((-2147483647 - 1), 1);
  test6 (4294967295U, 1);
  test7 (0, 0);
  test7 (0x7fffffffffffffffLL, 0);
  test7 ((-0x7fffffffffffffffLL -1), 1);
  test7 ((0x7fffffffffffffffLL * 2ULL + 1), 1);
  test8 (0, 0);
  test8 (0x7fffffffffffffffLL, 0);
  test8 ((-0x7fffffffffffffffLL -1), 1);
  test8 ((0x7fffffffffffffffLL * 2ULL + 1), 1);
  return 0;
}

