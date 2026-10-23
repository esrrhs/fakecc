#include "test_framework.h"
#include "fakecc/target.h"
#include "fakecc/ast.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/ir.h"
#include "fakecc/opt.h"
#include "fakecc/codegen.h"
#include "fakecc/regalloc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static EmitModule compile_snippet(const char *src, const TargetDesc *target) {
    target_set_current(target);
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "snippet.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu, 0);

    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir, 1);
    opt(&ir, 0, 0);

    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em, 0);

    ir_module_free(&ir);
    token_array_free(&arr);
    tu_free(&tu);
    return em;
}

static const EmitSymbol *find_sym(const EmitModule *m, const char *name) {
    for (size_t i = 0; i < m->num_syms; i++) {
        if (m->syms[i].name && strcmp(m->syms[i].name, name) == 0)
            return &m->syms[i];
    }
    return NULL;
}

/* Test 1: TargetDesc gp regs */
static void test_target_desc_gp_regs(void) {
    const TargetDesc *win = target_x86_64_windows();
    const TargetDesc *lin = target_x86_64_linux();

    T_ASSERT_EQ_INT(win->gp_arg_regs, 4);
    T_ASSERT_EQ_INT(lin->gp_arg_regs, 6);
    T_ASSERT_EQ_INT((int)win->os, (int)TOS_WINDOWS);
    T_ASSERT_EQ_INT((int)lin->os, (int)TOS_LINUX);
}

/* Test 2: long double width */
static void test_long_double_width(void) {
    target_set_current(target_x86_64_windows());
    T_ASSERT_EQ_INT((int)type_long_double_width(), 8);

    target_set_current(target_x86_64_linux());
    T_ASSERT_EQ_INT((int)type_long_double_width(), 16);
}

/* Test 3: va_list tag representation */
static void test_va_list_representation(void) {
    target_set_current(target_x86_64_windows());
    TranslationUnit tu_win;
    tu_init(&tu_win);
    const StructDef *va_win = struct_registry_find(&tu_win.structs, "__va_list_tag");
    T_ASSERT(va_win != NULL);
    T_ASSERT_EQ_INT(va_win->num_members, 1);
    T_ASSERT_STR_EQ(va_win->members[0].name, "overflow_arg_area");
    tu_free(&tu_win);

    target_set_current(target_x86_64_linux());
    TranslationUnit tu_lin;
    tu_init(&tu_lin);
    const StructDef *va_lin = struct_registry_find(&tu_lin.structs, "__va_list_tag");
    T_ASSERT(va_lin != NULL);
    T_ASSERT_EQ_INT(va_lin->num_members, 4);
    tu_free(&tu_lin);
}

/* Test 4: aggregate classification and ABI indirect rules */
static void test_aggregate_classification(void) {
    target_set_current(target_x86_64_windows());

    Type s4 = type_make_struct("S4", 4);
    SysVRegClass cls4[2] = {0, 0};
    T_ASSERT_EQ_INT(sysv_classify_agg(s4, cls4), 1);
    T_ASSERT_EQ_INT((int)cls4[0], (int)SYSV_CLS_INTEGER);
    T_ASSERT_EQ_INT(abi_indirect_agg(s4), 0);
    type_free(&s4);

    /* Struct of 12 bytes: on Win64 must be passed indirect (MEMORY / pointer) */
    Type s12 = type_make_struct("S12", 12);
    SysVRegClass cls12[2] = {0, 0};
    T_ASSERT_EQ_INT(sysv_classify_agg(s12, cls12), 0);
    T_ASSERT_EQ_INT(abi_indirect_agg(s12), 1);
    type_free(&s12);

    /* Struct of 16 bytes: on Win64 must be indirect (pointer) */
    Type s16 = type_make_struct("S16", 16);
    SysVRegClass cls16[2] = {0, 0};
    T_ASSERT_EQ_INT(sysv_classify_agg(s16, cls16), 0);
    T_ASSERT_EQ_INT(abi_indirect_agg(s16), 1);

    /* On Linux, S16 classifies as two INTEGER eightbytes by value */
    target_set_current(target_x86_64_linux());
    T_ASSERT_EQ_INT(sysv_classify_agg(s16, cls16), 2);
    T_ASSERT_EQ_INT((int)cls16[0], (int)SYSV_CLS_INTEGER);
    T_ASSERT_EQ_INT((int)cls16[1], (int)SYSV_CLS_INTEGER);
    T_ASSERT_EQ_INT(abi_indirect_agg(s16), 0);
    type_free(&s16);
}

