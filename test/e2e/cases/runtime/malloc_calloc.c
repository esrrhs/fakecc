// runtime.calloc / runtime.realloc: the remaining rt runtime.malloc.c entry points not covered
// by the runtime.strdup test.  runtime.calloc both allocates and zero-fills; runtime.realloc
// grows a block and preserves the contents.
// expect: 0
package main;

import runtime;
int main() {
    int *p;
    int i;
    char *q;

    /* runtime.calloc: 8 ints, all zero */
    p = (int *)runtime.calloc(8, 4);
    if (p == 0) return 1;
    i = 0;
    while (i < 8) {
        if (p[i] != 0) { runtime.free(p); return 2; }
        i = i + 1;
    }
    /* write through it */
    i = 0;
    while (i < 8) { p[i] = i + 1; i = i + 1; }

    /* runtime.realloc: grow from 8 to 16 ints, first 8 preserved */
    p = (int *)runtime.realloc(p, 16 * 4);
    if (p == 0) return 3;
    i = 0;
    while (i < 8) {
        if (p[i] != i + 1) { runtime.free(p); return 4; }
        i = i + 1;
    }
    runtime.free(p);

    /* runtime.realloc(NULL, n) acts like runtime.malloc */
    q = (char *)runtime.realloc(0, 10);
    if (q == 0) return 5;
    q[0] = 'Z';
    if (q[0] != 'Z') { runtime.free(q); return 6; }
    runtime.free(q);

    return 0;
}
