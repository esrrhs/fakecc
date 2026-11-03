#include "fakecc/emit.h"
#include "fakecc/iasm.h"
#include "fakecc/target.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int has_sym(const EmitModule *m, const char *name) {
    return emit_module_find_symbol((EmitModule *)m, name) >= 0;
}

static void test_assemble_io_subset(void) {
    static const char src[] =
        "    .text\n"
        "    .globl cpu_halt, inb, outb\n"
        "cpu_halt:\n"
        "    hlt\n"
        "    ret\n"
        "inb:\n"
        "    mov %di, %dx\n"
        "    xor %eax, %eax\n"
        "    in %dx, %al\n"
        "    ret\n"
        "outb:\n"
        "    mov %di, %dx\n"
        "    mov %sil, %al\n"
        "    out %al, %dx\n"
        "    ret\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "io.S", &em), 0);
    T_ASSERT(em.text.len > 0);
    T_ASSERT(has_sym(&em, "cpu_halt"));
    T_ASSERT(has_sym(&em, "inb"));
    T_ASSERT(has_sym(&em, "outb"));
    /* hlt = 0xF4 */
    int found_hlt = 0;
    for (size_t i = 0; i < em.text.len; i++)
        if ((unsigned char)em.text.data[i] == 0xF4) found_hlt = 1;
    T_ASSERT(found_hlt);
    emit_module_free(&em);
}

static void test_macro_usysc(void) {
    static const char src[] =
        "    .text\n"
        "    .macro USYSC1 name nr\n"
        "    .globl \\name\n"
        "\\name:\n"
        "    mov $\\nr, %eax\n"
        "    syscall\n"
        "    ret\n"
        "    .endm\n"
        "    USYSC1 u_read, 0\n"
        "    USYSC1 u_write, 1\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "usys.S", &em), 0);
    T_ASSERT(has_sym(&em, "u_read"));
    T_ASSERT(has_sym(&em, "u_write"));
    int found_syscall = 0;
    for (size_t i = 0; i + 1 < em.text.len; i++)
        if ((unsigned char)em.text.data[i] == 0x0F &&
            (unsigned char)em.text.data[i + 1] == 0x05)
            found_syscall = 1;
    T_ASSERT(found_syscall);
    emit_module_free(&em);
}

static void test_priv_and_rip(void) {
    static const char src[] =
        "    .text\n"
        "    .globl load_gdt, read_msr, touch\n"
        "    .extern gdt_ptr\n"
        "    .extern syscall_user_rsp\n"
        "load_gdt:\n"
        "    lgdt (%rdi)\n"
        "    ret\n"
        "read_msr:\n"
        "    rdmsr\n"
        "    ret\n"
        "touch:\n"
        "    lea syscall_user_rsp(%rip), %rax\n"
        "    ret\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "priv.S", &em), 0);
    T_ASSERT(has_sym(&em, "load_gdt"));
    T_ASSERT(has_sym(&em, "read_msr"));
    /* rdmsr = 0F 32 */
    int found = 0;
    for (size_t i = 0; i + 1 < em.text.len; i++)
        if ((unsigned char)em.text.data[i] == 0x0F &&
            (unsigned char)em.text.data[i + 1] == 0x32)
            found = 1;
    T_ASSERT(found);
    T_ASSERT(em.num_relocs >= 1); /* syscall_user_rsp(%rip) */
    emit_module_free(&em);
}

static void test_rept_stubs(void) {
    static const char src[] =
        "    .text\n"
        "    .globl stub0\n"
        "stub0:\n"
        "    .rept 3\n"
        "    nop\n"
        "    .endr\n"
        "    ret\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "rept.S", &em), 0);
    /* 3 nops + ret */
    T_ASSERT(em.text.len >= 4);
    emit_module_free(&em);
}


