package main;
// Struct locals get a named DW_TAG_structure_type with DW_TAG_member children,
// so gdb can print fields by name (`p.x`) and through a typed pointer (`q->x`).
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: print sizeof(p)
// gdb: print p.x
// gdb: print p.y
// gdb: print q->x
// gdb: print q->y
// gdb: continue
// gdb_expect: \$1 = 8
// gdb_expect: \$2 = 8
// gdb_expect: \$3 = 34
// gdb_expect: \$4 = 8
// gdb_expect: \$5 = 34
struct P { int x; int y; };
int main(void) {
    struct P p;
    struct P *q;
    p.x = 8;
    p.y = 34;
    q = &p;
    return q->x + q->y;   // BRK
}
