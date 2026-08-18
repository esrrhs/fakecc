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

void exit (int);
struct S {
  short a;
  short b;
  char c[8];
} s;
char arr[100];
char *ptr = arr;
int idx = 3;
void
arg_ptr (char *p)
{
  __builtin_prefetch (p, 0, 0);
}
void
arg_idx (char *p, int i)
{
  __builtin_prefetch (&p[i], 0, 0);
}
void
glob_ptr (void)
{
  __builtin_prefetch (ptr, 0, 0);
}
void
glob_idx (void)
{
  __builtin_prefetch (&ptr[idx], 0, 0);
}
int
main ()
{
  __builtin_prefetch (&s.b, 0, 0);
  __builtin_prefetch (&s.c[1], 0, 0);
  arg_ptr (&s.c[1]);
  arg_ptr (ptr+3);
  arg_idx (ptr, 3);
  arg_idx (ptr+1, 2);
  idx = 3;
  glob_ptr ();
  glob_idx ();
  ptr++;
  idx = 2;
  glob_ptr ();
  glob_idx ();
  exit (0);
}
