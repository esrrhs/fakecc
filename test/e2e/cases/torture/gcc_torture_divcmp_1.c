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


extern void abort(void);
int test1(int x)
{
  return x/10 == 2;
}
int test1u(unsigned int x)
{
  return x/10U == 2;
}
int test2(int x)
{
  return x/10 == 0;
}
int test2u(unsigned int x)
{
  return x/10U == 0;
}
int test3(int x)
{
  return x/10 != 2;
}
int test3u(unsigned int x)
{
  return x/10U != 2;
}
int test4(int x)
{
  return x/10 != 0;
}
int test4u(unsigned int x)
{
  return x/10U != 0;
}
int test5(int x)
{
  return x/10 < 2;
}
int test5u(unsigned int x)
{
  return x/10U < 2;
}
int test6(int x)
{
  return x/10 < 0;
}
int test7(int x)
{
  return x/10 <= 2;
}
int test7u(unsigned int x)
{
  return x/10U <= 2;
}
int test8(int x)
{
  return x/10 <= 0;
}
int test8u(unsigned int x)
{
  return x/10U <= 0;
}
int test9(int x)
{
  return x/10 > 2;
}
int test9u(unsigned int x)
{
  return x/10U > 2;
}
int test10(int x)
{
  return x/10 > 0;
}
int test10u(unsigned int x)
{
  return x/10U > 0;
}
int test11(int x)
{
  return x/10 >= 2;
}
int test11u(unsigned int x)
{
  return x/10U >= 2;
}
int test12(int x)
{
  return x/10 >= 0;
}
int main()
{
  if (test1(19) != 0)
    abort ();
  if (test1(20) != 1)
    abort ();
  if (test1(29) != 1)
    abort ();
  if (test1(30) != 0)
    abort ();
  if (test1u(19) != 0)
    abort ();
  if (test1u(20) != 1)
    abort ();
  if (test1u(29) != 1)
    abort ();
  if (test1u(30) != 0)
    abort ();
  if (test2(0) != 1)
    abort ();
  if (test2(9) != 1)
    abort ();
  if (test2(10) != 0)
    abort ();
  if (test2(-1) != 1)
    abort ();
  if (test2(-9) != 1)
    abort ();
  if (test2(-10) != 0)
    abort ();
  if (test2u(0) != 1)
    abort ();
  if (test2u(9) != 1)
    abort ();
  if (test2u(10) != 0)
    abort ();
  if (test2u(-1) != 0)
    abort ();
  if (test2u(-9) != 0)
    abort ();
  if (test2u(-10) != 0)
    abort ();
  if (test3(19) != 1)
    abort ();
  if (test3(20) != 0)
    abort ();
  if (test3(29) != 0)
    abort ();
  if (test3(30) != 1)
    abort ();
  if (test3u(19) != 1)
    abort ();
  if (test3u(20) != 0)
    abort ();
  if (test3u(29) != 0)
    abort ();
  if (test3u(30) != 1)
    abort ();
  if (test4(0) != 0)
    abort ();
  if (test4(9) != 0)
    abort ();
  if (test4(10) != 1)
    abort ();
  if (test4(-1) != 0)
    abort ();
  if (test4(-9) != 0)
    abort ();
  if (test4(-10) != 1)
    abort ();
  if (test4u(0) != 0)
    abort ();
  if (test4u(9) != 0)
    abort ();
  if (test4u(10) != 1)
    abort ();
  if (test4u(-1) != 1)
    abort ();
  if (test4u(-9) != 1)
    abort ();
  if (test4u(-10) != 1)
    abort ();
  if (test5(19) != 1)
    abort ();
  if (test5(20) != 0)
    abort ();
  if (test5(29) != 0)
    abort ();
  if (test5(30) != 0)
    abort ();
  if (test5u(19) != 1)
    abort ();
  if (test5u(20) != 0)
    abort ();
  if (test5u(29) != 0)
    abort ();
  if (test5u(30) != 0)
    abort ();
  if (test6(0) != 0)
    abort ();
  if (test6(9) != 0)
    abort ();
  if (test6(10) != 0)
    abort ();
  if (test6(-1) != 0)
    abort ();
  if (test6(-9) != 0)
    abort ();
  if (test6(-10) != 1)
    abort ();
  if (test7(19) != 1)
    abort ();
  if (test7(20) != 1)
    abort ();
  if (test7(29) != 1)
    abort ();
  if (test7(30) != 0)
    abort ();
  if (test7u(19) != 1)
    abort ();
  if (test7u(20) != 1)
    abort ();
  if (test7u(29) != 1)
    abort ();
  if (test7u(30) != 0)
    abort ();
  if (test8(0) != 1)
    abort ();
  if (test8(9) != 1)
    abort ();
  if (test8(10) != 0)
    abort ();
  if (test8(-1) != 1)
    abort ();
  if (test8(-9) != 1)
    abort ();
  if (test8(-10) != 1)
    abort ();
  if (test8u(0) != 1)
    abort ();
  if (test8u(9) != 1)
    abort ();
  if (test8u(10) != 0)
    abort ();
  if (test8u(-1) != 0)
    abort ();
  if (test8u(-9) != 0)
    abort ();
  if (test8u(-10) != 0)
    abort ();
  if (test9(19) != 0)
    abort ();
  if (test9(20) != 0)
    abort ();
  if (test9(29) != 0)
    abort ();
  if (test9(30) != 1)
    abort ();
  if (test9u(19) != 0)
    abort ();
  if (test9u(20) != 0)
    abort ();
  if (test9u(29) != 0)
    abort ();
  if (test9u(30) != 1)
    abort ();
  if (test10(0) != 0)
    abort ();
  if (test10(9) != 0)
    abort ();
  if (test10(10) != 1)
    abort ();
  if (test10(-1) != 0)
    abort ();
  if (test10(-9) != 0)
    abort ();
  if (test10(-10) != 0)
    abort ();
  if (test10u(0) != 0)
    abort ();
  if (test10u(9) != 0)
    abort ();
  if (test10u(10) != 1)
    abort ();
  if (test10u(-1) != 1)
    abort ();
  if (test10u(-9) != 1)
    abort ();
  if (test10u(-10) != 1)
    abort ();
  if (test11(19) != 0)
    abort ();
  if (test11(20) != 1)
    abort ();
  if (test11(29) != 1)
    abort ();
  if (test11(30) != 1)
    abort ();
  if (test11u(19) != 0)
    abort ();
  if (test11u(20) != 1)
    abort ();
  if (test11u(29) != 1)
    abort ();
  if (test11u(30) != 1)
    abort ();
  if (test12(0) != 1)
    abort ();
  if (test12(9) != 1)
    abort ();
  if (test12(10) != 1)
    abort ();
  if (test12(-1) != 1)
    abort ();
  if (test12(-9) != 1)
    abort ();
  if (test12(-10) != 0)
    abort ();
  return 0;
}

