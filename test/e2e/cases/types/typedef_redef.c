// expect: 7
/* GCC: a typedef may be restated only if it denotes the same type.
 * Incomplete `int[]` and complete `int[3]` are different (GCC rejects). */
package main;
typedef int T;
typedef int T;
int main() {
    T x = 7;
    return x;
}
