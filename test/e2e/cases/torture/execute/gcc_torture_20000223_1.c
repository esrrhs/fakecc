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

void abort (void);
void check (char const *type, int align)
{
  if ((align & -align) != align)
    {
      abort ();
    }
}
struct A
{
  char c;
  signed short ss;
  unsigned short us;
  signed int si;
  unsigned int ui;
  signed long sl;
  unsigned long ul;
  signed long long sll;
  unsigned long long ull;
  float f;
  double d;
  long double ld;
  void *dp;
  void (*fp)();
};
int main ()
{
  check("void", __alignof__(void));
  check("char", __alignof__(char));
  check("signed short", __alignof__(signed short));
  check("unsigned short", __alignof__(unsigned short));
  check("signed int", __alignof__(signed int));
  check("unsigned int", __alignof__(unsigned int));
  check("signed long", __alignof__(signed long));
  check("unsigned long", __alignof__(unsigned long));
  check("signed long long", __alignof__(signed long long));
  check("unsigned long long", __alignof__(unsigned long long));
  check("float", __alignof__(float));
  check("double", __alignof__(double));
  check("long double", __alignof__(long double));
  check("void *", __alignof__(void *));
  check("void (*)()", __alignof__(void (*)()));
  check("struct A", __alignof__(struct A));
  return 0;
}
