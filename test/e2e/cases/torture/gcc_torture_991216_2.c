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

void abort (void);
void exit (int);
void test(int n, ...)
{
  va_list ap;
  int i;
  va_start(ap, n);
  for (i = 2; i <= n; i++)
    {
      if (va_arg(ap, int) != i)
 abort ();
    }
  
if (va_arg(ap, long long) != 0x123456789abcdefLL)
    abort ();
  if (va_arg(ap, int) != 0x55)
    abort ();
  va_end(ap);
}
int main()
{
  test (1, 0x123456789abcdefLL, 0x55);
  test (2, 2, 0x123456789abcdefLL, 0x55);
  test (3, 2, 3, 0x123456789abcdefLL, 0x55);
  test (4, 2, 3, 4, 0x123456789abcdefLL, 0x55);
  test (5, 2, 3, 4, 5, 0x123456789abcdefLL, 0x55);
  test (6, 2, 3, 4, 5, 6, 0x123456789abcdefLL, 0x55);
  test (7, 2, 3, 4, 5, 6, 7, 0x123456789abcdefLL, 0x55);
  test (8, 2, 3, 4, 5, 6, 7, 8, 0x123456789abcdefLL, 0x55);
  exit (0);
}

