// calloc / realloc: the remaining rt malloc.c entry points not covered
// by the strdup test.  calloc both allocates and zero-fills; realloc
// grows a block and preserves the contents.
// expect: 0
package main;
extern void *calloc(long n, long m);
extern void *realloc(void *p, long n);
extern void *memset(void *d, int c, long n);
extern void free(void *p);

int main() {
    int *p;
    int i;
    char *q;

    /* calloc: 8 ints, all zero */
    p = (int *)calloc(8, 4);
    if (p == 0) return 1;
    i = 0;
    while (i < 8) {
        if (p[i] != 0) { free(p); return 2; }
        i = i + 1;
    }
    /* write through it */
    i = 0;
    while (i < 8) { p[i] = i + 1; i = i + 1; }

    /* realloc: grow from 8 to 16 ints, first 8 preserved */
    p = (int *)realloc(p, 16 * 4);
    if (p == 0) return 3;
    i = 0;
    while (i < 8) {
        if (p[i] != i + 1) { free(p); return 4; }
        i = i + 1;
    }
    free(p);

    /* realloc(NULL, n) acts like malloc */
    q = (char *)realloc(0, 10);
    if (q == 0) return 5;
    q[0] = 'Z';
    if (q[0] != 'Z') { free(q); return 6; }
    free(q);

    return 0;
}
