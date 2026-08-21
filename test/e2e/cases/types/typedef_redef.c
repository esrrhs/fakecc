// expect: 7
/* C11 / GCC: a typedef may be restated only if it denotes the same type. */
package main;
typedef int T;
typedef int T;
int main() {
    T x = 7;
    return x;
}
