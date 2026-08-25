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

typedef unsigned long long my_uint64_t;
void f(my_uint64_t *a, my_uint64_t aa) __attribute__((noinline));
void f(my_uint64_t *a, my_uint64_t aa)
{
  my_uint64_t new_value = aa;
  my_uint64_t old_value = *a;
  int bit_size = 32;
    my_uint64_t mask = (my_uint64_t)(unsigned)(-1);
    my_uint64_t tmp = old_value & mask;
    new_value &= mask;
    if (tmp > new_value)
        new_value += 1ull<<bit_size;
    new_value += old_value & ~mask;
    *a = new_value;
}
int main(void)
{
  my_uint64_t value, new_value, old_value;
  value = 0x100000001;
  old_value = value;
  new_value = (value+1)&(my_uint64_t)(unsigned)(-1);
  f(&value, new_value);
  if (value != old_value+1)
    abort ();
  return 0;
}
