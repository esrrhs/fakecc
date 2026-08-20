// link: -lc
// expect: 0
package main;

typedef long jmp_buf[32];
extern int setjmp(jmp_buf);
extern void longjmp(jmp_buf, int);

static jmp_buf buf;
static _Bool stop = 0;

void call_func(void (*func)(void))
{
  func();
}

void func(void)
{
  stop = 1;
  longjmp(buf, 1);
}

int main(void)
{
  setjmp(buf);

  while (!stop)
    call_func(func);

  return 0;
}
