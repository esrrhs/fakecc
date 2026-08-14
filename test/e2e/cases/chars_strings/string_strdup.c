// str.strdup: mem.malloc(s+1) and copy s into it, returning the new buffer
// (or NULL if mem.malloc fails).  The copy must include the \0, and the
// result must be a distinct, writable buffer.  Depends on rt/ mem.malloc
// and str.memcpy, so a failure here implicates those too.
// expect: 0
package main;
import str;
import mem;
int main() {
    char *p = str.strdup("hello");
    if (p == 0) return 1;
    if (str.strcmp(p, "hello") != 0) return 2;
    if (str.strlen(p) != 5) return 3;
    /* write through the copy to confirm it owns writable memory */
    p[0] = 'H';
    if (p[0] != 'H') return 4;
    mem.free(p);
    return 0;
}
