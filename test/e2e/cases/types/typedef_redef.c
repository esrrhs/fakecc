// expect: 14
/* C11 / GCC: a typedef may be restated if the type is compatible.
 * Incomplete array + complete array composes to the complete type. */
package main;
typedef int T;
typedef int T;
typedef int A[];
typedef int A[3];
int main() {
    T x = 7;
    A a;
    a[0] = 1;
    a[1] = 2;
    a[2] = 4;
    return x + a[0] + a[1] + a[2];
}
