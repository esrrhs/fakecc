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
typedef unsigned long long ull;
volatile int gvol[32];
ull gull;
ull  t1 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += -2048; } return x; } ull  t2 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += -513; } return x; } ull  t3 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += -512; } return x; } ull  t4 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += -511; } return x; } ull  t5 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += -1; } return x; } ull  t6 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += 1; } return x; } ull  t7 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += 511; } return x; } ull  t8 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += 512; } return x; } ull  t9 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += 513; } return x; } ull  t10 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += gull; } return x; } ull  t11 (int n, ull x) { while (n--) { int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30; x1 = gvol[1], x2 = gvol[2], x3 = gvol[3], x4 = gvol[4], x5 = gvol[5], x6 = gvol[6], x7 = gvol[7], x8 = gvol[8], x9 = gvol[9], x10 = gvol[10], x11 = gvol[11], x12 = gvol[12], x13 = gvol[13], x14 = gvol[14], x15 = gvol[15], x16 = gvol[16], x17 = gvol[17], x18 = gvol[18], x19 = gvol[19], x20 = gvol[20], x21 = gvol[21], x22 = gvol[22], x23 = gvol[23], x24 = gvol[24], x25 = gvol[25], x26 = gvol[26], x27 = gvol[27], x28 = gvol[28], x29 = gvol[29], x30 = gvol[30]; gvol[1] = x1, gvol[2] = x2, gvol[3] = x3, gvol[4] = x4, gvol[5] = x5, gvol[6] = x6, gvol[7] = x7, gvol[8] = x8, gvol[9] = x9, gvol[10] = x10, gvol[11] = x11, gvol[12] = x12, gvol[13] = x13, gvol[14] = x14, gvol[15] = x15, gvol[16] = x16, gvol[17] = x17, gvol[18] = x18, gvol[19] = x19, gvol[20] = x20, gvol[21] = x21, gvol[22] = x22, gvol[23] = x23, gvol[24] = x24, gvol[25] = x25, gvol[26] = x26, gvol[27] = x27, gvol[28] = x28, gvol[29] = x29, gvol[30] = x30; x += -gull; } return x; }
ull neg(ull x) { return -x; }
int main()
{
  gull = 100;
  if (t1 (3, ~0ULL) != -2048 * 3 - 1) abort (); if (t1 (3, 0xffffffffULL) != -2048 * 3 + 0xffffffffULL) abort (); if (t2 (3, ~0ULL) != -513 * 3 - 1) abort (); if (t2 (3, 0xffffffffULL) != -513 * 3 + 0xffffffffULL) abort (); if (t3 (3, ~0ULL) != -512 * 3 - 1) abort (); if (t3 (3, 0xffffffffULL) != -512 * 3 + 0xffffffffULL) abort (); if (t4 (3, ~0ULL) != -511 * 3 - 1) abort (); if (t4 (3, 0xffffffffULL) != -511 * 3 + 0xffffffffULL) abort (); if (t5 (3, ~0ULL) != -1 * 3 - 1) abort (); if (t5 (3, 0xffffffffULL) != -1 * 3 + 0xffffffffULL) abort (); if (t6 (3, ~0ULL) != 1 * 3 - 1) abort (); if (t6 (3, 0xffffffffULL) != 1 * 3 + 0xffffffffULL) abort (); if (t7 (3, ~0ULL) != 511 * 3 - 1) abort (); if (t7 (3, 0xffffffffULL) != 511 * 3 + 0xffffffffULL) abort (); if (t8 (3, ~0ULL) != 512 * 3 - 1) abort (); if (t8 (3, 0xffffffffULL) != 512 * 3 + 0xffffffffULL) abort (); if (t9 (3, ~0ULL) != 513 * 3 - 1) abort (); if (t9 (3, 0xffffffffULL) != 513 * 3 + 0xffffffffULL) abort (); if (t10 (3, ~0ULL) != gull * 3 - 1) abort (); if (t10 (3, 0xffffffffULL) != gull * 3 + 0xffffffffULL) abort (); if (t11 (3, ~0ULL) != -gull * 3 - 1) abort (); if (t11 (3, 0xffffffffULL) != -gull * 3 + 0xffffffffULL) abort ();
  if (neg (gull) != -100ULL)
    abort ();
  exit (0);
}

