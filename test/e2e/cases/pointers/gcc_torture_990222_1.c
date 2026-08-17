// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/990222-1.c
package main;

char line[4] = { '1', '9', '9', '\0' };

int main()
{
  char *ptr = line + 3;

  while ((*--ptr += 1) > '9') *ptr = '0';
  if (line[0] != '2' || line[1] != '0' || line[2] != '0')
    return 1;
  return 0;
}