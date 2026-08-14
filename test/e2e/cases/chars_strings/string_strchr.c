// strchr: returns a pointer to the first occurrence of c in s, or
// NULL if absent.  c is matched bytewise; searching for \0 finds the
// terminator.  A sign-extension bug on c (char -> int) makes high
// bytes unfindable, so pin 0xEF as well.
// expect: 0
package main;
extern char *strchr(const char *s, int c);
int main() {
    char *s = "hello";
    char *p;

    /* found */
    p = strchr(s, 'e');
    if (p != s + 1) return 1;
    /* first occurrence only */
    p = strchr(s, 'l');
    if (p != s + 2) return 2;
    /* not found */
    p = strchr(s, 'z');
    if (p != 0) return 3;
    /* search for \0 returns the terminator */
    p = strchr(s, 0);
    if (p != s + 5) return 4;
    /* high byte 0xEF */
    char buf[3];
    buf[0] = (char)0xEF; buf[1] = 'a'; buf[2] = 0;
    p = strchr(buf, 0xEF);
    if (p != buf) return 5;
    return 0;
}
