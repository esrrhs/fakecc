// expect: 0
package main;
typedef int v4si __attribute__((vector_size(16)));
short t1(short a, short b) { return (~a | b) ^ a; }
unsigned int t2(unsigned int a, unsigned int b) { return (b | ~a) ^ a; }
signed char t3(signed char a, signed char b) { return a ^ (~a | b); }
unsigned char t4(unsigned char a, unsigned char b) { return a ^ (b | ~a); }
short t5(short a, short b) { return (~b | a) ^ b; }
unsigned short t6(unsigned short a, unsigned short b) { return (a | ~b) ^ b; }
long t7(long a, long b) { return b ^ (~b | a); }
unsigned long t8(unsigned long a, unsigned long b) { return b ^ (a | ~b); }
long long t9(long long a, long long b) { return (~a | b) ^ a; }
unsigned long long t10(unsigned long long a, unsigned long long b) { return (b | ~a) ^ a; }
short t11(short a, short b) { return a ^ (~a | b); }
unsigned int t12(unsigned int a, unsigned int b) { return a ^ (b | ~a); }
signed char t13(signed char a, signed char b) { return (~b | a) ^ b; }
unsigned char t14(unsigned char a, unsigned char b) { return (a | ~b) ^ b; }
short t15(short a, short b) { return b ^ (~b | a); }
unsigned short t16(unsigned short a, unsigned short b) { return b ^ (a | ~b); }
int t17(int a, int b) { return (~a | b) ^ a; }
unsigned long t18(unsigned long a, unsigned long b) { return (b | ~a) ^ a; }
long long t19(long long a, long long b) { return a ^ (~a | b); }
unsigned long long t20(unsigned long long a, unsigned long long b) { return a ^ (b | ~a); }
v4si t21(v4si a, v4si b) { return (~a | b) ^ a; }
v4si t22(v4si a, v4si b) { return a ^ (b | ~a); }
int
main ()
{
  if (t1 (29789, 29477) != -28678) __builtin_abort ();
  if (t2 (20196, -18743) != 4294965567) __builtin_abort ();
  if (t3 (127, 99) != -100) __builtin_abort ();
  if (t4 (100, 53) != 219) __builtin_abort ();
  if (t5 (20100, 1283) != -1025) __builtin_abort ();
  if (t6 (20100, 10283) != 63487) __builtin_abort ();
  if (t7 (2136614690L, 1136698390L) != -1128276995L) __builtin_abort ();
  if (t8 (1136698390L, 2136614690L) != -1128276995UL) __builtin_abort ();
  if (t9 (9176690219839792930LL, 3176690219839721234LL) != -3175044472123688707LL)
    __builtin_abort ();
  if (t10 (9176690219839792930LL, 3176690219839721234LL) != 15271699601585862909ULL)
    __builtin_abort ();
  if (t11 (29789, 29477) != -28678) __builtin_abort ();
  if (t12 (20196, -18743) != 4294965567) __builtin_abort ();
  if (t13 (127, 99) != -100) __builtin_abort ();
  if (t14 (100, 53) != 219) __builtin_abort ();
  if (t15 (20100, 1283) != -1025) __builtin_abort ();
  if (t16 (20100, 10283) != 63487) __builtin_abort ();
  if (t17 (2136614690, 1136698390) != -1128276995) __builtin_abort ();
  if (t18 (1136698390L, 2136614690L) != -1128276995UL) __builtin_abort ();
  if (t19 (9176690219839792930LL, 3176690219839721234LL) != -3175044472123688707LL)
    __builtin_abort ();
  if (t20 (9176690219839792930LL, 3176690219839721234LL) != 15271699601585862909ULL)
    __builtin_abort ();
  v4si a1 = {1, 2, 3, 4};
  v4si a2 = {6, 7, 8, 9};
  v4si r1 = {-1, -3, -1, -1};
  v4si b1 = t21 (a1, a2);
  v4si b2 = t22 (a1, a2);
  if (__builtin_memcmp (&b1, &r1, sizeof (b1) != 0)) __builtin_abort();
  if (__builtin_memcmp (&b2, &r1, sizeof (b2) != 0)) __builtin_abort();
  return 0;
}
