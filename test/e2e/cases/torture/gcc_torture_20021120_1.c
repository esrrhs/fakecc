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
volatile double gd[32];
volatile float gf[32];
void foo(int n)
{
  double d00 , d10 , d20 , d30 , d01 , d11 , d21 , d31 , d02 , d12 , d22 , d32 , d03 , d13 , d23 , d33 , d04 , d14 , d24 , d34 , d05 , d15 , d25 , d35 , d06 , d16 , d26 , d36 , d07 , d17 , d27 , d37 ;
  float f00 , f10 , f20 , f30 , f01 , f11 , f21 , f31 , f02 , f12 , f22 , f32 , f03 , f13 , f23 , f33 , f04 , f14 , f24 , f34 , f05 , f15 , f25 , f35 , f06 , f16 , f26 , f36 , f07 , f17 , f27 , f37 ;
  volatile double *pd;
  volatile float *pf;
  int i;
  pd = gd; d00 = *(pd++), d10 = *(pd++), d20 = *(pd++), d30 = *(pd++), d01 = *(pd++), d11 = *(pd++), d21 = *(pd++), d31 = *(pd++), d02 = *(pd++), d12 = *(pd++), d22 = *(pd++), d32 = *(pd++), d03 = *(pd++), d13 = *(pd++), d23 = *(pd++), d33 = *(pd++), d04 = *(pd++), d14 = *(pd++), d24 = *(pd++), d34 = *(pd++), d05 = *(pd++), d15 = *(pd++), d25 = *(pd++), d35 = *(pd++), d06 = *(pd++), d16 = *(pd++), d26 = *(pd++), d36 = *(pd++), d07 = *(pd++), d17 = *(pd++), d27 = *(pd++), d37 = *(pd++);
  for (i = 0; i < n; i++)
    {
      pf = gf; f00 = *(pf++), f10 = *(pf++), f20 = *(pf++), f30 = *(pf++), f01 = *(pf++), f11 = *(pf++), f21 = *(pf++), f31 = *(pf++), f02 = *(pf++), f12 = *(pf++), f22 = *(pf++), f32 = *(pf++), f03 = *(pf++), f13 = *(pf++), f23 = *(pf++), f33 = *(pf++), f04 = *(pf++), f14 = *(pf++), f24 = *(pf++), f34 = *(pf++), f05 = *(pf++), f15 = *(pf++), f25 = *(pf++), f35 = *(pf++), f06 = *(pf++), f16 = *(pf++), f26 = *(pf++), f36 = *(pf++), f07 = *(pf++), f17 = *(pf++), f27 = *(pf++), f37 = *(pf++);
      pd = gd; d00 += *(pd++), d10 += *(pd++), d20 += *(pd++), d30 += *(pd++), d01 += *(pd++), d11 += *(pd++), d21 += *(pd++), d31 += *(pd++), d02 += *(pd++), d12 += *(pd++), d22 += *(pd++), d32 += *(pd++), d03 += *(pd++), d13 += *(pd++), d23 += *(pd++), d33 += *(pd++), d04 += *(pd++), d14 += *(pd++), d24 += *(pd++), d34 += *(pd++), d05 += *(pd++), d15 += *(pd++), d25 += *(pd++), d35 += *(pd++), d06 += *(pd++), d16 += *(pd++), d26 += *(pd++), d36 += *(pd++), d07 += *(pd++), d17 += *(pd++), d27 += *(pd++), d37 += *(pd++);
      pd = gd; d00 += *(pd++), d10 += *(pd++), d20 += *(pd++), d30 += *(pd++), d01 += *(pd++), d11 += *(pd++), d21 += *(pd++), d31 += *(pd++), d02 += *(pd++), d12 += *(pd++), d22 += *(pd++), d32 += *(pd++), d03 += *(pd++), d13 += *(pd++), d23 += *(pd++), d33 += *(pd++), d04 += *(pd++), d14 += *(pd++), d24 += *(pd++), d34 += *(pd++), d05 += *(pd++), d15 += *(pd++), d25 += *(pd++), d35 += *(pd++), d06 += *(pd++), d16 += *(pd++), d26 += *(pd++), d36 += *(pd++), d07 += *(pd++), d17 += *(pd++), d27 += *(pd++), d37 += *(pd++);
      pd = gd; d00 += *(pd++), d10 += *(pd++), d20 += *(pd++), d30 += *(pd++), d01 += *(pd++), d11 += *(pd++), d21 += *(pd++), d31 += *(pd++), d02 += *(pd++), d12 += *(pd++), d22 += *(pd++), d32 += *(pd++), d03 += *(pd++), d13 += *(pd++), d23 += *(pd++), d33 += *(pd++), d04 += *(pd++), d14 += *(pd++), d24 += *(pd++), d34 += *(pd++), d05 += *(pd++), d15 += *(pd++), d25 += *(pd++), d35 += *(pd++), d06 += *(pd++), d16 += *(pd++), d26 += *(pd++), d36 += *(pd++), d07 += *(pd++), d17 += *(pd++), d27 += *(pd++), d37 += *(pd++);
      pf = gf; *(pf++) = f00 , *(pf++) = f10 , *(pf++) = f20 , *(pf++) = f30 , *(pf++) = f01 , *(pf++) = f11 , *(pf++) = f21 , *(pf++) = f31 , *(pf++) = f02 , *(pf++) = f12 , *(pf++) = f22 , *(pf++) = f32 , *(pf++) = f03 , *(pf++) = f13 , *(pf++) = f23 , *(pf++) = f33 , *(pf++) = f04 , *(pf++) = f14 , *(pf++) = f24 , *(pf++) = f34 , *(pf++) = f05 , *(pf++) = f15 , *(pf++) = f25 , *(pf++) = f35 , *(pf++) = f06 , *(pf++) = f16 , *(pf++) = f26 , *(pf++) = f36 , *(pf++) = f07 , *(pf++) = f17 , *(pf++) = f27 , *(pf++) = f37 ;
    }
  pd = gd; *(pd++) = d00 , *(pd++) = d10 , *(pd++) = d20 , *(pd++) = d30 , *(pd++) = d01 , *(pd++) = d11 , *(pd++) = d21 , *(pd++) = d31 , *(pd++) = d02 , *(pd++) = d12 , *(pd++) = d22 , *(pd++) = d32 , *(pd++) = d03 , *(pd++) = d13 , *(pd++) = d23 , *(pd++) = d33 , *(pd++) = d04 , *(pd++) = d14 , *(pd++) = d24 , *(pd++) = d34 , *(pd++) = d05 , *(pd++) = d15 , *(pd++) = d25 , *(pd++) = d35 , *(pd++) = d06 , *(pd++) = d16 , *(pd++) = d26 , *(pd++) = d36 , *(pd++) = d07 , *(pd++) = d17 , *(pd++) = d27 , *(pd++) = d37 ;
}
int main()
{
  int i;
  for (i = 0; i < 32; i++)
    gd[i] = i, gf[i] = i;
  foo (1);
  for (i = 0; i < 32; i++)
    if (gd[i] != i * 4 || gf[i] != i)
      abort ();
  exit (0);
}

