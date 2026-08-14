// str.strchr: returns a pointer to the first occurrence of c in s, or
// NULL if absent.  c is matched bytewise; searching for \0 finds the
// terminator.  A sign-extension bug on c (char -> int) makes high
// bytes unfindable, so pin 0xEF as well.
// expect: 0
package main;
import str;
int main() {
    char *s = "hello";
    char *p;

    /* found */
    p = str.strchr(s, 'e');
    if (p != s + 1) return 1;
    /* first occurrence only */
    p = str.strchr(s, 'l');
    if (p != s + 2) return 2;
    /* not found */
    p = str.strchr(s, 'z');
    if (p != 0) return 3;
    /* search for \0 returns the terminator */
    p = str.strchr(s, 0);
    if (p != s + 5) return 4;
    /* high byte 0xEF */
    char buf[3];
    buf[0] = (char)0xEF; buf[1] = 'a'; buf[2] = 0;
    p = str.strchr(buf, 0xEF);
    if (p != buf) return 5;
    return 0;
}
