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
typedef unsigned long int st;
typedef unsigned long long dt;
typedef union
{
  dt d;
  struct
  {
    st h, l;
  }
  s;
} t_be;
typedef union
{
  dt d;
  struct
  {
    st l, h;
  }
  s;
} t_le;
int f_le(t_le afh, t_le bfh) { t_le hh; t_le hp, lp, dp, m; st ad, bd; int s; s = 0; ad = afh.s.h - afh.s.l; bd = bfh.s.l - bfh.s.h; if (bd > bfh.s.l) { bd = -bd; s = ~s; } lp.d = (dt) afh.s.l * bfh.s.l; hp.d = (dt) afh.s.h * bfh.s.h; dp.d = (dt) ad *bd; dp.d ^= s; hh.d = hp.d + hp.s.h + lp.s.h + dp.s.h; m.d = (dt) lp.s.h + hp.s.l + lp.s.l + dp.s.l; return hh.s.l + m.s.l; }
int f_be(t_be afh, t_be bfh) { t_be hh; t_be hp, lp, dp, m; st ad, bd; int s; s = 0; ad = afh.s.h - afh.s.l; bd = bfh.s.l - bfh.s.h; if (bd > bfh.s.l) { bd = -bd; s = ~s; } lp.d = (dt) afh.s.l * bfh.s.l; hp.d = (dt) afh.s.h * bfh.s.h; dp.d = (dt) ad *bd; dp.d ^= s; hh.d = hp.d + hp.s.h + lp.s.h + dp.s.h; m.d = (dt) lp.s.h + hp.s.l + lp.s.l + dp.s.l; return hh.s.l + m.s.l; }
int main(void)
{
  t_be x;
  x.s.h = 0x10000000U;
  x.s.l = 0xe0000000U;
  if (x.d == 0x10000000e0000000ULL
      && f_be ((t_be) 0x100000000ULL, (t_be) 0x100000000ULL) != -1)
    abort ();
  if (x.d == 0xe000000010000000ULL
      && f_le ((t_le) 0x100000000ULL, (t_le) 0x100000000ULL) != -1)
    abort ();
  exit (0);
}

