// expect: 1
package main;
int main() {
    /* Verify both prefix and postfix --, and that the side effect sticks. */
    int i = 3;
    int a = --i;   /* a = 2, i = 2 */
    int b = i--;   /* b = 2, i = 1 */
    return a - b + i;  /* 2 - 2 + 1 = 1 */
}
