// expect: 7
/* M2: naked function — no push %rbp; whole body is asm. */
package main;

static int g;

extern void target_land(void);

__attribute__((naked)) void stub_jump(void) {
    __asm__ volatile (
        "jmp target_land"
    );
}

void target_land(void) {
    g = 7;
}

int main(void) {
    g = 0;
    stub_jump();
    if (g != 7) return 1;
    return 7;
}
