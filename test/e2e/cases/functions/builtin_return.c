// expect: 0
// GNU __builtin_apply / __builtin_return restore the callee's result regs.
package main;
int add1(int x) { return x + 1; }
int wrap(int x) {
    void *args = __builtin_apply_args();
    __builtin_return(__builtin_apply((void (*)())add1, args, 16));
}
int main(void) {
    if (wrap(41) != 42) return 1;
    return 0;
}
