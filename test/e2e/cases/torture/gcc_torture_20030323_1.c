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


void * NSReturnAddress(int offset)
{
 switch (offset) {
 case 0: return ((void*)0);
 case 1: return ((void*)0);
 case 2: return ((void*)0);
 case 3: return ((void*)0);
 case 4: return ((void*)0);
 case 5: return ((void*)0);
 case 6: return ((void*)0);
 case 7: return ((void*)0);
 case 8: return ((void*)0);
 case 9: return ((void*)0);
 case 10: return ((void*)0);
 case 11: return ((void*)0);
 case 12: return ((void*)0);
 case 13: return ((void*)0);
 case 14: return ((void*)0);
 case 15: return ((void*)0);
 case 16: return ((void*)0);
 case 17: return ((void*)0);
 case 18: return ((void*)0);
 case 19: return ((void*)0);
 case 20: return ((void*)0);
 case 21: return ((void*)0);
 case 22: return ((void*)0);
 case 23: return ((void*)0);
 case 24: return ((void*)0);
 case 25: return ((void*)0);
 case 26: return ((void*)0);
 case 27: return ((void*)0);
 case 28: return ((void*)0);
 case 29: return ((void*)0);
 case 30: return ((void*)0);
 case 31: return ((void*)0);
 case 32: return ((void*)0);
 case 33: return ((void*)0);
 case 34: return ((void*)0);
 case 35: return ((void*)0);
 case 36: return ((void*)0);
 case 37: return ((void*)0);
 case 38: return ((void*)0);
 case 39: return ((void*)0);
 case 40: return ((void*)0);
 case 41: return ((void*)0);
 case 42: return ((void*)0);
 case 43: return ((void*)0);
 case 44: return ((void*)0);
 case 45: return ((void*)0);
 case 46: return ((void*)0);
 case 47: return ((void*)0);
 case 48: return ((void*)0);
 case 49: return ((void*)0);
 case 50: return ((void*)0);
 case 51: return ((void*)0);
 case 52: return ((void*)0);
 case 53: return ((void*)0);
 case 54: return ((void*)0);
 case 55: return ((void*)0);
 case 56: return ((void*)0);
 case 57: return ((void*)0);
 case 58: return ((void*)0);
 case 59: return ((void*)0);
 case 60: return ((void*)0);
 case 61: return ((void*)0);
 case 62: return ((void*)0);
 case 63: return ((void*)0);
 case 64: return ((void*)0);
 case 65: return ((void*)0);
 case 66: return ((void*)0);
 case 67: return ((void*)0);
 case 68: return ((void*)0);
 case 69: return ((void*)0);
 case 70: return ((void*)0);
 case 71: return ((void*)0);
 case 72: return ((void*)0);
 case 73: return ((void*)0);
 case 74: return ((void*)0);
 case 75: return ((void*)0);
 case 76: return ((void*)0);
 case 77: return ((void*)0);
 case 78: return ((void*)0);
 case 79: return ((void*)0);
 case 80: return ((void*)0);
 case 81: return ((void*)0);
 case 82: return ((void*)0);
 case 83: return ((void*)0);
 case 84: return ((void*)0);
 case 85: return ((void*)0);
 case 86: return ((void*)0);
 case 87: return ((void*)0);
 case 88: return ((void*)0);
 case 89: return ((void*)0);
 case 90: return ((void*)0);
 case 91: return ((void*)0);
 case 92: return ((void*)0);
 case 93: return ((void*)0);
 case 94: return ((void*)0);
 case 95: return ((void*)0);
 case 96: return ((void*)0);
 case 97: return ((void*)0);
 case 98: return ((void*)0);
 case 99: return ((void*)0);
 }
 return 0;
}
int main()
{
  return 0;
}

