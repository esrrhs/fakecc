// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20140212-2.c
package main;

/* This used to fail as we would convert f into just return (unsigned int)usVlanID
   which is wrong. */

int f(unsigned short usVlanID) ;
int f(unsigned short usVlanID)
{
  unsigned int uiVlanID = 0xffffffff;
  int i;
  if ((unsigned short)0xffff != usVlanID)
    uiVlanID = (unsigned int)usVlanID;
  return uiVlanID;
}

int main(void)
{
  if (f(1) != 1)
    return 1;
  if (f(0xffff) != -1)
    return 1;
  return 0;
}