static void test_incbin_rodata(void) {
    const char *blob = "/tmp/fakecc_incbin_blob.bin";
    const char *asmf = "/tmp/fakecc_incbin.S";
    {
        FILE *f = fopen(blob, "wb");
        T_ASSERT(f != NULL);
        fwrite("HELLO", 1, 5, f);
        fclose(f);
    }
    {
        FILE *f = fopen(asmf, "w");
        T_ASSERT(f != NULL);
        fputs("    .section .rodata\n"
              "    .globl foo_start\n"
              "    .globl foo_end\n"
              "    .align 8\n"
              "foo_start:\n"
              "    .incbin \"/tmp/fakecc_incbin_blob.bin\"\n"
              "foo_end:\n", f);
        fclose(f);
    }
    char *src = NULL;
    {
        FILE *f = fopen(asmf, "r");
        T_ASSERT(f != NULL);
        fseek(f, 0, SEEK_END);
        long n = ftell(f);
        fseek(f, 0, SEEK_SET);
        src = malloc((size_t)n + 1);
        T_ASSERT(src != NULL);
        T_ASSERT_EQ_INT((int)fread(src, 1, (size_t)n, f), (int)n);
        src[n] = 0;
        fclose(f);
    }
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, asmf, &em), 0);
    free(src);
    T_ASSERT(em.rodata.len >= 5);
    T_ASSERT(memcmp(em.rodata.data, "HELLO", 5) == 0);
    T_ASSERT(has_sym(&em, "foo_start"));
    T_ASSERT(has_sym(&em, "foo_end"));
    int si = emit_module_find_symbol(&em, "foo_start");
    int ei = emit_module_find_symbol(&em, "foo_end");
    T_ASSERT(si >= 0 && ei >= 0);
    T_ASSERT(em.syms[ei].value >= em.syms[si].value + 5);
    emit_module_free(&em);
    unlink(blob);
    unlink(asmf);
}


static void test_switch_subset(void) {
    static const char src[] =
        "    .text\n"
        "    .globl switch_context, spin_lock, cpu_irq_save\n"
        "    .extern kthread_exit\n"
        "switch_context:\n"
        "    push %rbp\n"
        "    push %rbx\n"
        "    mov %rsp, (%rdi)\n"
        "    mov %rsi, %rsp\n"
        "    pop %rbx\n"
        "    pop %rbp\n"
        "    ret\n"
        "spin_lock:\n"
        "    mov $1, %eax\n"
        ".retry:\n"
        "    xchg %eax, (%rdi)\n"
        "    test %eax, %eax\n"
        "    jz .acquired\n"
        "    pause\n"
        "    jmp .retry\n"
        ".acquired:\n"
        "    ret\n"
        "cpu_irq_save:\n"
        "    pushfq\n"
        "    pop %rax\n"
        "    cli\n"
        "    ret\n"
        "tramp:\n"
        "    call *%rax\n"
        "    call kthread_exit\n"
        "    ud2\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "switch.S", &em), 0);
    T_ASSERT(has_sym(&em, "switch_context"));
    T_ASSERT(has_sym(&em, "spin_lock"));
    T_ASSERT(has_sym(&em, "cpu_irq_save"));
    /* pushfq=0x9C, pause=F3 90, call *%rax=FF D0, ud2=0F 0B */
    int found_pushfq = 0, found_pause = 0, found_callstar = 0, found_ud2 = 0;
    for (size_t i = 0; i < em.text.len; i++) {
        unsigned char c = (unsigned char)em.text.data[i];
        if (c == 0x9C) found_pushfq = 1;
        if (i + 1 < em.text.len && c == 0xF3 &&
            (unsigned char)em.text.data[i + 1] == 0x90)
            found_pause = 1;
        if (i + 1 < em.text.len && c == 0xFF &&
            (unsigned char)em.text.data[i + 1] == 0xD0)
            found_callstar = 1;
        if (i + 1 < em.text.len && c == 0x0F &&
            (unsigned char)em.text.data[i + 1] == 0x0B)
            found_ud2 = 1;
    }
    T_ASSERT(found_pushfq);
    T_ASSERT(found_pause);
    T_ASSERT(found_callstar);
    T_ASSERT(found_ud2);
    emit_module_free(&em);
}

