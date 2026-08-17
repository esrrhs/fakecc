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
  int off;
  char *p;
  for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 1);
      if (p != u.buf + off) abort ();
      check (off, 1, '\0');
      p = memset (u.buf + off, A, 1);
      if (p != u.buf + off) abort ();
      check (off, 1, 'A');
      p = memset (u.buf + off, 'B', 1);
      if (p != u.buf + off) abort ();
      check (off, 1, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 2);
      if (p != u.buf + off) abort ();
      check (off, 2, '\0');
      p = memset (u.buf + off, A, 2);
      if (p != u.buf + off) abort ();
      check (off, 2, 'A');
      p = memset (u.buf + off, 'B', 2);
      if (p != u.buf + off) abort ();
      check (off, 2, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 3);
      if (p != u.buf + off) abort ();
      check (off, 3, '\0');
      p = memset (u.buf + off, A, 3);
      if (p != u.buf + off) abort ();
      check (off, 3, 'A');
      p = memset (u.buf + off, 'B', 3);
      if (p != u.buf + off) abort ();
      check (off, 3, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 4);
      if (p != u.buf + off) abort ();
      check (off, 4, '\0');
      p = memset (u.buf + off, A, 4);
      if (p != u.buf + off) abort ();
      check (off, 4, 'A');
      p = memset (u.buf + off, 'B', 4);
      if (p != u.buf + off) abort ();
      check (off, 4, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 5);
      if (p != u.buf + off) abort ();
      check (off, 5, '\0');
      p = memset (u.buf + off, A, 5);
      if (p != u.buf + off) abort ();
      check (off, 5, 'A');
      p = memset (u.buf + off, 'B', 5);
      if (p != u.buf + off) abort ();
      check (off, 5, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 6);
      if (p != u.buf + off) abort ();
      check (off, 6, '\0');
      p = memset (u.buf + off, A, 6);
      if (p != u.buf + off) abort ();
      check (off, 6, 'A');
      p = memset (u.buf + off, 'B', 6);
      if (p != u.buf + off) abort ();
      check (off, 6, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 7);
      if (p != u.buf + off) abort ();
      check (off, 7, '\0');
      p = memset (u.buf + off, A, 7);
      if (p != u.buf + off) abort ();
      check (off, 7, 'A');
      p = memset (u.buf + off, 'B', 7);
      if (p != u.buf + off) abort ();
      check (off, 7, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 8);
      if (p != u.buf + off) abort ();
      check (off, 8, '\0');
      p = memset (u.buf + off, A, 8);
      if (p != u.buf + off) abort ();
      check (off, 8, 'A');
      p = memset (u.buf + off, 'B', 8);
      if (p != u.buf + off) abort ();
      check (off, 8, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 9);
      if (p != u.buf + off) abort ();
      check (off, 9, '\0');
      p = memset (u.buf + off, A, 9);
      if (p != u.buf + off) abort ();
      check (off, 9, 'A');
      p = memset (u.buf + off, 'B', 9);
      if (p != u.buf + off) abort ();
      check (off, 9, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 10);
      if (p != u.buf + off) abort ();
      check (off, 10, '\0');
      p = memset (u.buf + off, A, 10);
      if (p != u.buf + off) abort ();
      check (off, 10, 'A');
      p = memset (u.buf + off, 'B', 10);
      if (p != u.buf + off) abort ();
      check (off, 10, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 11);
      if (p != u.buf + off) abort ();
      check (off, 11, '\0');
      p = memset (u.buf + off, A, 11);
      if (p != u.buf + off) abort ();
      check (off, 11, 'A');
      p = memset (u.buf + off, 'B', 11);
      if (p != u.buf + off) abort ();
      check (off, 11, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 12);
      if (p != u.buf + off) abort ();
      check (off, 12, '\0');
      p = memset (u.buf + off, A, 12);
      if (p != u.buf + off) abort ();
      check (off, 12, 'A');
      p = memset (u.buf + off, 'B', 12);
      if (p != u.buf + off) abort ();
      check (off, 12, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 13);
      if (p != u.buf + off) abort ();
      check (off, 13, '\0');
      p = memset (u.buf + off, A, 13);
      if (p != u.buf + off) abort ();
      check (off, 13, 'A');
      p = memset (u.buf + off, 'B', 13);
      if (p != u.buf + off) abort ();
      check (off, 13, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 14);
      if (p != u.buf + off) abort ();
      check (off, 14, '\0');
      p = memset (u.buf + off, A, 14);
      if (p != u.buf + off) abort ();
      check (off, 14, 'A');
      p = memset (u.buf + off, 'B', 14);
      if (p != u.buf + off) abort ();
      check (off, 14, 'B');
    }
  
for (off = 0; off < (sizeof (long long)); off++)
    {
      reset ();
      p = memset (u.buf + off, '\0', 15);
      if (p != u.buf + off) abort ();
      check (off, 15, '\0');
      p = memset (u.buf + off, A, 15);
      if (p != u.buf + off) abort ();
      check (off, 15, 'A');
      p = memset (u.buf + off, 'B', 15);
      if (p != u.buf + off) abort ();
      check (off, 15, 'B');
    }
  
exit (0);
}

