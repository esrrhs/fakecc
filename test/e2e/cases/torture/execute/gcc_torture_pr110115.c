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


/* PR middle-end/110115 */

int a;
signed char b;

static int
foo(signed char *e, int f) {

  int d;
  for (d = 0; d < f; d++)
    e[d] = 0;
  return d;
}

int
bar(signed char e, int f) {

  signed char h[20];
  int i = foo (h, f);
  return i;
}

int
baz() {

  switch (a)
    {
    case 'f':
      return 0;
    default:
      return ~0;
    }
}

int
main() {

  {
    signed char *k[3];
    int d;
    for (d = 0; bar (8, 15) - 15 + d < 1; d++)
      k[baz () + 1] = &b;
    *k[0] = -*k[0];
  }
  return 0;
}
