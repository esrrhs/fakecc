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

typedef __builtin_va_list va_list;
void abort (void);
void exit (int);
va_list global;
void vat(va_list param, ...)
{
  va_list local;
  __builtin_va_start(local,param);
  __builtin_va_copy(global,local);
  __builtin_va_copy(param,local);
  if (__builtin_va_arg(local,int) != 1)
    abort();
  __builtin_va_end(local);
  if (__builtin_va_arg(global,int) != 1)
    abort();
  __builtin_va_end(global);
  if (__builtin_va_arg(param,int) != 1)
    abort();
  __builtin_va_end(param);
  __builtin_va_start(param,param);
  __builtin_va_start(global,param);
  __builtin_va_copy(local,param);
  if (__builtin_va_arg(local,int) != 1)
    abort();
  __builtin_va_end(local);
  __builtin_va_copy(local,global);
  if (__builtin_va_arg(local,int) != 1)
    abort();
  __builtin_va_end(local);
  if (__builtin_va_arg(global,int) != 1)
    abort();
  __builtin_va_end(global);
  if (__builtin_va_arg(param,int) != 1)
    abort();
  __builtin_va_end(param);
}
int main(void)
{
  va_list t;
  vat (t, 1);
  exit (0);
}