static void test_movdqa_xmm(void) {
    static const char src[] =
        "    .text\n"
        "    .globl save_x, load_x\n"
        "save_x:\n"
        "    movdqa %xmm0, 0x08(%rsp)\n"
        "    movdqa %xmm8, 0x88(%rsp)\n"
        "    ret\n"
        "load_x:\n"
        "    movdqa 0x08(%rsp), %xmm1\n"
        "    movdqa 0xf8(%rsp), %xmm15\n"
        "    ret\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "movdqa.S", &em), 0);
    T_ASSERT(has_sym(&em, "save_x"));
    T_ASSERT(has_sym(&em, "load_x"));
    /* store xmm0,8(%rsp): 66 0F 7F 44 24 08
     * store xmm8,0x88(%rsp): 66 44 0F 7F 84 24 88 00 00 00
     * load 8(%rsp)->xmm1: 66 0F 6F 4C 24 08
     * load 0xf8(%rsp)->xmm15: 66 44 0F 6F BC 24 F8 00 00 00 */
    int found_store0 = 0, found_store8 = 0, found_load1 = 0, found_load15 = 0;
    const unsigned char *d = (const unsigned char *)em.text.data;
    size_t n = em.text.len;
    for (size_t i = 0; i + 5 < n; i++) {
        if (d[i] == 0x66 && d[i + 1] == 0x0F && d[i + 2] == 0x7F &&
            d[i + 3] == 0x44 && d[i + 4] == 0x24 && d[i + 5] == 0x08)
            found_store0 = 1;
        if (d[i] == 0x66 && d[i + 1] == 0x0F && d[i + 2] == 0x6F &&
            d[i + 3] == 0x4C && d[i + 4] == 0x24 && d[i + 5] == 0x08)
            found_load1 = 1;
    }
    for (size_t i = 0; i + 9 < n; i++) {
        if (d[i] == 0x66 && d[i + 1] == 0x44 && d[i + 2] == 0x0F &&
            d[i + 3] == 0x7F && d[i + 4] == 0x84 && d[i + 5] == 0x24 &&
            d[i + 6] == 0x88 && d[i + 7] == 0x00 && d[i + 8] == 0x00 &&
            d[i + 9] == 0x00)
            found_store8 = 1;
        if (d[i] == 0x66 && d[i + 1] == 0x44 && d[i + 2] == 0x0F &&
            d[i + 3] == 0x6F && d[i + 4] == 0xBC && d[i + 5] == 0x24 &&
            d[i + 6] == 0xF8 && d[i + 7] == 0x00 && d[i + 8] == 0x00 &&
            d[i + 9] == 0x00)
            found_load15 = 1;
    }
    T_ASSERT(found_store0);
    T_ASSERT(found_store8);
    T_ASSERT(found_load1);
    T_ASSERT(found_load15);
    emit_module_free(&em);
}


