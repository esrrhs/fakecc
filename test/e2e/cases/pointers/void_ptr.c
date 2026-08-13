// expect: 99
// `void*` as a parameter type and as a pointee.  f takes a void* and stores
// the int it points to into a global.
package main;
int g;
void read_void_ptr(void *p) {
    int *q;
    q = (int *)p;
    g = *q;
    return;
}
int main() {
    int x = 99;
    read_void_ptr(&x);
    return g;
}
