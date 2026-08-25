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
extern void f (void);
extern int x (int, char **);
extern int r (const char *);
extern char *s (char *, char **);
extern char *m (char *);
char *u;
char *h;
int check = 0;
int o = 0;
int main(int argc, char **argv)
{
  char *args[] = {"a", "b", "c", "d", "e"};
  if (x (5, args) != 0 || check != 2 || o != 5)
    abort ();
  exit (0);
}
int x(int argc, char **argv)
{
  int opt = 0;
  char *g = 0;
  char *p = 0;
  if (argc > o && argc > 2 && argv[o])
    {
      g = s (argv[o], &p);
      if (g)
 {
   *g++ = '\0';
   h = s (g, &p);
   if (g == p)
     h = m (g);
 }
      u = s (argv[o], &p);
      if (argv[o] == p)
 u = m (argv[o]);
    }
  else
    abort ();
  while (++o < argc)
    if (r (argv[o]) == 0)
      return 1;
  return 0;
}
char * m(char *x) { abort (); }
char * s(char *v, char **pp)
{
  if (strcmp (v, "a") != 0 || check++ > 1)
    abort ();
  *pp = v+1;
  return 0;
}
int r(const char *f)
{
  static char c[2] = "b";
  static int cnt = 0;
  if (*f != *c || f[1] != c[1] || cnt > 3)
    abort ();
  c[0]++;
  cnt++;
  return 1;
}