static void test_data_quad_byte(void) {
    static const char src[] =
        "    .text\n"
        "    .globl touch, stub0, stub1\n"
        "touch:\n"
        "    mov syscall_user_rsp(%rip), %rax\n"
        "    mov %rax, syscall_user_rsp(%rip)\n"
        "    ret\n"
        "stub0:\n"
        "    push $0\n"
        "    ret\n"
        "stub1:\n"
        "    push $1\n"
        "    ret\n"
        "    .data\n"
        "    .align 8\n"
        "    .globl syscall_user_rsp, isr_stub_table, marker_bytes\n"
        "syscall_user_rsp:\n"
        "    .quad 0\n"
        "isr_stub_table:\n"
        "    .quad stub0\n"
        "    .quad stub1\n"
        "marker_bytes:\n"
        "    .byte 0x48, 0x0F, 0x07\n"
        "    .long 0x11223344\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "data.S", &em), 0);
    T_ASSERT(has_sym(&em, "syscall_user_rsp"));
    T_ASSERT(has_sym(&em, "isr_stub_table"));
    T_ASSERT(has_sym(&em, "marker_bytes"));
    T_ASSERT(em.data.len >= 8 + 8 + 8 + 3 + 4);
    /* syscall_user_rsp = 8 zero bytes */
    T_ASSERT_EQ_INT((int)(unsigned char)em.data.data[0], 0);
    /* marker_bytes at offset 24: 0x48 0x0F 0x07 */
    T_ASSERT_EQ_INT((int)(unsigned char)em.data.data[24], 0x48);
    T_ASSERT_EQ_INT((int)(unsigned char)em.data.data[25], 0x0F);
    T_ASSERT_EQ_INT((int)(unsigned char)em.data.data[26], 0x07);
    T_ASSERT_EQ_INT((int)(unsigned char)em.data.data[27], 0x44);
    T_ASSERT_EQ_INT((int)(unsigned char)em.data.data[28], 0x33);
    T_ASSERT_EQ_INT((int)(unsigned char)em.data.data[29], 0x22);
    T_ASSERT_EQ_INT((int)(unsigned char)em.data.data[30], 0x11);
    /* two R_X86_64_64 data relocs for stub0/stub1 */
    T_ASSERT(em.num_data_relocs >= 2);
    int si = emit_module_find_symbol(&em, "syscall_user_rsp");
    T_ASSERT(si >= 0);
    T_ASSERT_EQ_INT((int)em.syms[si].shndx, (int)SECT_DATA);
    /* RIP-relative loads/stores against conduit */
    T_ASSERT(em.num_relocs >= 2);
    emit_module_free(&em);
}

static void test_byte_in_text_sysret(void) {
    static const char src[] =
        "    .text\n"
        "    .globl raw_sysret\n"
        "raw_sysret:\n"
        "    .byte 0x48, 0x0F, 0x07\n"
        "    ret\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "sysret.S", &em), 0);
    T_ASSERT(em.text.len >= 4);
    T_ASSERT_EQ_INT((int)(unsigned char)em.text.data[0], 0x48);
    T_ASSERT_EQ_INT((int)(unsigned char)em.text.data[1], 0x0F);
    T_ASSERT_EQ_INT((int)(unsigned char)em.text.data[2], 0x07);
    T_ASSERT_EQ_INT((int)(unsigned char)em.text.data[3], 0xC3);
    emit_module_free(&em);
}


