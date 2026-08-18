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
extern void exit (int);
long baz1 (void *a)
{
  static long l;
  return l++;
}
int baz2 (const char *a)
{
  return 0;
}
int baz3 (int i)
{
  if (!i)
    abort ();
  return 1;
}
void **bar;
int foo (void *a, long b, int c)
{
  int d = 0, e, f = 0, i;
  char g[256];
  void **h;
  g[0] = '\n';
  g[1] = 0;
  while (baz1 (a) < b) {
    if (g[0] != ' ' && g[0] != '\t') {
      f = 1;
      e = 0;
      if (!d && baz2 (g) == 0) {
 if ((c & 0x10) == 0)
   continue;
 e = d = 1;
      }
      if (!((c & 0x10) && (c & 0x4000) && e) && (c & 2))
 continue;
      if ((c & 0x2000) && baz2 (g) == 0)
 continue;
      if ((c & 0x1408) && baz2 (g) == 0)
 continue;
      if ((c & 0x200) && baz2 (g) == 0)
 continue;
      if (c & 0x80) {
 for (h = bar, i = 0; h; h = (void **)*h, i++)
   if (baz3 (i))
     break;
      }
      f = 0;
    }
  }
  return 0;
}
int main ()
{
  void *n = 0;
  bar = &n;
  foo (&n, 1, 0xc811);
  exit (0);
}
