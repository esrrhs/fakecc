// expect: 0
// An uninitialized static this large is .bss, not a host-side calloc.
package main;
int main(void) {
    static char buf[20000000];
    return buf[0];
}
