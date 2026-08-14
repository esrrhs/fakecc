// mem.calloc / mem.realloc: the remaining rt mem.malloc.c entry points not covered
// by the str.strdup test.  mem.calloc both allocates and zero-fills; mem.realloc
// grows a block and preserves the contents.
// expect: 0
package main;

import mem;
int main() {
    int *p;
    int i;
    char *q;

    /* mem.calloc: 8 ints, all zero */
    p = (int *)mem.calloc(8, 4);
    if (p == 0) return 1;
    i = 0;
    while (i < 8) {
        if (p[i] != 0) { mem.free(p); return 2; }
        i = i + 1;
    }
    /* write through it */
    i = 0;
    while (i < 8) { p[i] = i + 1; i = i + 1; }

    /* mem.realloc: grow from 8 to 16 ints, first 8 preserved */
    p = (int *)mem.realloc(p, 16 * 4);
    if (p == 0) return 3;
    i = 0;
    while (i < 8) {
        if (p[i] != i + 1) { mem.free(p); return 4; }
        i = i + 1;
    }
    mem.free(p);

    /* mem.realloc(NULL, n) acts like mem.malloc */
    q = (char *)mem.realloc(0, 10);
    if (q == 0) return 5;
    q[0] = 'Z';
    if (q[0] != 'Z') { mem.free(q); return 6; }
    mem.free(q);

    return 0;
}
