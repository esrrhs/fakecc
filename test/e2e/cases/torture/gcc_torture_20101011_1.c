// link: -lc
// expect: 0
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
