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
long f1(long a){return a&0xff000000L;}
long f2 (long a){return a&~0xff000000L;}
long f3(long a){return a&0x000000ffL;}
long f4(long a){return a&~0x000000ffL;}
long f5(long a){return a&0x0000ffffL;}
long f6(long a){return a&~0x0000ffffL;}
int
main (void)
{
  long a = 0x89ABCDEF;
  if (f1(a)!=0x89000000L||
      f2(a)!=0x00ABCDEFL||
      f3(a)!=0x000000EFL||
      f4(a)!=0x89ABCD00L||
      f5(a)!=0x0000CDEFL||
      f6(a)!=0x89AB0000L)
    abort();
  exit(0);
}
