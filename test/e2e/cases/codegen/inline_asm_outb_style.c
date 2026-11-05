// unsupported_on: darwin-arm64
// expect: 0
/* M1: outb-style constrained asm must emit OUT; mov path checks storeback. */
package main;

void outb_like(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

int main(void) {
    unsigned int x = 0x5a;
    unsigned int y = 0;
    __asm__ volatile ("movl %1, %0" : "=a"(y) : "a"(x));
    if (y != 0x5a) return 1;
    if ((unsigned long)outb_like == 0) return 2;
    return 0;
}
