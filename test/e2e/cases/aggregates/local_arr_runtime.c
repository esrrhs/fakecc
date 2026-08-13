// expect: 9
// Local array initialized with non-constant element expressions (computed
// at runtime).  a[0]=2*1+1=3, a[1]=x*2+1=5 (x=2), a[2]=1; sum all three.
package main;
int main() {
    int x = 2;
    int a[3] = {2 * 1 + 1, x * 2 + 1, 1};
    return a[0] + a[1] + a[2];
}
