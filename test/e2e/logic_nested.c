// expect: 1
package main;
int main() {
    /* && binds tighter than ||: 0 || (1 && 1) => 0 || 1 => 1 */
    return (0 || (1 && 1));
}
