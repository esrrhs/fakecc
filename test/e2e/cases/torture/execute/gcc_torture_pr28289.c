// expect: 0
package main;

extern void exit(int);
int ok(int);

static int gen_x86_64_shrd(int a __attribute__((__unused__)))
{
  return 0;
}

void ix86_split_ashr(int mode)
{
  (mode != 0 ? ok : gen_x86_64_shrd)(0);
}

volatile int one = 1;

int ok(int i)
{
  exit(i);
  return i;
}

int main(void)
{
  ix86_split_ashr(one);
  return 1;
}
