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
  return -(x >> ((sizeof(int)*8)-1));
}
int test2(unsigned int x)
{
  return -((int)(x >> ((sizeof(int)*8)-1)));
}
int test3(int x)
{
  int y;
  y = (sizeof(int)*8)-1;
  return -(x >> y);
}
int test4(unsigned int x)
{
  int y;
  y = (sizeof(int)*8)-1;
  return -((int)(x >> y));
}
int main()
{
  if (test1(0) != 0)
    abort ();
  if (test1(1) != 0)
    abort ();
  if (test1(-1) != 1)
    abort ();
  if (test2(0) != 0)
    abort ();
  if (test2(1) != 0)
    abort ();
  if (test2((unsigned int)-1) != -1)
    abort ();
  if (test3(0) != 0)
    abort ();
  if (test3(1) != 0)
    abort ();
  if (test3(-1) != 1)
    abort ();
  if (test4(0) != 0)
    abort ();
  if (test4(1) != 0)
    abort ();
  if (test4((unsigned int)-1) != -1)
    abort ();
  return 0;
}

