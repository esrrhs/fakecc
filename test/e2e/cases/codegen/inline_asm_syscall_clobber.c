// expect: 42
/* M1: syscall-like asm with clobbers + rcx→r10 style mov. */
package main;

long fake_syscall3(long nr, long a0, long a1, long a2) {
    long ret;
    __asm__ volatile (
        "nop"
        : "=a"(ret)
        : "a"(nr), "D"(a0), "S"(a1), "d"(a2)
        : "rcx", "r11", "memory");
    return ret;
}

long fake_mmap_style(long flags) {
    long ret;
    __asm__ volatile (
        "mov %%rcx, %%r10\n\t"
        "nop"
        : "=a"(ret)
        : "a"(9L), "c"(flags)
        : "r10", "r11", "memory");
    return ret;
}

int main(void) {
    long r = fake_syscall3(42, 1, 2, 3);
    if (r != 42) return 1;
    r = fake_mmap_style(3);
    if (r != 9) return 2;
    return 42;
}
