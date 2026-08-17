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


typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef int wchar_t;
void abort (void);
void exit (int);
static union {
  char buf[((sizeof (long long)) + 15 + (sizeof (long long)))];
  long long align_int;
  long double align_fp;
} u;
char A = 'A';
void reset()
{
  int i;
  for (i = 0; i < ((sizeof (long long)) + 15 + (sizeof (long long))); i++)
    u.buf[i] = 'a';
}
void check(int off, int len, int ch)
{
  char *q;
  int i;
  q = u.buf;
  for (i = 0; i < off; i++, q++)
    if (*q != 'a')
      abort ();
  for (i = 0; i < len; i++, q++)
    if (*q != ch)
      abort ();
  for (i = 0; i < (sizeof (long long)); i++, q++)
    if (*q != 'a')
      abort ();
}
int main()
{
  int len;
  char *p;
  for (len = 0; len < 15; len++)
    {
      reset ();
      p = memset (u.buf, '\0', len);
      if (p != u.buf) abort ();
      check (0, len, '\0');
      p = memset (u.buf, A, len);
      if (p != u.buf) abort ();
      check (0, len, 'A');
      p = memset (u.buf, 'B', len);
      if (p != u.buf) abort ();
      check (0, len, 'B');
    }
  
for (len = 0; len < 15; len++)
    {
      reset ();
      p = memset (u.buf+1, '\0', len);
      if (p != u.buf+1) abort ();
      check (1, len, '\0');
      p = memset (u.buf+1, A, len);
      if (p != u.buf+1) abort ();
      check (1, len, 'A');
      p = memset (u.buf+1, 'B', len);
      if (p != u.buf+1) abort ();
      check (1, len, 'B');
    }
  
for (len = 0; len < 15; len++)
    {
      reset ();
      p = memset (u.buf+2, '\0', len);
      if (p != u.buf+2) abort ();
      check (2, len, '\0');
      p = memset (u.buf+2, A, len);
      if (p != u.buf+2) abort ();
      check (2, len, 'A');
      p = memset (u.buf+2, 'B', len);
      if (p != u.buf+2) abort ();
      check (2, len, 'B');
    }
  
for (len = 0; len < 15; len++)
    {
      reset ();
      p = memset (u.buf+3, '\0', len);
      if (p != u.buf+3) abort ();
      check (3, len, '\0');
      p = memset (u.buf+3, A, len);
      if (p != u.buf+3) abort ();
      check (3, len, 'A');
      p = memset (u.buf+3, 'B', len);
      if (p != u.buf+3) abort ();
      check (3, len, 'B');
    }
  
for (len = 0; len < 15; len++)
    {
      reset ();
      p = memset (u.buf+4, '\0', len);
      if (p != u.buf+4) abort ();
      check (4, len, '\0');
      p = memset (u.buf+4, A, len);
      if (p != u.buf+4) abort ();
      check (4, len, 'A');
      p = memset (u.buf+4, 'B', len);
      if (p != u.buf+4) abort ();
      check (4, len, 'B');
    }
  
for (len = 0; len < 15; len++)
    {
      reset ();
      p = memset (u.buf+5, '\0', len);
      if (p != u.buf+5) abort ();
      check (5, len, '\0');
      p = memset (u.buf+5, A, len);
      if (p != u.buf+5) abort ();
      check (5, len, 'A');
      p = memset (u.buf+5, 'B', len);
      if (p != u.buf+5) abort ();
      check (5, len, 'B');
    }
  
for (len = 0; len < 15; len++)
    {
      reset ();
      p = memset (u.buf+6, '\0', len);
      if (p != u.buf+6) abort ();
      check (6, len, '\0');
      p = memset (u.buf+6, A, len);
      if (p != u.buf+6) abort ();
      check (6, len, 'A');
      p = memset (u.buf+6, 'B', len);
      if (p != u.buf+6) abort ();
      check (6, len, 'B');
    }
  
for (len = 0; len < 15; len++)
    {
      reset ();
      p = memset (u.buf+7, '\0', len);
      if (p != u.buf+7) abort ();
      check (7, len, '\0');
      p = memset (u.buf+7, A, len);
      if (p != u.buf+7) abort ();
      check (7, len, 'A');
      p = memset (u.buf+7, 'B', len);
      if (p != u.buf+7) abort ();
      check (7, len, 'B');
    }
  
exit (0);
}

