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
void f (int value, int expect)
{
  if (value != expect)
    abort ();
}
int main()
{
  int a = 7, b = 6, c = 4, d = 7, e = 2;
  f (a||b%c, 1);
  f (a?b%c:0, 2);
  f (a=b%c, 2);
  f (a*=b%c, 4);
  f (a/=b%c, 2);
  f (a%=b%c, 0);
  f (a+=b%c, 2);
  f (d||c&&e, 1);
  f (d?c&&e:0, 1);
  f (d=c&&e, 1);
  f (d*=c&&e, 1);
  f (d%=c&&e, 0);
  f (d+=c&&e, 1);
  f (d-=c&&e, 0);
  f (d||c||e, 1);
  f (d?c||e:0, 0);
  f (d=c||e, 1);
  f (d*=c||e, 1);
  f (d%=c||e, 0);
  f (d+=c||e, 1);
  f (d-=c||e, 0);
  exit (0);
}
