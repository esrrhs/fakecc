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
int ieq (int x, int y, int ok)
{
  if ((x<=y) && (x>=y))
    {
      if (!ok) abort ();
    }
  else
    if (ok) abort ();
  if ((x<=y) && (x==y))
    {
      if (!ok) abort ();
    }
  else
    if (ok) abort ();
  if ((x<=y) && (y<=x))
    {
      if (!ok) abort ();
    }
  else
    if (ok) abort ();
  if ((y==x) && (x<=y))
    {
      if (!ok) abort ();
    }
  else
    if (ok) abort ();
}
int ine (int x, int y, int ok)
{
  if ((x<y) || (x>y))
    {
      if (!ok) abort ();
    }
  else
    if (ok) abort ();
}
int ilt (int x, int y, int ok)
{
  if ((x<y) && (x!=y))
    {
      if (!ok) abort ();
    }
  else
    if (ok) abort ();
}
int ile (int x, int y, int ok)
{
  if ((x<y) || (x==y))
    {
      if (!ok) abort ();
    }
  else
    if (ok) abort ();
}
int igt (int x, int y, int ok)
{
  if ((x>y) && (x!=y))
    {
      if (!ok) abort ();
    }
  else
    if (ok) abort ();
}
int ige (int x, int y, int ok)
{
  if ((x>y) || (x==y))
    {
      if (!ok) abort ();
    }
  else
    if (ok) abort ();
}
int
main ()
{
  ieq (1, 4, 0);
  ieq (3, 3, 1);
  ieq (5, 2, 0);
  ine (1, 4, 1);
  ine (3, 3, 0);
  ine (5, 2, 1);
  ilt (1, 4, 1);
  ilt (3, 3, 0);
  ilt (5, 2, 0);
  ile (1, 4, 1);
  ile (3, 3, 1);
  ile (5, 2, 0);
  igt (1, 4, 0);
  igt (3, 3, 0);
  igt (5, 2, 1);
  ige (1, 4, 0);
  ige (3, 3, 1);
  ige (5, 2, 1);
  return 0;
}
