// expect: 0
package main;

/* Mixed member + index: `&((T){...}).arr[i]` and `&((struct { int n; int a[4]; }){...}).a[2]`. */
struct V { int n; int a[4]; };

static int *p = &((struct V){ 5, { 10, 20, 30, 40 } }).a[2];
static struct V *pv = &((struct V){ 7, { 1, 2, 3, 4 } });

int main(void) {
    if (*p != 30) return 1;
    if (pv->n != 7) return 2;
    if (pv->a[0] != 1) return 3;
    if (pv->a[3] != 4) return 4;
    return 0;
}
