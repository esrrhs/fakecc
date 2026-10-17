// The linked runtime owns the allocator, and the arm64 backend must not
// shadow it with the inline fallback that maps one fresh page per
// allocation: a self-hosted compile then needed 13 GB where 0.4 GB is
// enough.  A page per allocation puts 16 KB between two blocks that are
// live at the same time and never hands a freed block out again, so both
// are checked here.
// skip_difftest
// expect: 0
package main;
import runtime;
int main(void) {
    void *a;
    void *b;
    void *c;
    long d;
    a = runtime.malloc(64);
    b = runtime.malloc(64);
    if (a == 0) return 1;
    if (b == 0) return 1;
    d = (long)((char *)b - (char *)a);
    if (d < 0) d = 0 - d;
    if (d > 4096) return 2;
    runtime.free(b);
    runtime.free(a);
    c = runtime.malloc(64);
    if (c == 0) return 3;
    if (c != a) return 4;
    runtime.free(c);
    return 0;
}
