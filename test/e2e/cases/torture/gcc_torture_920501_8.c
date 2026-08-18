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

void abort (void);
void exit (int);
char buf[50];
int va(int a, double b, int c, ...)
{
  va_list ap;
  int d, e, f, g, h, i, j, k, l, m, n, o, p;
  va_start (ap, c);
  d = va_arg (ap, int);
  e = va_arg (ap, int);
  f = va_arg (ap, int);
  g = va_arg (ap, int);
  h = va_arg (ap, int);
  i = va_arg (ap, int);
  j = va_arg (ap, int);
  k = va_arg (ap, int);
  l = va_arg (ap, int);
  m = va_arg (ap, int);
  n = va_arg (ap, int);
  o = va_arg (ap, int);
  p = va_arg (ap, int);
  sprintf (buf,
    "%d,%f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
    a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p);
  va_end (ap);
}
int main(void)
{
  va (1, 1.0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
  if (__builtin_strcmp ("1,1.000000,2,3,4,5,6,7,8,9,10,11,12,13,14,15", buf))
    abort();
  exit(0);
}
