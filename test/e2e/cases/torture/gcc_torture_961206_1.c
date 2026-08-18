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
int
sub1 (unsigned long long i)
{
  if (i < 0x80000000)
    return 1;
  else
    return 0;
}
int
sub2 (unsigned long long i)
{
  if (i <= 0x7FFFFFFF)
    return 1;
  else
    return 0;
}
int
sub3 (unsigned long long i)
{
  if (i >= 0x80000000)
    return 0;
  else
    return 1;
}
int
sub4 (unsigned long long i)
{
  if (i > 0x7FFFFFFF)
    return 0;
  else
    return 1;
}
int
main(void)
{
  if (sub1 (0x80000000ULL))
    abort ();
  if (sub2 (0x80000000ULL))
    abort ();
  if (sub3 (0x80000000ULL))
    abort ();
  if (sub4 (0x80000000ULL))
    abort ();
  exit (0);
}
