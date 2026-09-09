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

extern void abort(void);
extern int f2(void);
extern int f3(void);
extern void f1(void);
void ff(int fname, int part, int nparts)
{
  if (fname)
    {
      if (nparts)
 f1();
    }
  else
    fname = 2;
  while (f3() )
    {
      if (nparts && f2() )
 {
   f1();
   nparts = part;
   if (f3())
     f1();
   f1();
   break;
 }
    }
  
if (nparts)
    f1();
  return;
}
int main(void)
{
  ff(0, 1, 0);
  return 0;
}
int f3(void) { static int x = 0; x = !x; return x; }
void f1(void) { abort(); }
int f2(void) { abort(); }

