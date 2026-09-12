// expect: 0
// The comma operator decays an array operand to a pointer, so
// sizeof (0, a) is sizeof(int *), not sizeof(int[10]).
package main;
int main(void) {
    int a[10];
    if (sizeof (0, a) != sizeof(int *)) return 1;
    if (sizeof a != 10 * sizeof(int)) return 2;
    return 0;
}
