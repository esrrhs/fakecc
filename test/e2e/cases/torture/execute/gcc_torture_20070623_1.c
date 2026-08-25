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
int  nge(int a, int b) {return -(a >= b);}
int  ngt(int a, int b) {return -(a > b);}
int  nle(int a, int b) {return -(a <= b);}
int  nlt(int a, int b) {return -(a < b);}
int  neq(int a, int b) {return -(a == b);}
int  nne(int a, int b) {return -(a != b);}
int  ngeu(unsigned a, unsigned b) {return -(a >= b);}
int  ngtu(unsigned a, unsigned b) {return -(a > b);}
int  nleu(unsigned a, unsigned b) {return -(a <= b);}
int  nltu(unsigned a, unsigned b) {return -(a < b);}
int main()
{
  if (nge((-2147483647 - 1), 2147483647) != 0) abort();
  if (nge(2147483647, (-2147483647 - 1)) != -1) abort();
  if (ngt((-2147483647 - 1), 2147483647) != 0) abort();
  if (ngt(2147483647, (-2147483647 - 1)) != -1) abort();
  if (nle((-2147483647 - 1), 2147483647) != -1) abort();
  if (nle(2147483647, (-2147483647 - 1)) != 0) abort();
  if (nlt((-2147483647 - 1), 2147483647) != -1) abort();
  if (nlt(2147483647, (-2147483647 - 1)) != 0) abort();
  if (neq((-2147483647 - 1), 2147483647) != 0) abort();
  if (neq(2147483647, (-2147483647 - 1)) != 0) abort();
  if (nne((-2147483647 - 1), 2147483647) != -1) abort();
  if (nne(2147483647, (-2147483647 - 1)) != -1) abort();
  if (ngeu(0, ~0U) != 0) abort();
  if (ngeu(~0U, 0) != -1) abort();
  if (ngtu(0, ~0U) != 0) abort();
  if (ngtu(~0U, 0) != -1) abort();
  if (nleu(0, ~0U) != -1) abort();
  if (nleu(~0U, 0) != 0) abort();
  if (nltu(0, ~0U) != -1) abort();
  if (nltu(~0U, 0) != 0) abort();
  exit(0);
}

