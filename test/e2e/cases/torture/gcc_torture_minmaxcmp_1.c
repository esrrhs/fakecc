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


_Bool min_lt_(int a, int b) { int x = ((a) < (b) ? (a) : (b)); return ((x) < (a)); } _Bool min_lt_volatile (int a, int b) { volatile int x = ((a) < (b) ? (a) : (b)); return ((x) < (a)); } _Bool max_lt_ (int a, int b) { int x = ((a) > (b) ? (a) : (b)); return ((x) < (a)); } _Bool max_lt_volatile (int a, int b) { volatile int x = ((a) > (b) ? (a) : (b)); return ((x) < (a)); } _Bool min_gt_ (int a, int b) { int x = ((a) < (b) ? (a) : (b)); return ((x) > (a)); } _Bool min_gt_volatile (int a, int b) { volatile int x = ((a) < (b) ? (a) : (b)); return ((x) > (a)); } _Bool max_gt_ (int a, int b) { int x = ((a) > (b) ? (a) : (b)); return ((x) > (a)); } _Bool max_gt_volatile (int a, int b) { volatile int x = ((a) > (b) ? (a) : (b)); return ((x) > (a)); } _Bool min_le_ (int a, int b) { int x = ((a) < (b) ? (a) : (b)); return ((x) <= (a)); } _Bool min_le_volatile (int a, int b) { volatile int x = ((a) < (b) ? (a) : (b)); return ((x) <= (a)); } _Bool max_le_ (int a, int b) { int x = ((a) > (b) ? (a) : (b)); return ((x) <= (a)); } _Bool max_le_volatile (int a, int b) { volatile int x = ((a) > (b) ? (a) : (b)); return ((x) <= (a)); } _Bool min_ge_ (int a, int b) { int x = ((a) < (b) ? (a) : (b)); return ((x) >= (a)); } _Bool min_ge_volatile (int a, int b) { volatile int x = ((a) < (b) ? (a) : (b)); return ((x) >= (a)); } _Bool max_ge_ (int a, int b) { int x = ((a) > (b) ? (a) : (b)); return ((x) >= (a)); } _Bool max_ge_volatile (int a, int b) { volatile int x = ((a) > (b) ? (a) : (b)); return ((x) >= (a)); } _Bool min_ne_ (int a, int b) { int x = ((a) < (b) ? (a) : (b)); return ((x) != (a)); } _Bool min_ne_volatile (int a, int b) { volatile int x = ((a) < (b) ? (a) : (b)); return ((x) != (a)); } _Bool max_ne_ (int a, int b) { int x = ((a) > (b) ? (a) : (b)); return ((x) != (a)); } _Bool max_ne_volatile (int a, int b) { volatile int x = ((a) > (b) ? (a) : (b)); return ((x) != (a)); } _Bool min_eq_ (int a, int b) { int x = ((a) < (b) ? (a) : (b)); return ((x) == (a)); } _Bool min_eq_volatile (int a, int b) { volatile int x = ((a) < (b) ? (a) : (b)); return ((x) == (a)); } _Bool max_eq_ (int a, int b) { int x = ((a) > (b) ? (a) : (b)); return ((x) == (a)); } _Bool max_eq_volatile (int a, int b) { volatile int x = ((a) > (b) ? (a) : (b)); return ((x) == (a)); }
int main()
{
  for(int x = -10; x < 10; x++)
    for(int y = -10; y < 10; y++)
    {
        do { if (min_lt_(x,y) != min_lt_volatile(x,y)) abort(); } 
while(0); do { if (max_lt_(x,y) != max_lt_volatile(x,y)) abort(); } 
while(0); do { if (min_gt_(x,y) != min_gt_volatile(x,y)) abort(); } 
while(0); do { if (max_gt_(x,y) != max_gt_volatile(x,y)) abort(); } 
while(0); do { if (min_le_(x,y) != min_le_volatile(x,y)) abort(); } 
while(0); do { if (max_le_(x,y) != max_le_volatile(x,y)) abort(); } 
while(0); do { if (min_ge_(x,y) != min_ge_volatile(x,y)) abort(); } 
while(0); do { if (max_ge_(x,y) != max_ge_volatile(x,y)) abort(); } 
while(0); do { if (min_ne_(x,y) != min_ne_volatile(x,y)) abort(); } 
while(0); do { if (max_ne_(x,y) != max_ne_volatile(x,y)) abort(); } 
while(0); do { if (min_eq_(x,y) != min_eq_volatile(x,y)) abort(); } 
while(0); do { if (max_eq_(x,y) != max_eq_volatile(x,y)) abort(); } 
while(0);
    }
}