/* Test 5: Regalloc caller-saved vs callee-saved masks */
static void test_regalloc_win64(void) {
    target_set_current(target_x86_64_windows());
    const RaRegClasses *rc = ra_classes_current();
    /* On Win64: R8, R9, R10, R11 caller-saved => bits 2, 3, 4, 5 => 0x3C */
    T_ASSERT_EQ_INT((int)rc->gp.caller_saved, 0x3C);

    target_set_current(target_x86_64_linux());
    rc = ra_classes_current();
    /* On Linux: caller-saved includes RSI (bit 3), RDI (bit 4), RDX, RCX, R8..R11 */
    T_ASSERT(rc->gp.caller_saved != 0x3C);
}

/* Test 6: Win64 shadow space and positional argument passing */
static void test_win64_shadow_space_codegen(void) {
    const char *src =
        "package main;\n"
        "void callee(int a, int b, int c, int d);\n"
        "void caller(void) {\n"
        "    callee(1, 2, 3, 4);\n"
        "}\n";

    EmitModule em = compile_snippet(src, target_x86_64_windows());
    const EmitSymbol *sym = find_sym(&em, "caller");
    T_ASSERT(sym != NULL);
    T_ASSERT(sym->size > 0);

    /* Shadow space of at least 32 bytes allocated before calling callee:
     * sub rsp, 32 (48 83 ec 20) or larger.
     */
    const uint8_t *code = (const uint8_t *)(em.text.data + sym->value);
    int found_sub_rsp = 0;
    for (size_t i = 0; i + 3 < sym->size; i++) {
        if (code[i] == 0x48 && code[i+1] == 0x83 && code[i+2] == 0xec) {
            uint8_t imm = code[i+3];
            if (imm >= 32) {
                found_sub_rsp = 1;
                break;
            }
        }
    }
    T_ASSERT(found_sub_rsp);
    emit_module_free(&em);
}

/* Test 7: Variadic function shadow spill */
static void test_win64_variadic_shadow_spill(void) {
    const char *src =
        "package main;\n"
        "int vfn(int count, ...) {\n"
        "    return count;\n"
        "}\n";

    EmitModule em = compile_snippet(src, target_x86_64_windows());
    const EmitSymbol *sym = find_sym(&em, "vfn");
    T_ASSERT(sym != NULL);
    T_ASSERT(sym->size > 0);

    /* Variadic functions on Win64 spill registers to [rbp+16], [rbp+24], [rbp+32], [rbp+40]:
     * mov [rbp+16], rcx -> 48 89 4d 10
     * mov [rbp+24], rdx -> 48 89 55 18
     * mov [rbp+32], r8  -> 4c 89 45 20
     * mov [rbp+40], r9  -> 4c 89 4d 28
     */
    const uint8_t *code = (const uint8_t *)(em.text.data + sym->value);
    int found_spill_rdx = 0;
    int found_spill_r8 = 0;
    int found_spill_r9 = 0;
    for (size_t i = 0; i + 3 < sym->size; i++) {
        if (code[i] == 0x48 && code[i+1] == 0x89 && code[i+2] == 0x55 && code[i+3] == 0x18)
            found_spill_rdx = 1;
        if (code[i] == 0x4c && code[i+1] == 0x89 && code[i+2] == 0x45 && code[i+3] == 0x20)
            found_spill_r8 = 1;
        if (code[i] == 0x4c && code[i+1] == 0x89 && code[i+2] == 0x4d && code[i+3] == 0x28)
            found_spill_r9 = 1;
    }
    T_ASSERT(found_spill_rdx);
    T_ASSERT(found_spill_r8);
    T_ASSERT(found_spill_r9);
    emit_module_free(&em);
}

