// expect: 0
package main;

extern void abort(void);

int arr[4] = {1,2,3,4};
int count = 0;

int __attribute__((noinline))
incr (void)
{
  count++;
  return count;
}

int main(void)
{
  arr[count++] = incr ();
  if (count != 2 || arr[count] != 3)
    abort ();
  return 0;
}
