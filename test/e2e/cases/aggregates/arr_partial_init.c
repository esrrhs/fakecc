// expect: 3
// Partial array init: int a[5] = {1,2}; the rest are zero.  Return a[0]+a[1]+a[4].
package main;
int main() {
    int a[5] = {1, 2};
    return a[0] + a[1] + a[4];
}
