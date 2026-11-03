// expect: 0
/* M1: .byte / sysretq raw opcode path. */
package main;

__attribute__((naked)) void sysret_bytes(void) {
    __asm__ volatile (
        ".byte 0x48, 0x0f, 0x07\n"
    );
}

int main(void) {
    if ((unsigned long)sysret_bytes == 0) return 1;
    return 0;
}
