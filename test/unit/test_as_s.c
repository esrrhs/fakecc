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

int main(void) {
    target_set_current(target_x86_64_linux());
    test_assemble_io_subset();
    test_macro_usysc();
    test_priv_and_rip();
    test_rept_stubs();
    test_incbin_rodata();
    test_switch_subset();
    test_movdqa_xmm();
    return t_finalize();
}
