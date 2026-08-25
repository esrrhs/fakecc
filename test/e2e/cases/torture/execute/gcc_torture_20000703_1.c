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

void abort(void);
void exit(int);
struct baz 
{
  char a[17];
  char b[3];
  unsigned int c;
  unsigned int d;
};

void foo(struct baz *p, unsigned int c, unsigned int d)
{
  __builtin_memcpy (p->b, "abc", 3);
  p->c = c;
  p->d = d;
}

void bar(struct baz *p, unsigned int c, unsigned int d)
{
  ({ void *s = (p);
     __builtin_memset (s, '\0', sizeof (struct baz));
     s; });
  __builtin_memcpy (p->a, "01234567890123456", 17);
  __builtin_memcpy (p->b, "abc", 3);
  p->c = c;
  p->d = d;
}

int main()
{
  struct baz p;
  foo(&p, 71, 18);
  if (p.c != 71 || p.d != 18)
    abort();
  bar(&p, 59, 26);
  if (p.c != 59 || p.d != 26)
    abort();
  exit(0);
}
