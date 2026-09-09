// expect: 0
package main;

extern void abort(void);

char c = 1;
__attribute__((aligned (sizeof (unsigned long long)))) unsigned long long ll;

int main ()
{
  unsigned long long x = __sync_add_and_fetch (&ll, c + 0xfedcba9876543210ULL);
  if (x != 0xfedcba9876543211ULL)
    abort ();
  return 0;
}
