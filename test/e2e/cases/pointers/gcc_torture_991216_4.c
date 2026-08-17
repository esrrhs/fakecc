// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/991216-4.c
package main;

/* Test against a problem with loop reversal.  */

static void bug(int size, int tries)
{
    int i;
    int num = 0;
    while (num < size)
    {
        for (i = 1; i < tries; i++) num++;
    }
}

int main()
{
    bug(5, 10);
    return 0;
}