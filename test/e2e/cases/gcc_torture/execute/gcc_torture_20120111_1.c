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
typedef signed char my_int8_t;
typedef unsigned char my_uint8_t;
typedef short my_int16_t;
typedef unsigned short my_uint16_t;
typedef int my_int32_t;
typedef unsigned int my_uint32_t;
typedef long long my_int64_t;
typedef unsigned long long my_uint64_t;
typedef long my_intptr_t;
typedef unsigned long my_uintptr_t;
typedef long my_intmax_t;
typedef unsigned long my_uintmax_t;
my_uint32_t f0a (my_uint64_t arg2) ;
my_uint32_t
f0a (my_uint64_t arg)
{
  return ~((unsigned) (arg > -3));
}
int main() {
  my_uint32_t r1;
  r1 = f0a (12094370573988097329ULL);
  if (r1 != ~0U)
    abort ();
  return 0;
}

