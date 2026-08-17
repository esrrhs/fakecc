// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20021015-1.c
package main;

/* PR opt/7409.  */

char g_list[] = { '1' };

int g (void *p, char *list, int length, char **elementPtr, char **nextPtr)
{
  if (*nextPtr != g_list)
    return 1;

  **nextPtr = 0;

    return 0;}

int main (void)
{
  char *list = g_list;
  char *element;
  int i, length = 100;

  for (i = 0; *list != 0; i++) 
    {
      char *prevList = list;
      g (0, list, length, &element, &list);
      length -= (list - prevList);
    }

  return 0;
}