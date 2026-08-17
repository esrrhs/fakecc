// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20030203-1.c
package main;

void f(int);
int do_layer3(int single)
{
  int stereo1;

  if(single >= 0) /* stream is stereo, but force to mono */
    stereo1 = 1;
  else
    stereo1 = 2;
  f(single);

  return stereo1;
}

int main()
{
  if (do_layer3(-1) != 2)
    return 1;
  return 0;
}

void f(int i) {}