// expect: 7
package main;
int main() {
    int x = 0;
    x = 3;
    goto skip;
    x = 100;   /* skipped */
skip:
    x = x + 4; /* 3 + 4 = 7 */
    return x;
}
