// expect: 5
// Adjacent string literals concatenate at compile time.
package main;
int main(void) {
    char *s = "hel" "lo";
    int n = 0;
    while (s[n] != 0) n = n + 1;
    return n;
}
