// link: -lc
// expect: 0
// unsupported_on: darwin-arm64
// AArch64 SDIV/UDIV with a zero divisor return 0 instead of trapping, so
// there is no SIGFPE for this handler to catch.  Verified: `i / j` with both
// zero exits with 42 from `return k + 42`, not with SIGFPE.
package main;

extern void abort(void);
extern void exit(int);
extern void (*signal(int sig, void (*func)(int)))(int);

void sigfpe(int signum)
{
  exit(0);
}

static int i;
static int j;
int k;

int main(void)
{
  signal(8, sigfpe);
  k = i / j;
  abort();
}
