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
extern void *stdin;
extern void *stdout;
extern void *stderr;
extern int open(const char*, int, ...);
extern int close(int);
extern long read(int, void*, unsigned long);
extern int unlink(const char*);
extern char* tmpnam(char*);

/* Test __builtin_bswap64 . */

unsigned long long g(unsigned long long a) __attribute__((noinline));
unsigned long long g(unsigned long long a)
{
  return __builtin_bswap64(a);
}


unsigned long long f(unsigned long long c)
{
  union {
    unsigned long long a;
    unsigned char b[8];
  } a, b;
  a.a = c;
  b.b[0] = a.b[7];
  b.b[1] = a.b[6];
  b.b[2] = a.b[5];
  b.b[3] = a.b[4];
  b.b[4] = a.b[3];
  b.b[5] = a.b[2];
  b.b[6] = a.b[1];
  b.b[7] = a.b[0];
  return b.a;
}

int main(void)
{
  unsigned long long i;
  /* The rest of the testcase assumes 8 byte long long. */
  if (sizeof(i) != sizeof(char)*8)
    return 0;
  if (f(0x12) != g(0x12))
    abort();
  if (f(0x1234) != g(0x1234))
    abort();
  if (f(0x123456) != g(0x123456))
    abort();
  if (f(0x12345678ull) != g(0x12345678ull))
    abort();
  if (f(0x1234567890ull) != g(0x1234567890ull))
    abort();
  if (f(0x123456789012ull) != g(0x123456789012ull))
    abort();
  if (f(0x12345678901234ull) != g(0x12345678901234ull))
    abort();
  if (f(0x1234567890123456ull) != g(0x1234567890123456ull))
    abort();
  return 0;
}
