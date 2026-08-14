// runtime.strrchr: returns a pointer to the last occurrence of c in s, or
// NULL if absent.  Same byte-matching rules as runtime.strchr.  Pin the last
// of several occurrences, a singleton, not-found, and the \0 search.
// expect: 0
package main;
import runtime;
int main() {
    char *s = "hello";
    char *p;

    /* last occurrence */
    p = runtime.strrchr(s, 'l');
    if (p != s + 3) return 1;
    /* singleton */
    p = runtime.strrchr(s, 'o');
    if (p != s + 4) return 2;
    /* not found */
    p = runtime.strrchr(s, 'z');
    if (p != 0) return 3;
    /* search for \0 returns the terminator */
    p = runtime.strrchr(s, 0);
    if (p != s + 5) return 4;
    return 0;
}
