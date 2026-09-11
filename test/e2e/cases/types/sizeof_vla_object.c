// expect: 0
// sizeof of a VLA object is the size captured at the declaration, even if
// the bound variable later changes.  sizeof of a VLA type still re-evaluates.
package main;
int main(void) {
    int n = 5;
    int a[n];
    n = 10;
    if ((int)sizeof a != 20) return 1;
    if ((int)sizeof(int[n]) != 40) return 2;
    return 0;
}
