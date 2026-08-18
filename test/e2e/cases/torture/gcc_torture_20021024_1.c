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

void exit (int);
void abort (void);
unsigned long long *cp, m;
void foo (void)
{
}
void bar (unsigned rop, unsigned long long *r)
{
  unsigned rs1, rs2, rd;
top:
  rs2 = (rop >> 23) & 0x1ff;
  rs1 = (rop >> 9) & 0x1ff;
  rd = rop & 0x1ff;
  *cp = 1;
  m = r[rs1] + r[rs2];
  *cp = 2;
  foo();
  if (!rd)
    goto top;
  r[rd] = 1;
}
int main(void)
{
  static unsigned long long r[64];
  unsigned long long cr;
  cp = &cr;
  r[4] = 47;
  r[8] = 11;
  bar((8 << 23) | (4 << 9) | 15, r);
  if (m != 47 + 11)
    abort ();
  exit (0);
}
