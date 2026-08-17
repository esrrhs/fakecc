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


extern void exit (int);
extern void abort (void);
typedef unsigned int u_int32_t;
typedef unsigned char u_int8_t;
typedef int int32_t;
typedef enum {
        TXNLIST_DELETE,
        TXNLIST_LSN,
        TXNLIST_TXNID,
        TXNLIST_PGNO
} db_txnlist_type;
struct __db_lsn; typedef struct __db_lsn DB_LSN;
struct __db_lsn {
        u_int32_t file;
        u_int32_t offset;
};
struct __db_txnlist; typedef struct __db_txnlist DB_TXNLIST;
struct __db_txnlist {
        db_txnlist_type type;
        struct { struct __db_txnlist *le_next; struct __db_txnlist **le_prev; } links;
        union {
                struct {
                        u_int32_t txnid;
                        int32_t generation;
                        int32_t aborted;
                } t;
                struct {
                        u_int32_t flags;
                        int32_t fileid;
                        u_int32_t count;
                        char *fname;
                } d;
                struct {
                        int32_t ntxns;
                        int32_t maxn;
                        DB_LSN *lsn_array;
                } l;
                struct {
                        int32_t nentries;
                        int32_t maxentry;
                        char *fname;
                        int32_t fileid;
                        void *pgno_array;
                        u_int8_t uid[20];
                } p;
        } u;
};
int log_compare(const DB_LSN *a, const DB_LSN *b)
{
  return 1;
}
int __db_txnlist_lsnadd(int val, DB_TXNLIST *elp, DB_LSN *lsnp, u_int32_t flags)
{
   int i;
   for (i = 0; i < (!(flags & (0x1)) ? 1 : elp->u.l.ntxns); i++)
     {
 int __j;
 DB_LSN __tmp;
 val++;
 for (__j = 0; __j < elp->u.l.ntxns - 1; __j++)
   if (log_compare(&elp->u.l.lsn_array[__j], &elp->u.l.lsn_array[__j + 1]) < 0)
   {
      __tmp = elp->u.l.lsn_array[__j];
      elp->u.l.lsn_array[__j] = elp->u.l.lsn_array[__j + 1];
      elp->u.l.lsn_array[__j + 1] = __tmp;
   }
     }
   *lsnp = elp->u.l.lsn_array[0];
   return val;
}
int main(void)
{
  DB_TXNLIST el;
  DB_LSN lsn, lsn_a[1235];
  el.u.l.ntxns = 1235 -1;
  el.u.l.lsn_array = lsn_a;
  if (__db_txnlist_lsnadd (0, &el, &lsn, 0) != 1)
    abort ();
  if (__db_txnlist_lsnadd (0, &el, &lsn, 1) != 1235 -1)
    abort ();
  exit (0);
}