static void test_sysentry_trampolines(void) {
    static const char src[] =
        "    .text\n"
        "    .globl syscall_entry, syscall_epilogue\n"
        "    .globl user_iret_trampoline, fork_ret_trampoline, syscall_user_rsp\n"
        "    .extern syscall_dispatch\n"
        "    .extern syscall_kstack_top\n"
        "syscall_entry:\n"
        "    mov %rsp, syscall_user_rsp(%rip)\n"
        "    mov syscall_kstack_top(%rip), %rsp\n"
        "    sub $264, %rsp\n"
        "    movdqa %xmm0, 0x08(%rsp)\n"
        "    push %r11\n"
        "    push %rcx\n"
        "    push %rax\n"
        "    mov syscall_user_rsp(%rip), %rax\n"
        "    mov %rax, 120(%rsp)\n"
        "    mov %rsp, %rdi\n"
        "    cld\n"
        "    call syscall_dispatch\n"
        "syscall_epilogue:\n"
        "    add $8, %rsp\n"
        "    pop %rcx\n"
        "    pop %r11\n"
        "    movdqa 0x08(%rsp), %xmm0\n"
        "    mov (%rsp), %rsp\n"
        "    sysretq\n"
        "user_iret_trampoline:\n"
        "    push $0x1B\n"
        "    push %rbx\n"
        "    push $0x202\n"
        "    push $0x23\n"
        "    push %r12\n"
        "    iretq\n"
        "fork_ret_trampoline:\n"
        "    xor %eax, %eax\n"
        "    mov %r12, %rsp\n"
        "    jmp syscall_epilogue\n"
        "    .data\n"
        "    .align 8\n"
        "syscall_user_rsp:\n"
        "    .quad 0\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "sysentry.S", &em), 0);
    T_ASSERT(has_sym(&em, "syscall_entry"));
    T_ASSERT(has_sym(&em, "syscall_epilogue"));
    T_ASSERT(has_sym(&em, "user_iret_trampoline"));
    T_ASSERT(has_sym(&em, "fork_ret_trampoline"));
    T_ASSERT(has_sym(&em, "syscall_user_rsp"));
    /* sysretq = 48 0F 07 ; iretq = 48 CF */
    int found_sysret = 0, found_iret = 0;
    const unsigned char *d = (const unsigned char *)em.text.data;
    for (size_t i = 0; i + 2 < em.text.len; i++) {
        if (d[i] == 0x48 && d[i + 1] == 0x0F && d[i + 2] == 0x07)
            found_sysret = 1;
        if (d[i] == 0x48 && d[i + 1] == 0xCF)
            found_iret = 1;
    }
    T_ASSERT(found_sysret);
    T_ASSERT(found_iret);
    /* RIP mov + call dispatch */
    T_ASSERT(em.num_relocs >= 3);
    emit_module_free(&em);
}

