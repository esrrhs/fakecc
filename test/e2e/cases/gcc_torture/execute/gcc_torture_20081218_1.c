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

struct A { int i, j; char pad[512]; } a;
int  foo (void)
{
  memset (&a, 0x26, sizeof a);
  return a.i;
}
void  bar (void)
{
  memset (&a, 0x36, sizeof a);
  a.i = 0x36363636;
  a.j = 0x36373636;
}
int main(void)
{
  int i;
  if (sizeof (int) != 4 || 8 != 8)
    return 0;
  if (foo () != 0x26262626)
    abort();
  for (i = 0; i < sizeof a; i++)
    if (((char *)&a)[i] != 0x26)
      abort();
  bar ();
  if (a.j != 0x36373636)
    abort();
  a.j = 0x36363636;
  for (i = 0; i < sizeof a; i++)
    if (((char *)&a)[i] != 0x36)
      abort();
  return 0;
}

