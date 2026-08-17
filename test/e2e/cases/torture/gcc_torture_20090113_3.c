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


struct obstack {};
struct bitmap_head_def;
typedef struct bitmap_head_def *bitmap;
typedef const struct bitmap_head_def *const_bitmap;
typedef unsigned long BITMAP_WORD;
typedef struct bitmap_obstack
{
  struct bitmap_element_def *elements;
  struct bitmap_head_def *heads;
  struct obstack obstack;
} bitmap_obstack;
typedef struct bitmap_element_def
{
  struct bitmap_element_def *next;
  struct bitmap_element_def *prev;
  unsigned int indx;
  BITMAP_WORD bits[(2)];
} bitmap_element;
struct bitmap_descriptor;
typedef struct bitmap_head_def {
    bitmap_element *first;
    bitmap_element *current;
    unsigned int indx;
    bitmap_obstack *obstack;
} bitmap_head;
bitmap_element bitmap_zero_bits;
typedef struct
{
  bitmap_element *elt1;
  bitmap_element *elt2;
  unsigned word_no;
  BITMAP_WORD bits;
} bitmap_iterator;
static  void bmp_iter_set_init (bitmap_iterator *bi, const_bitmap map,
     unsigned start_bit, unsigned *bit_no)
{
  bi->elt1 = map->first;
  bi->elt2 = ((void *)0);
  while (1)
    {
      if (!bi->elt1)
 {
   bi->elt1 = &bitmap_zero_bits;
   break;
 }
      
if (bi->elt1->indx >= start_bit / (128u))
 break;
      bi->elt1 = bi->elt1->next;
    }
  
if (bi->elt1->indx != start_bit / (128u))
    start_bit = bi->elt1->indx * (128u);
  bi->word_no = start_bit / 64u % (2);
  bi->bits = bi->elt1->bits[bi->word_no];
  bi->bits >>= start_bit % 64u;
  start_bit += !bi->bits;
  *bit_no = start_bit;
}
static   void bmp_iter_next (bitmap_iterator *bi, unsigned *bit_no)
{
  bi->bits >>= 1;
  *bit_no += 1;
}
static   unsigned char bmp_iter_set (bitmap_iterator *bi, unsigned *bit_no)
{
  if (bi->bits)
    {
      while (!(bi->bits & 1))
 {
   bi->bits >>= 1;
   *bit_no += 1;
 }
      return 1;
    }
  *bit_no = ((*bit_no + 64u - 1) / 64u * 64u);
  bi->word_no++;
  while (1)
    {
      while (bi->word_no != (2))
 {
   bi->bits = bi->elt1->bits[bi->word_no];
   if (bi->bits)
     {
       while (!(bi->bits & 1))
  {
    bi->bits >>= 1;
    *bit_no += 1;
  }
       return 1;
     }
   *bit_no += 64u;
   bi->word_no++;
 }
      bi->elt1 = bi->elt1->next;
      if (!bi->elt1)
 return 0;
      *bit_no = bi->elt1->indx * (128u);
      bi->word_no = 0;
    }
}
static void  foobar (bitmap_head *live_throughout)
{
  bitmap_iterator rsi;
  unsigned int regno;
  for (bmp_iter_set_init (&(rsi), (live_throughout), (0), &(regno));
       bmp_iter_set (&(rsi), &(regno));
       bmp_iter_next (&(rsi), &(regno)))
    ;
}
int main()
{
  bitmap_element elem = { (void *)0, (void *)0, 0, { 1, 1 } };
  bitmap_head live_throughout = { &elem, &elem, 0, (void *)0 };
  foobar (&live_throughout);
  return 0;
}

