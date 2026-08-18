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
enum locality { none, low, moderate, high };
enum rws { read, write, read_shared };
int arr[10];
void
good_const (const int *p)
{
  __builtin_prefetch (p, 0, 0);
  __builtin_prefetch (p, 0, 1);
  __builtin_prefetch (p, 0, 2);
  __builtin_prefetch (p, 0, 3);
  __builtin_prefetch (p, 1, 0);
  __builtin_prefetch (p, 1, 1);
  __builtin_prefetch (p, 1, 1);
  __builtin_prefetch (p, 1, 3);
}
void
good_enum (const int *p)
{
    __builtin_prefetch (p, read, none);
    __builtin_prefetch (p, read, low);
    __builtin_prefetch (p, read, moderate);
    __builtin_prefetch (p, read, high);
    __builtin_prefetch (p, write, none);
    __builtin_prefetch (p, write, low);
    __builtin_prefetch (p, write, moderate);
    __builtin_prefetch (p, write, high);
}
void
good_expr (const int *p)
{
  __builtin_prefetch (p, 1 - 1, 6 - (2 * 3));
  __builtin_prefetch (p, 1 + 0, 1 + 2);
}
void
good_vararg (const int *p)
{
  __builtin_prefetch (p, 0, 3);
  __builtin_prefetch (p, 0);
  __builtin_prefetch (p, 1);
  __builtin_prefetch (p);
}
int
main ()
{
  good_const (arr);
  good_enum (arr);
  good_expr (arr);
  good_vararg (arr);
  exit (0);
}