/* Test 8: Variadic caller duplicates FP arguments to GP registers */
static void test_win64_variadic_fp_duplication(void) {
    const char *src =
        "package main;\n"
        "int printf(char *fmt, ...);\n"
        "void test(void) {\n"
        "    printf(\"%f\", 3.14);\n"
        "}\n";

    EmitModule em = compile_snippet(src, target_x86_64_windows());
    const EmitSymbol *sym = find_sym(&em, "test");
    T_ASSERT(sym != NULL);
    T_ASSERT(sym->size > 0);

    /* In printf("%f", 3.14):
     * arg 0 is "%f" (RCX).
     * arg 1 is 3.14 in XMM1.
     * On Win64 variadic calls, arg 1 in XMM1 must also be duplicated to RDX:
     * emit_movq_gp_xmm: 66 REX.W 0F 7E /r
     */
    const uint8_t *code = (const uint8_t *)(em.text.data + sym->value);
    int found_movq = 0;
    for (size_t i = 0; i + 3 < sym->size; i++) {
        if (code[i] == 0x66 && (code[i+1] & 0xf8) == 0x48 &&
            code[i+2] == 0x0f && code[i+3] == 0x7e) {
            found_movq = 1;
            break;
        }
    }
    T_ASSERT(found_movq);
    emit_module_free(&em);
}

/* Test 9: Win64 TLS codegen (GS:[0x58], _tls_index, SECREL) */
static void test_win64_tls_codegen(void) {
    const char *src =
        "package main;\n"
        "__thread int g_tls = 42;\n"
        "int get_tls(void) {\n"
        "    return g_tls;\n"
        "}\n";

    EmitModule em = compile_snippet(src, target_x86_64_windows());
    const EmitSymbol *sym = find_sym(&em, "get_tls");
    T_ASSERT(sym != NULL);
    T_ASSERT(sym->size > 0);

    /* Win64 TLS codegen:
     * 1. Accesses _tls_index via RIP-relative displacement
     * 2. mov reg, gs:[0x58] (0x65, 0x48/0x4C, 0x8B, ..., 0x58)
     * 3. Relocation for g_tls is R_PE_SECREL
     */
    const uint8_t *code = (const uint8_t *)(em.text.data + sym->value);
    int found_gs58 = 0;
    for (size_t i = 0; i + 5 < sym->size; i++) {
        if (code[i] == 0x65 && (code[i+1] == 0x48 || code[i+1] == 0x4c) &&
            code[i+2] == 0x8b && code[i+5] == 0x58) {
            found_gs58 = 1;
            break;
        }
    }
    T_ASSERT(found_gs58);

    int found_secrel = 0;
    int found_tls_index_rel = 0;
    for (size_t i = 0; i < em.num_relocs; i++) {
        const EmitReloc *r = &em.relocs[i];
        if (r->type == R_PE_SECREL) found_secrel = 1;
        if (r->sym < em.num_syms && em.syms[r->sym].name &&
            strcmp(em.syms[r->sym].name, "_tls_index") == 0)
            found_tls_index_rel = 1;
    }
    T_ASSERT(found_secrel);
    T_ASSERT(found_tls_index_rel);
    emit_module_free(&em);
}

/* Test 10: Linux System V TLS codegen is preserved intact */
static void test_linux_tls_preserved(void) {
    const char *src =
        "package main;\n"
        "__thread int g_tls = 42;\n"
        "int get_tls(void) {\n"
        "    return g_tls;\n"
        "}\n";

    EmitModule em = compile_snippet(src, target_x86_64_linux());
    const EmitSymbol *sym = find_sym(&em, "get_tls");
    T_ASSERT(sym != NULL);
    T_ASSERT(sym->size > 0);

    /* Linux System V TLS:
     * %fs:0 access: 0x64, 0x48, 0x8B ...
     * Relocation is R_X86_64_GOTTPOFF
     */
    const uint8_t *code = (const uint8_t *)(em.text.data + sym->value);
    int found_fs0 = 0;
    for (size_t i = 0; i + 3 < sym->size; i++) {
        if (code[i] == 0x64 && code[i+1] == 0x48 && code[i+2] == 0x8b) {
            found_fs0 = 1;
            break;
        }
    }
    T_ASSERT(found_fs0);

    int found_gottpoff = 0;
    for (size_t i = 0; i < em.num_relocs; i++) {
        if (em.relocs[i].type == R_X86_64_GOTTPOFF)
            found_gottpoff = 1;
    }
    T_ASSERT(found_gottpoff);
    emit_module_free(&em);
}

int main(void) {
    test_target_desc_gp_regs();
    test_long_double_width();
    test_va_list_representation();
    test_aggregate_classification();
    test_regalloc_win64();
    test_win64_shadow_space_codegen();
    test_win64_variadic_shadow_spill();
    test_win64_variadic_fp_duplication();
    test_win64_tls_codegen();
    test_linux_tls_preserved();
    return t_finalize();
}