static void test_desc_isr_and_helpers(void) {
    static const char src[] =
        "    .text\n"
        "    .globl isr_stub_table, isr_common, gdt_flush, cr2_read, msr_read\n"
        "    .extern isr_dispatch\n"
        "    .extern gdt\n"
        "    .macro ISR_NOERR n\n"
        "    .globl isr_stub_\\n\n"
        "isr_stub_\\n:\n"
        "    push $0\n"
        "    push $\\n\n"
        "    jmp isr_common\n"
        "    .endm\n"
        "    .macro ISR_ERR n\n"
        "    .globl isr_stub_\\n\n"
        "isr_stub_\\n:\n"
        "    push $\\n\n"
        "    jmp isr_common\n"
        "    .endm\n"
        "    ISR_NOERR 0\n"
        "    ISR_ERR 8\n"
        "    ISR_NOERR 32\n"
        "isr_common:\n"
        "    push %rax\n"
        "    sub $256, %rsp\n"
        "    movdqa %xmm0, 0x00(%rsp)\n"
        "    mov %rsp, %rdi\n"
        "    add $256, %rdi\n"
        "    call isr_dispatch\n"
        "    movdqa 0x00(%rsp), %xmm0\n"
        "    add $256, %rsp\n"
        "    pop %rax\n"
        "    add $16, %rsp\n"
        "    iretq\n"
        "gdt_flush:\n"
        "    lgdt gdtr(%rip)\n"
        "    push $0x08\n"
        "    lea .reload_cs(%rip), %rax\n"
        "    push %rax\n"
        "    retfq\n"
        ".reload_cs:\n"
        "    mov $0x10, %ax\n"
        "    mov %ax, %ds\n"
        "    ret\n"
        "cr2_read:\n"
        "    mov %cr2, %rax\n"
        "    ret\n"
        "msr_read:\n"
        "    rdmsr\n"
        "    shl $32, %rdx\n"
        "    or %rdx, %rax\n"
        "    ret\n"
        "    .data\n"
        "    .align 8\n"
        "isr_stub_table:\n"
        "    .quad isr_stub_0\n"
        "    .quad isr_stub_8\n"
        "    .quad isr_stub_32\n"
        "    .section .rodata\n"
        "    .align 4\n"
        "gdtr:\n"
        "    .word 55\n"
        "    .quad gdt\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "desc.S", &em), 0);
    T_ASSERT(has_sym(&em, "isr_stub_0"));
    T_ASSERT(has_sym(&em, "isr_stub_8"));
    T_ASSERT(has_sym(&em, "isr_stub_32"));
    T_ASSERT(has_sym(&em, "isr_common"));
    T_ASSERT(has_sym(&em, "isr_stub_table"));
    T_ASSERT(has_sym(&em, "gdt_flush"));
    T_ASSERT(has_sym(&em, "cr2_read"));
    T_ASSERT(has_sym(&em, "msr_read"));
    /* retfq=48 CB, mov cr2=0F 20 /r, shl imm=C1 /4 */
    int found_retfq = 0, found_cr2 = 0, found_shl = 0, found_sreg = 0;
    const unsigned char *d = (const unsigned char *)em.text.data;
    size_t n = em.text.len;
    for (size_t i = 0; i + 1 < n; i++) {
        if (d[i] == 0x48 && d[i + 1] == 0xCB) found_retfq = 1;
        if (d[i] == 0x0F && d[i + 1] == 0x20) found_cr2 = 1;
        if (d[i] == 0x8E) found_sreg = 1;
    }
    for (size_t i = 0; i + 3 < n; i++) {
        if (d[i] == 0x48 && d[i + 1] == 0xC1 && d[i + 2] == 0xE2 &&
            d[i + 3] == 0x20)
            found_shl = 1;
    }
    T_ASSERT(found_retfq);
    T_ASSERT(found_cr2);
    T_ASSERT(found_shl);
    T_ASSERT(found_sreg);
    /* stub table: 3x R_X86_64_64 ; gdtr RIP + gdt abs */
    T_ASSERT(em.num_data_relocs >= 4);
    T_ASSERT(em.num_relocs >= 2); /* lgdt gdtr(%rip), call isr_dispatch */
    /* mid-function local lea .reload_cs must NOT leave an undef reloc */
    int bad_reload = 0;
    for (size_t i = 0; i < (size_t)em.num_syms; i++) {
        if (em.syms[i].name && strcmp(em.syms[i].name, ".reload_cs") == 0 &&
            em.syms[i].shndx == 0)
            bad_reload = 1;
    }
    T_ASSERT(!bad_reload);
    emit_module_free(&em);
}

static void test_byte_in_text_order(void) {
    /* .byte mid-function must stay after preceding insns (not prepended). */
    static const char src[] =
        "    .text\n"
        "    .globl midbyte\n"
        "midbyte:\n"
        "    nop\n"
        "    .byte 0x48, 0x0F, 0x07\n"
        "    ret\n";
    EmitModule em;
    emit_module_init(&em);
    T_ASSERT_EQ_INT(assemble_s_file(src, "midbyte.S", &em), 0);
    T_ASSERT(em.text.len >= 5);
    T_ASSERT_EQ_INT((int)(unsigned char)em.text.data[0], 0x90);
    T_ASSERT_EQ_INT((int)(unsigned char)em.text.data[1], 0x48);
    T_ASSERT_EQ_INT((int)(unsigned char)em.text.data[2], 0x0F);
    T_ASSERT_EQ_INT((int)(unsigned char)em.text.data[3], 0x07);
    T_ASSERT_EQ_INT((int)(unsigned char)em.text.data[4], 0xC3);
    emit_module_free(&em);
}

int main(void) {
    target_set_current(target_x86_64_linux());
    test_assemble_io_subset();
    test_macro_usysc();
    test_priv_and_rip();
    test_rept_stubs();
    test_incbin_rodata();
    test_switch_subset();
    test_movdqa_xmm();
    test_data_quad_byte();
    test_byte_in_text_sysret();
    test_sysentry_trampolines();
    test_desc_isr_and_helpers();
    test_byte_in_text_order();
    return t_finalize();
}
