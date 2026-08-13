// expect: 6
package main;
int main() {
    /* &=, |=, ^= */
    int x = 5;    /* 0b101 */
    x &= 3;       /* 0b011 -> 0b001 = 1 */
    x |= 6;       /* 0b110 -> 0b111 = 7 */
    x ^= 1;       /* 0b001 -> 0b110 = 6 */
    return x;
}
