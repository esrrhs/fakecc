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
char c = ((char)0xf234);
short s = ((short)0xf234);
int i = ((int)0xf234);
long l = ((long)0xf2345678L);
long long ll = ((long long)0xf2345678abcdef0LL);
int shift1 = 4;
int shift2 = ((sizeof (long long) * 8) - 4);
int main(void)
{
  if ((((c) >> (shift1)) | ((c) << ((sizeof (c) * 8) - (shift1)))) != (((((char)0xf234)) >> (4)) | ((((char)0xf234)) << ((sizeof (((char)0xf234)) * 8) - (4)))))
    abort ();
  if ((((c) >> (4)) | ((c) << ((sizeof (c) * 8) - (4)))) != (((((char)0xf234)) >> (4)) | ((((char)0xf234)) << ((sizeof (((char)0xf234)) * 8) - (4)))))
    abort ();
  if ((((s) >> (shift1)) | ((s) << ((sizeof (s) * 8) - (shift1)))) != (((((short)0xf234)) >> (4)) | ((((short)0xf234)) << ((sizeof (((short)0xf234)) * 8) - (4)))))
    abort ();
  if ((((s) >> (4)) | ((s) << ((sizeof (s) * 8) - (4)))) != (((((short)0xf234)) >> (4)) | ((((short)0xf234)) << ((sizeof (((short)0xf234)) * 8) - (4)))))
    abort ();
  if ((((i) >> (shift1)) | ((i) << ((sizeof (i) * 8) - (shift1)))) != (((((int)0xf234)) >> (4)) | ((((int)0xf234)) << ((sizeof (((int)0xf234)) * 8) - (4)))))
    abort ();
  if ((((i) >> (4)) | ((i) << ((sizeof (i) * 8) - (4)))) != (((((int)0xf234)) >> (4)) | ((((int)0xf234)) << ((sizeof (((int)0xf234)) * 8) - (4)))))
    abort ();
  if ((((l) >> (shift1)) | ((l) << ((sizeof (l) * 8) - (shift1)))) != (((((long)0xf2345678L)) >> (4)) | ((((long)0xf2345678L)) << ((sizeof (((long)0xf2345678L)) * 8) - (4)))))
    abort ();
  if ((((l) >> (4)) | ((l) << ((sizeof (l) * 8) - (4)))) != (((((long)0xf2345678L)) >> (4)) | ((((long)0xf2345678L)) << ((sizeof (((long)0xf2345678L)) * 8) - (4)))))
    abort ();
  if ((((ll) >> (shift1)) | ((ll) << ((sizeof (ll) * 8) - (shift1)))) != (((((long long)0xf2345678abcdef0LL)) >> (4)) | ((((long long)0xf2345678abcdef0LL)) << ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (4)))))
    abort ();
  if ((((ll) >> (4)) | ((ll) << ((sizeof (ll) * 8) - (4)))) != (((((long long)0xf2345678abcdef0LL)) >> (4)) | ((((long long)0xf2345678abcdef0LL)) << ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (4)))))
    abort ();
  if ((((ll) >> (shift2)) | ((ll) << ((sizeof (ll) * 8) - (shift2)))) != (((((long long)0xf2345678abcdef0LL)) >> (((sizeof (long long) * 8) - 4))) | ((((long long)0xf2345678abcdef0LL)) << ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  if ((((ll) >> (((sizeof (long long) * 8) - 4))) | ((ll) << ((sizeof (ll) * 8) - (((sizeof (long long) * 8) - 4))))) != (((((long long)0xf2345678abcdef0LL)) >> (((sizeof (long long) * 8) - 4))) | ((((long long)0xf2345678abcdef0LL)) << ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  if ((((c) << (shift1)) | ((c) >> ((sizeof (c) * 8) - (shift1)))) != (((((char)0xf234)) << (4)) | ((((char)0xf234)) >> ((sizeof (((char)0xf234)) * 8) - (4)))))
    abort ();
  if ((((c) << (4)) | ((c) >> ((sizeof (c) * 8) - (4)))) != (((((char)0xf234)) << (4)) | ((((char)0xf234)) >> ((sizeof (((char)0xf234)) * 8) - (4)))))
    abort ();
  if ((((s) << (shift1)) | ((s) >> ((sizeof (s) * 8) - (shift1)))) != (((((short)0xf234)) << (4)) | ((((short)0xf234)) >> ((sizeof (((short)0xf234)) * 8) - (4)))))
    abort ();
  if ((((s) << (4)) | ((s) >> ((sizeof (s) * 8) - (4)))) != (((((short)0xf234)) << (4)) | ((((short)0xf234)) >> ((sizeof (((short)0xf234)) * 8) - (4)))))
    abort ();
  if ((((i) << (shift1)) | ((i) >> ((sizeof (i) * 8) - (shift1)))) != (((((int)0xf234)) << (4)) | ((((int)0xf234)) >> ((sizeof (((int)0xf234)) * 8) - (4)))))
    abort ();
  if ((((i) << (4)) | ((i) >> ((sizeof (i) * 8) - (4)))) != (((((int)0xf234)) << (4)) | ((((int)0xf234)) >> ((sizeof (((int)0xf234)) * 8) - (4)))))
    abort ();
  if ((((l) << (shift1)) | ((l) >> ((sizeof (l) * 8) - (shift1)))) != (((((long)0xf2345678L)) << (4)) | ((((long)0xf2345678L)) >> ((sizeof (((long)0xf2345678L)) * 8) - (4)))))
    abort ();
  if ((((l) << (4)) | ((l) >> ((sizeof (l) * 8) - (4)))) != (((((long)0xf2345678L)) << (4)) | ((((long)0xf2345678L)) >> ((sizeof (((long)0xf2345678L)) * 8) - (4)))))
    abort ();
  if ((((ll) << (shift1)) | ((ll) >> ((sizeof (ll) * 8) - (shift1)))) != (((((long long)0xf2345678abcdef0LL)) << (4)) | ((((long long)0xf2345678abcdef0LL)) >> ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (4)))))
    abort ();
  if ((((ll) << (4)) | ((ll) >> ((sizeof (ll) * 8) - (4)))) != (((((long long)0xf2345678abcdef0LL)) << (4)) | ((((long long)0xf2345678abcdef0LL)) >> ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (4)))))
    abort ();
  if ((((ll) << (shift2)) | ((ll) >> ((sizeof (ll) * 8) - (shift2)))) != (((((long long)0xf2345678abcdef0LL)) << (((sizeof (long long) * 8) - 4))) | ((((long long)0xf2345678abcdef0LL)) >> ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  if ((((ll) << (((sizeof (long long) * 8) - 4))) | ((ll) >> ((sizeof (ll) * 8) - (((sizeof (long long) * 8) - 4))))) != (((((long long)0xf2345678abcdef0LL)) << (((sizeof (long long) * 8) - 4))) | ((((long long)0xf2345678abcdef0LL)) >> ((sizeof (((long long)0xf2345678abcdef0LL)) * 8) - (((sizeof (long long) * 8) - 4))))))
    abort ();
  exit (0);
}

