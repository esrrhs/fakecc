// expect: 0
// A tentative incomplete array completed by a later declarator has that
// length: `int a[]; int a[5];` → sizeof(a) == 20.
package main;
int a[];
int a[5];
int main(void) {
    if ((int)sizeof(a) != 20) return 1;
    return 0;
}
