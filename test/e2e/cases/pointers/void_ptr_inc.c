// expect: 0
// GNU C void* arithmetic strides by 1 for both p++ and p+1.
package main;
int main(void) {
    char buf[8];
    void *p = buf;
    p++;
    if (p != (void *)(buf + 1)) return 1;
    p = buf;
    p = p + 1;
    if (p != (void *)(buf + 1)) return 2;
    return 0;
}
