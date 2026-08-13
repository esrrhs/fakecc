// expect: 209
// String literal — read individual chars.  'h'=104, 'i'=105, sum=209.
package main;
int main() {
    char *s = "hi";
    return s[0] + s[1];
}
