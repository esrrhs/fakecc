package main;
// A struct local lives in memory, its size reaches the debugger, and a pointer
// to it compares equal to its address.  Naming individual members is a known
// gap (no DW_TAG_member DIEs, and the struct type DIE is unnamed), so this case
// pins down what does work rather than the shape we would eventually like.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: print sizeof(p)
// gdb: print q == &p
// gdb: print *(int *)q
// gdb: continue
// gdb_expect: \$1 = 8
// gdb_expect: \$2 = 1
// gdb_expect: \$3 = 8
struct P { int x; int y; };
int main(void) {
    struct P p;
    struct P *q;
    p.x = 8;
    p.y = 34;
    q = &p;
    return q->x + q->y;   // BRK
}
