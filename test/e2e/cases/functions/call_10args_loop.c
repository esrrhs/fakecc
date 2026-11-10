// expect: 0
// 10-arg call inside a loop: stack args (i+2, i+3, i+4, i+5) must be
// rematerialized from the live loop-carried i at -O1, not pushed from an
// uninitialized frame slot.
package main;

int sum10(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
    return a + b + c + d + e + f + g + h + i + j;
}

int main(void) {
    int acc = 0;
    int i;
    for (i = 1; i <= 3; i = i + 1) {
        acc = acc + sum10(i, i, i, i, i, i, i + 2, i + 3, i + 4, i + 5);
    }
    /* i=1: 6+3+4+5+6=24; i=2: 12+4+5+6+7=34; i=3: 18+5+6+7+8=44; sum=102 */
    if (acc != 102) return 1;
    return 0;
}
