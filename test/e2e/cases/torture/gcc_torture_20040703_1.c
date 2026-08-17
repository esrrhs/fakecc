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
typedef unsigned int cpp_num_part;
typedef struct cpp_num cpp_num;
struct cpp_num
{
   cpp_num_part high;
   cpp_num_part low;
   int unsignedp;
   int overflow;
};
static int num_positive(cpp_num num, unsigned int precision)
{
   if (precision > (sizeof (cpp_num_part) * 8))
     {
       precision -= (sizeof (cpp_num_part) * 8);
       return (num.high & (cpp_num_part) 1 << (precision - 1)) == 0;
     }
   
return (num.low & (cpp_num_part) 1 << (precision - 1)) == 0;
}
static cpp_num num_trim(cpp_num num, unsigned int precision)
{
   if (precision > (sizeof (cpp_num_part) * 8))
     {
       precision -= (sizeof (cpp_num_part) * 8);
       if (precision < (sizeof (cpp_num_part) * 8))
         num.high &= ((cpp_num_part) 1 << precision) - 1;
     }
   else
     {
       if (precision < (sizeof (cpp_num_part) * 8))
         num.low &= ((cpp_num_part) 1 << precision) - 1;
       num.high = 0;
     }
   return num;
}
static cpp_num num_rshift(cpp_num num, unsigned int precision, unsigned int n)
{
   cpp_num_part sign_mask;
   int x = num_positive (num, precision);
   if (num.unsignedp || x)
     sign_mask = 0;
   else
     sign_mask = ~(cpp_num_part) 0;
   if (n >= precision)
     num.high = num.low = sign_mask;
   else
     {
       if (precision < (sizeof (cpp_num_part) * 8))
         num.high = sign_mask, num.low |= sign_mask << precision;
       else if (precision < 2 * (sizeof (cpp_num_part) * 8))
         num.high |= sign_mask << (precision - (sizeof (cpp_num_part) * 8));
       if (n >= (sizeof (cpp_num_part) * 8))
         {
           n -= (sizeof (cpp_num_part) * 8);
           num.low = num.high;
           num.high = sign_mask;
         }
       
if (n)
         {
           num.low = (num.low >> n) | (num.high << ((sizeof (cpp_num_part) * 8) - n));
           num.high = (num.high >> n) | (sign_mask << ((sizeof (cpp_num_part) * 8) - n));
         }
     }
   num = num_trim (num, precision);
   num.overflow = 0;
   return num;
}
cpp_num
num_lshift (cpp_num num, unsigned int precision, unsigned int n)
{
   if (n >= precision)
     {
       num.overflow = !num.unsignedp && !((num.low | num.high) == 0);
       num.high = num.low = 0;
     }
   else
     {
       cpp_num orig;
       unsigned int m = n;
       orig = num;
       if (m >= (sizeof (cpp_num_part) * 8))
         {
           m -= (sizeof (cpp_num_part) * 8);
           num.high = num.low;
           num.low = 0;
         }
       
if (m)
         {
           num.high = (num.high << m) | (num.low >> ((sizeof (cpp_num_part) * 8) - m));
           num.low <<= m;
         }
       num = num_trim (num, precision);
       if (num.unsignedp)
         num.overflow = 0;
       else
         {
           cpp_num maybe_orig = num_rshift (num, precision, n);
           num.overflow = !(orig.low == maybe_orig.low && orig.high == maybe_orig.high);
         }
     }
   return num;
}
unsigned int precision = 64;
unsigned int n = 16;
cpp_num num = { 0, 3, 0, 0 };
int main()
{
   cpp_num res = num_lshift (num, 64, n);
   if (res.low != 0x30000)
     abort ();
   if (res.high != 0)
     abort ();
   if (res.overflow != 0)
     abort ();
   exit (0);
}

