// runtime.strdup: runtime.malloc(s+1) and copy s into it, returning the new buffer
// (or NULL if runtime.malloc fails).  The copy must include the \0, and the
// result must be a distinct, writable buffer.  Depends on rt/ runtime.malloc
// and runtime.memcpy, so a failure here implicates those too.
// expect: 0
package main;
import runtime;
int main() {
    char *p = runtime.strdup("hello");
    if (p == 0) return 1;
    if (runtime.strcmp(p, "hello") != 0) return 2;
    if (runtime.strlen(p) != 5) return 3;
    /* write through the copy to confirm it owns writable memory */
    p[0] = 'H';
    if (p[0] != 'H') return 4;
    runtime.free(p);
    return 0;
}
