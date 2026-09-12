// expect: 0
// C99 6.7.8p14: a brace-wrapped string initializes a char array.  `char s[] =
// { "hi" }` infers length 3 (including NUL); `char t[4] = { "hi" }` copies the
// bytes and zero-fills the rest.
package main;
int main(void) {
    char s[] = { "hi" };
    if (sizeof(s) != 3) return 1;
    if (s[0] != 'h') return 2;
    if (s[1] != 'i') return 3;
    if (s[2] != 0) return 4;
    char t[4] = { "hi" };
    if (t[0] != 'h') return 5;
    if (t[1] != 'i') return 6;
    if (t[2] != 0) return 7;
    if (t[3] != 0) return 8;
    char u[] = "hi";
    if (sizeof(u) != 3) return 9;
    if (u[0] != 'h' || u[1] != 'i' || u[2] != 0) return 10;
    return 0;
}
