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

typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef int wchar_t;
void abort (void);
void exit (int);
int count;
void a1() { ++count; }
void b(unsigned long data)
{
  if (data & 0x80000000) a1();
  data <<= 1;
  if (data & 0x80000000) a1();
  data <<= 1;
  if (data & 0x80000000) a1();
}
int main(void)
{
  count = 0;
  b (0);
  if (count != 0)
    abort ();
  count = 0;
  b (0x80000000);
  if (count != 1)
    abort ();
  count = 0;
  b (0x40000000);
  if (count != 1)
    abort ();
  count = 0;
  b (0x20000000);
  if (count != 1)
    abort ();
  count = 0;
  b (0xc0000000);
  if (count != 2)
    abort ();
  count = 0;
  b (0xa0000000);
  if (count != 2)
    abort ();
  count = 0;
  b (0x60000000);
  if (count != 2)
    abort ();
  count = 0;
  b (0xe0000000);
  if (count != 3)
    abort ();
  exit (0);
}

