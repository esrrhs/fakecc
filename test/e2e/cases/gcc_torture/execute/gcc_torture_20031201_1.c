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
typedef struct { unsigned int e0 : 16; unsigned int e1 : 16; } s1;
typedef struct { unsigned int e0 : 16; unsigned int e1 : 16; } s2;
typedef struct { s1 i12; s2 i16; } io;
static int test_length = 2;
static io *i;
static int m = 1;
static int d = 1;
static unsigned long test_t0;
static unsigned long test_t1;
void test(void) ;
extern int f1 (void *port) ;
extern void f0 (void) ;
int f1(void *port)
{
  int fail_count = 0;
  unsigned long tlen;
  s1 x0 = {0};
  s2 x1 = {0};
  i = port;
  x0.e0 = x1.e0 = 32;
  i->i12 = x0;
  i->i16 = x1;
  do f0(); while (test_t1);
  x0.e0 = x1.e0 = 8;
  i->i12 = x0;
  i->i16 = x1;
  test ();
  if (m)
    {
      unsigned long e = 1000000000 / 460800 * test_length;
      tlen = test_t1 - test_t0;
      if (((tlen-e) & 0x7FFFFFFF) > 1000)
 f0();
    }
  
if (d)
    {
      unsigned long e = 1000000000 / 460800 * test_length;
      tlen = test_t1 - test_t0;
      if (((tlen - e) & 0x7FFFFFFF) > 1000)
 f0();
    }
  return fail_count != 0 ? 1 : 0;
}
int main()
{
  io io0;
  f1 (&io0);
  abort ();
}
void test(void)
{
  io *iop = i;
  if (iop->i12.e0 != 8 || iop->i16.e0 != 8)
    abort ();
  exit (0);
}
void f0(void)
{
  static int washere = 0;
  io *iop = i;
  if (washere++ || iop->i12.e0 != 32 || iop->i16.e0 != 32)
    abort ();
}

