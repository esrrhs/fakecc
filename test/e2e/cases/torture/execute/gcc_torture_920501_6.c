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
long long unsigned str2llu(char * str) {

  long long unsigned acc;
  int d;
  acc = *str++ - '0';
  for (;;)
    {
      d = *str++;
      if (d == '\0')
 break;
      d -= '0';
      acc = acc * 10 + d;
    }
  return acc;
}
long unsigned sqrtllu(long long unsigned t)
{
  long long unsigned s;
  long long unsigned b;
  for (b = 0, s = t; b++, (s >>= 1) != 0; )
    ;
  s = 1LL << (b >> 1);
  if (b & 1)
    s += s >> 1;
  do
    {
      b = t / s;
      s = (s + b) >> 1;
    }
  
while (b < s);
  return s;
}
int plist(long long unsigned p0, long long unsigned p1, long long unsigned * tab) {

  long long unsigned p;
  long unsigned d;
  long unsigned s;
  long long unsigned *xp = tab;
  for (p = p0; p <= p1; p += 2)
    {
      s = sqrtllu (p);
      for (d = 3; d <= s; d += 2)
 {
   long long unsigned q = p % d;
   if (q == 0)
     goto not_prime;
 }
      *xp++ = p;
    not_prime:;
    }
  *xp = 0;
  return xp - tab;
}
int main(int argc, char ** argv) {

  long long tab[10];
  int nprimes;
  nprimes = plist (str2llu ("1234111111"), str2llu ("1234111127"), tab);
  if(tab[0]!=1234111117LL||tab[1]!=1234111121LL||tab[2]!=1234111127LL||tab[3]!=0)
    abort();
  exit(0);
}

