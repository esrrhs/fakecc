/* Minimal ISR stub + address table for fakeos desc.asm migration (S3).
 * Shows .macro / .rept for stubs and .quad symbol addresses into .data.
 * Not a full desc.asm port (no isr_common / error-code variants / vec 255).
 * Assemble: fakecc -c docs/examples/desc_stubs.S -o desc_stubs.o
 */
    .text
    .globl isr_stub_table, isr_common

    .macro ISR_NOERR n
    .globl isr_stub_\n
isr_stub_\n:
    push $0
    push $\n
    jmp isr_common
    .endm

    ISR_NOERR 0
    ISR_NOERR 1
    ISR_NOERR 2
    ISR_NOERR 3

    .align 16
isr_common:
    /* placeholder — real path saves GPRs and calls C dispatcher */
    pop %rax
    pop %rax
    iretq

    .data
    .align 8
isr_stub_table:
    .quad isr_stub_0
    .quad isr_stub_1
    .quad isr_stub_2
    .quad isr_stub_3
