// expect: 0
package main;

/* File-scope compound literal: `&(int){...}`.
 * GCC gives it static storage, so its address is a link-time constant and the
 * value must be readable at runtime. */
static int *p = &(int){42};

int main(void) {
    if (*p != 42) return 1;
    return 0;
}
