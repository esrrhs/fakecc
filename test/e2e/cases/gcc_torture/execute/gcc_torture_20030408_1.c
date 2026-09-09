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
extern void* memchr(const void*, int, unsigned long);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int fprintf(void*, const char*, ...);
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
extern int isprint(int);

/* PR optimization/8634 */
/* Contributed by Glen Nakamura <glen at imodulo dot com> */

extern void abort (void);

struct foo {
  char a, b, c, d, e, f, g, h, i, j;
};

int test1 ()
{
  const char X[8] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
  char buffer[8];
  __builtin_memcpy (buffer, X, 8);
  if (buffer[0] != 'A' || buffer[1] != 'B'
      || buffer[2] != 'C' || buffer[3] != 'D'
      || buffer[4] != 'E' || buffer[5] != 'F'
      || buffer[6] != 'G' || buffer[7] != 'H')
    abort ();
  return 0;
}

int test2 ()
{
  const char X[10] = { 'A', 'B', 'C', 'D', 'E' };
  char buffer[10];
  __builtin_memcpy (buffer, X, 10);
  if (buffer[0] != 'A' || buffer[1] != 'B'
      || buffer[2] != 'C' || buffer[3] != 'D'
      || buffer[4] != 'E' || buffer[5] != '\0'
      || buffer[6] != '\0' || buffer[7] != '\0'
      || buffer[8] != '\0' || buffer[9] != '\0')
    abort ();
  return 0;
}

int test3 ()
{
  const struct foo X = { a : 'A', c : 'C', e : 'E', g : 'G', i : 'I' };
  char buffer[10];
  __builtin_memcpy (buffer, &X, 10);
  if (buffer[0] != 'A' || buffer[1] != '\0'
      || buffer[2] != 'C' || buffer[3] != '\0'
      || buffer[4] != 'E' || buffer[5] != '\0'
      || buffer[6] != 'G' || buffer[7] != '\0'
      || buffer[8] != 'I' || buffer[9] != '\0')
    abort ();
  return 0;
}

int test4 ()
{
  const struct foo X = { .b = 'B', .d = 'D', .f = 'F', .h = 'H' , .j = 'J' };
  char buffer[10];
  __builtin_memcpy (buffer, &X, 10);
  if (buffer[0] != '\0' || buffer[1] != 'B'
      || buffer[2] != '\0' || buffer[3] != 'D'
      || buffer[4] != '\0' || buffer[5] != 'F'
      || buffer[6] != '\0' || buffer[7] != 'H'
      || buffer[8] != '\0' || buffer[9] != 'J')
    abort ();
  return 0;
}

int main ()
{
  test1 (); test2 (); test3 (); test4 ();
  return 0;
}
