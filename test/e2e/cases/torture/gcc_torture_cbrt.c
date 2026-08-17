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



static int __fakecc_clz(unsigned int x) {
    if (!x) return 32;
    int n = 0;
    if (!(x & 0xFFFF0000)) { n += 16; x <<= 16; }
    if (!(x & 0xFF000000)) { n += 8; x <<= 8; }
    if (!(x & 0xF0000000)) { n += 4; x <<= 4; }
    if (!(x & 0xC0000000)) { n += 2; x <<= 2; }
    if (!(x & 0x80000000)) { n += 1; }
    return n;
}
static int __fakecc_clzll(unsigned long long x) {
    if (!x) return 64;
    int n = 0;
    if (!(x & 0xFFFFFFFF00000000ULL)) { n += 32; x <<= 32; }
    if (!(x & 0xFFFF000000000000ULL)) { n += 16; x <<= 16; }
    if (!(x & 0xFF00000000000000ULL)) { n += 8; x <<= 8; }
    if (!(x & 0xF000000000000000ULL)) { n += 4; x <<= 4; }
    if (!(x & 0xC000000000000000ULL)) { n += 2; x <<= 2; }
    if (!(x & 0x8000000000000000ULL)) { n += 1; }
    return n;
}
static int __fakecc_ctz(unsigned int x) {
    if (!x) return 32;
    int n = 0;
    if (!(x & 0x0000FFFF)) { n += 16; x >>= 16; }
    if (!(x & 0x000000FF)) { n += 8; x >>= 8; }
    if (!(x & 0x0000000F)) { n += 4; x >>= 4; }
    if (!(x & 0x00000003)) { n += 2; x >>= 2; }
    if (!(x & 0x00000001)) { n += 1; }
    return n;
}
static int __fakecc_ctzll(unsigned long long x) {
    if (!x) return 64;
    int n = 0;
    if (!(x & 0x00000000FFFFFFFFULL)) { n += 32; x >>= 32; }
    if (!(x & 0x000000000000FFFFULL)) { n += 16; x >>= 16; }
    if (!(x & 0x00000000000000FFULL)) { n += 8; x >>= 8; }
    if (!(x & 0x000000000000000FULL)) { n += 4; x >>= 4; }
    if (!(x & 0x0000000000000003ULL)) { n += 2; x >>= 2; }
    if (!(x & 0x0000000000000001ULL)) { n += 1; }
    return n;
}
static int __fakecc_popcount(unsigned int x) {
    int c = 0;
    while (x) { c += (x & 1); x >>= 1; }
    return c;
}
static int __fakecc_popcountll(unsigned long long x) {
    int c = 0;
    while (x) { c += (x & 1); x >>= 1; }
    return c;
}
static int __fakecc_parity(unsigned int x) {
    return __fakecc_popcount(x) & 1;
}
static int __fakecc_parityll(unsigned long long x) {
    return __fakecc_popcountll(x) & 1;
}
static int __fakecc_ffs(unsigned int x) {
    if (!x) return 0;
    return __fakecc_ctz(x) + 1;
}
static int __fakecc_ffsll(unsigned long long x) {
    if (!x) return 0;
    return __fakecc_ctzll(x) + 1;
}


void abort (void);
void exit (int);
static const unsigned long
 B1 = 715094163,
 B2 = 696219795;
static const double
 C = 5.42857142857142815906e-01,
 D = -7.05306122448979611050e-01,
 E = 1.41428571428571436819e+00,
 F = 1.60714285714285720630e+00,
 G = 3.57142857142857150787e-01;
double cbrtl(double x)
{
  long hx;
  double r,s,w;
  double lt;
  unsigned sign;
  typedef unsigned unsigned32 ;
  union {
    double t;
    unsigned32 pt[2];
  } ut, ux;
  int n0;
  ut.t = 1.0;
  n0 = (ut.pt[0] == 0);
  ut.t = 0.0;
  ux.t = x;
  hx = ux.pt[n0];
  sign=hx&0x80000000;
  hx ^=sign;
  if(hx>=0x7ff00000) return(x+x);
  if((hx| ux.pt[1-n0])==0)
    return(ux.t);
  ux.pt[n0] = hx;
  if(hx<0x00100000)
    {ut.pt[n0]=0x43500000;
     ut.t*=x; ut.pt[n0]=ut.pt[n0]/3+B2;
   }
  else
    ut.pt[n0]=hx/3+B1;
  r=ut.t*ut.t/ux.t;
  s=C+r*ut.t;
  ut.t*=G+F/(s+E+D/s);
  ut.pt[1-n0]=0; ut.pt[n0]+=0x00000001;
  s=ut.t*ut.t;
  r=ux.t/s;
  w=ut.t+ut.t;
  r=(r-ut.t)/(w+r);
  ut.t=ut.t+ut.t*r;
  ut.pt[n0] |= sign;
  lt = ut.t;
  lt -= (lt - (x/(lt*lt))) * 0.333333333333333333333;
  return lt;
}
int main(void)
{
  if ((int) (cbrtl (27.0) + 0.5) != 3)
    abort ();
  exit (0);
}

