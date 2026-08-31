// expect: 0
package main;

typedef unsigned long ul;
typedef unsigned char uc;

struct S { int a; int b; int c; };
struct Nested { struct S s; int d; };

/* offsetof as array dimension */
char arr_b[(ul)((uc *)&((struct S *)0)->b - (uc *)0)];  /* size 4 */
char arr_c[(ul)((uc *)&((struct S *)0)->c - (uc *)0)];  /* size 8 */

/* offsetof with typedef'd struct pointer */
typedef struct S S_t;
ul g_typedef_offset = (ul)((uc *)&((S_t *)0)->c - (uc *)0);

int main(void) {
    if (sizeof(arr_b) != 4) return 1;
    if (sizeof(arr_c) != 8) return 2;
    if (g_typedef_offset != 8) return 3;
    return 0;
}
