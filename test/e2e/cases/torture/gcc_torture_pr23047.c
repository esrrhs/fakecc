// expect: 0
package main;

extern void abort(void);
extern void exit(int);

void f(int i)
{
  i = i > 0 ? i : -i;
  if (i<0)
    return;
  abort ();
}

int main(int argc, char *argv[])
{
  f(-2147483648);
  exit (0);
}
