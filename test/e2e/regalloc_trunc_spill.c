// expect: 31
// A narrowing cast (`int b = (int)bi`) lowers to IR_TRUNC.  When register
// pressure forces the result to spill, codegen used to read the source via
// the pre-regalloc "one stack slot per SSA value" layout, which addresses
// memory outside the frame — so `b` picked up garbage.
//
// The pressure comes from the inner loop and its branch; without them `b`
// stays in a register and the bug is invisible.  This is the shape that
// broke domtree_build during self-hosting.
package main;
int main() {
    int idom[4];
    char proc[4];
    int preds[8];
    for (int i = 0; i < 4; i++) { idom[i] = -1; proc[i] = 0; }
    for (int i = 0; i < 8; i++) preds[i] = 0;
    proc[0] = 1;
    for (unsigned long bi = 0; bi < 4; bi++) {
        int b = (int)bi;
        int new_idom = -1;
        for (int i = 0; i < 2; i++) {
            int p = preds[bi * 2 + i];
            if (proc[p]) { new_idom = p; break; }
        }
        if (idom[b] != new_idom) {
            idom[b] = new_idom;
        }
    }
    return (idom[1] + 1) * 25 + (idom[2] + 1) * 5 + (idom[3] + 1);
}
