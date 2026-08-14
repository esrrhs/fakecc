// runtime.strstr: returns a pointer to the first occurrence of needle in
// hay, or NULL if absent.  An empty needle returns hay.  Must not
// read past a \0 in either string.  Pin a mid-string match, a prefix
// match, not-found, empty needle, needle longer than hay, and a match
// at the very last byte.
// expect: 0
package main;
import runtime;
int main() {
    char *h = "foobar";
    char *p;

    /* mid-string match */
    p = runtime.strstr(h, "bar");
    if (p != h + 3) return 1;
    /* prefix match */
    p = runtime.strstr(h, "foo");
    if (p != h) return 2;
    /* not found */
    p = runtime.strstr(h, "baz");
    if (p != 0) return 3;
    /* empty needle returns hay */
    p = runtime.strstr(h, "");
    if (p != h) return 4;
    /* needle longer than hay */
    p = runtime.strstr("hi", "hijkl");
    if (p != 0) return 5;
    /* match at the very last byte */
    p = runtime.strstr(h, "r");
    if (p != h + 5) return 6;
    return 0;
}
