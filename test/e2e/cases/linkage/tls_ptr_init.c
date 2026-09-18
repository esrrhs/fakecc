// expect: 0
package main;

/* Pointer initializer in .tdata must survive as a R_X86_64_64 reloc
 * (written to .rela.tdata, not dropped into .rela.data). */
__thread const char *msg = "ok";
__thread int *slot;

int main() {
    static int local = 7;
    if (msg[0] != 'o') return 1;
    if (msg[1] != 'k') return 2;
    if (msg[2] != 0) return 3;
    slot = &local;
    if (*slot != 7) return 4;
    *slot = 9;
    if (*slot != 9) return 5;
    return 0;
}
