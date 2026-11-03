#include "fakecc/emit.h"
#include "fakecc/iasm.h"
#include "fakecc/target.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(void) {
    target_set_current(target_x86_64_linux());
    test_assemble_io_subset();
    test_macro_usysc();
    test_priv_and_rip();
    test_rept_stubs();
    return t_finalize();
}
