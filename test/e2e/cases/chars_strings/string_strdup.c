// strdup: malloc(s+1) and copy s into it, returning the new buffer
// (or NULL if malloc fails).  The copy must include the \0, and the
// result must be a distinct, writable buffer.  Depends on rt/ malloc
// and memcpy, so a failure here implicates those too.
// expect: 0
package main;
extern char *strdup(const char *s);
extern unsigned long strlen(const char *s);
extern int strcmp(const char *a, const char *b);
extern void free(void *p);
int main() {
    char *p = strdup("hello");
    if (p == 0) return 1;
    if (strcmp(p, "hello") != 0) return 2;
    if (strlen(p) != 5) return 3;
    /* write through the copy to confirm it owns writable memory */
    p[0] = 'H';
    if (p[0] != 'H') return 4;
    free(p);
    return 0;
}
