// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20110418-1.c
package main;

typedef unsigned long long my_uint64_t;
void f(my_uint64_t *a, my_uint64_t aa) ;
void f(my_uint64_t *a, my_uint64_t aa)
{
  my_uint64_t new_value = aa;
  my_uint64_t old_value = *a;
  int bit_size = 32;
    my_uint64_t mask = (my_uint64_t)(unsigned)(-1);
    my_uint64_t tmp = old_value & mask;
    new_value &= mask;
    /* On overflow we need to add 1 in the upper bits */
    if (tmp > new_value)
        new_value += 1ull<<bit_size;
    /* Add in the upper bits from the old value */
    new_value += old_value & ~mask;
    *a = new_value;
}
int main(void)
{
  my_uint64_t value, new_value, old_value;
  value = 0x100000001;
  old_value = value;
  new_value = (value+1)&(my_uint64_t)(unsigned)(-1);
  f(&value, new_value);
  if (value != old_value+1)
    return 1;
  return 0;
}