#include "fakecc/ast.h"
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/opt.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---- helper: find a defined symbol by name ---- */
static const EmitSymbol *find_sym(const EmitModule *m, const char *name) {
    for (size_t i = 0; i < m->num_syms; i++) {
        if (m->syms[i].name && strcmp(m->syms[i].name, name) == 0)
            return &m->syms[i];
    }
    return NULL;
}

/* ---- helper: compile source to EmitModule ---- */
static EmitModule compile_to_code(const char *src) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu, 1);

    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir, 0);

    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em, 0);

    ir_module_free(&ir);
    token_array_free(&arr);
    tu_free(&tu);

    return em;
}

/* Full -O0 pipeline: pin locals, RA, then isel. */
static EmitModule compile_o0(const char *src) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu, 1);

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

/* ---- tests ---- */

static void test_return_zero(void) {
    EmitModule em = compile_to_code("package main; int main() { return 0; }");
    const EmitSymbol *main_sym = find_sym(&em, "main");
    T_ASSERT(main_sym != NULL);
    /* With stack evaluation, code is longer than Slice 1's 11 bytes.
     * Just verify non-empty and a minimum size. */
    T_ASSERT(main_sym->size > 0);
    T_ASSERT(em.text.len > 0);
    emit_module_free(&em);
}

static void test_return_42(void) {
    EmitModule em = compile_to_code("package main; int main() { return 42; }");
    T_ASSERT(find_sym(&em, "main")->size > 0);
    T_ASSERT(em.text.len > 0);
    emit_module_free(&em);
}

static void test_return_255(void) {
    EmitModule em = compile_to_code("package main; int main() { return 255; }");
    T_ASSERT(find_sym(&em, "main")->size > 0);
    emit_module_free(&em);
}

static void test_prologue_present(void) {
    EmitModule em = compile_to_code("package main; int main() { return 1; }");
    /* pushq %rbp = 55 */
    T_ASSERT((unsigned char)em.text.data[0] == 0x55);
    /* movq %rsp, %rbp = 48 89 e5 */
    T_ASSERT((unsigned char)em.text.data[1] == 0x48);
    T_ASSERT((unsigned char)em.text.data[2] == 0x89);
    T_ASSERT((unsigned char)em.text.data[3] == 0xe5);
    /* sub $N, %rsp = 48 83 EC ib (imm8) or 48 81 EC id (imm32) */
    T_ASSERT((unsigned char)em.text.data[4] == 0x48);
    T_ASSERT((unsigned char)em.text.data[5] == 0x83
             || (unsigned char)em.text.data[5] == 0x81);
    T_ASSERT((unsigned char)em.text.data[6] == 0xEC);
    emit_module_free(&em);
}

/* ---- Slice 3: variable codegen tests ---- */

static void test_var_codegen(void) {
    /* int x; x = 42; return x; — non-empty code + correct prologue */
    EmitModule em = compile_to_code(
        "package main; int main() { int x; x = 42; return x; }");
    const EmitSymbol *main_sym = find_sym(&em, "main");
    T_ASSERT(main_sym != NULL);
    T_ASSERT(main_sym->size > 0);
    T_ASSERT(em.text.len > 0);
    /* prologue still intact */
    T_ASSERT((unsigned char)em.text.data[0] == 0x55);
    T_ASSERT((unsigned char)em.text.data[1] == 0x48);
    T_ASSERT((unsigned char)em.text.data[2] == 0x89);
    T_ASSERT((unsigned char)em.text.data[3] == 0xe5);
    emit_module_free(&em);
}

static void test_arith_codegen_longer(void) {
    EmitModule simple = compile_to_code(
        "package main; int main() { return 1; }");
    EmitModule arith = compile_to_code(
        "package main; int main() { return 1 + 2 * 3 - 4; }");
    T_ASSERT(arith.text.len > simple.text.len);
    T_ASSERT(find_sym(&arith, "main") != NULL);
    emit_module_free(&simple);
    emit_module_free(&arith);
}

static void test_if_codegen_has_main(void) {
    EmitModule em = compile_to_code(
        "package main; int main() { int x = 1; if (x) return 2; return 3; }");
    const EmitSymbol *main_sym = find_sym(&em, "main");
    T_ASSERT(main_sym != NULL);
    T_ASSERT(main_sym->size > 16);
    emit_module_free(&em);
}

static void test_multi_function_symbols(void) {
    EmitModule em = compile_to_code(
        "package main;"
        "int add(int a, int b) { return a + b; }"
        "int main() { return add(1, 2); }");
    T_ASSERT(find_sym(&em, "main") != NULL);
    T_ASSERT(find_sym(&em, "add") != NULL);
    emit_module_free(&em);
}

static void test_bitfield_codegen(void) {
    EmitModule em = compile_to_code(
        "package main;"
        "struct F { unsigned a : 3; unsigned b : 5; };"
        "int main() { struct F f; f.a = 1; f.b = 2; return f.a + f.b; }");
    T_ASSERT(find_sym(&em, "main") != NULL);
    T_ASSERT(em.text.len > 0);
    emit_module_free(&em);
}

static int text_has_sib(const EmitModule *em, unsigned char sib) {
    /* ModRM.rm == 4 means a SIB byte follows. */
    for (size_t i = 1; i < em->text.len; i++) {
        unsigned char modrm = (unsigned char)em->text.data[i - 1];
        if ((modrm & 7) == 4 && (unsigned char)em->text.data[i] == sib)
            return 1;
    }
    return 0;
}

/* g[i] at -O0 should be movl (%rax,%rcx), not lea+add+indirect. */
static void test_sib_global_index(void) {
    EmitModule em = compile_o0(
        "package main;"
        "int g[8];"
        "int idx(int i) { return g[i]; }"
        "int main() { return idx(3); }");
    /* [rax+rcx] SIB is 0x08; swapped [rcx+rax] is 0x01. */
    T_ASSERT(text_has_sib(&em, 0x08) || text_has_sib(&em, 0x01));
    emit_module_free(&em);
}

/* a[i] of a pinned local should be [rbp+rcx+off] (SIB base=rbp index=rcx). */
static void test_sib_local_index(void) {
    EmitModule em = compile_o0(
        "package main;"
        "int f(int i) { int a[8]; a[0] = 1; a[i] = 2; return a[i]; }"
        "int main() { return f(1); }");
    /* SIB index=rcx (1), base=rbp (5) → 0x0D. */
    T_ASSERT(text_has_sib(&em, 0x0D));
    emit_module_free(&em);
}

/* 0F 10/11 without F2/F3/66 is movups — required to move a 16-byte vector
 * through one XMM.  Scalar float uses F2/F3 prefixes (movsd/movss). */
static int text_has_unprefixed_0f(const EmitModule *em, unsigned char op) {
    for (size_t i = 0; i + 1 < em->text.len; i++) {
        if ((unsigned char)em->text.data[i] != 0x0F) continue;
        if ((unsigned char)em->text.data[i + 1] != op) continue;
        int pref = 0;
        if (i > 0) {
            unsigned char p = (unsigned char)em->text.data[i - 1];
            if (p == 0xF2 || p == 0xF3 || p == 0x66) pref = 1;
            if (!pref && i > 1 && (p & 0xF0) == 0x40) {
                unsigned char p2 = (unsigned char)em->text.data[i - 2];
                if (p2 == 0xF2 || p2 == 0xF3 || p2 == 0x66) pref = 1;
            }
        }
        if (!pref) return 1;
    }
    return 0;
}

static void test_vector16_uses_movups(void) {
    EmitModule em = compile_o0(
        "package main;"
        "typedef double V __attribute__((vector_size(16)));"
        "V id(V v) { return v; }"
        "int main(void) { V a = { 1.0, 2.0 }; V b = id(a); return (int)b[0]; }");
    T_ASSERT(find_sym(&em, "id") != NULL);
    T_ASSERT(text_has_unprefixed_0f(&em, 0x10)
             || text_has_unprefixed_0f(&em, 0x11));
    emit_module_free(&em);
}

static void test_constructor_init_array(void) {
    EmitModule em = compile_to_code(
        "package main;"
        "int g;"
        "__attribute__((constructor)) void ctor(void) { g = 7; }"
        "int main(void) { return g; }");
    T_ASSERT(find_sym(&em, "ctor") != NULL);
    T_ASSERT_EQ_INT((int)em.init_array.len, 8);
    T_ASSERT(em.num_data_relocs >= 1);
    int saw = 0;
    for (size_t i = 0; i < em.num_data_relocs; i++) {
        if (em.data_relocs[i].shndx == SECT_INIT_ARRAY
            && em.data_relocs[i].type == R_X86_64_64)
            saw = 1;
    }
    T_ASSERT(saw);
    emit_module_free(&em);
}

static void test_destructor_fini_array(void) {
    EmitModule em = compile_to_code(
        "package main;"
        "int g;"
        "__attribute__((destructor)) void dtor(void) { g = 1; }"
        "int main(void) { return g; }");
    T_ASSERT(find_sym(&em, "dtor") != NULL);
    T_ASSERT_EQ_INT((int)em.fini_array.len, 8);
    int saw = 0;
    for (size_t i = 0; i < em.num_data_relocs; i++) {
        if (em.data_relocs[i].shndx == SECT_FINI_ARRAY
            && em.data_relocs[i].type == R_X86_64_64)
            saw = 1;
    }
    T_ASSERT(saw);
    emit_module_free(&em);
}

static void test_constructor_priority_slots(void) {
    EmitModule em = compile_to_code(
        "package main;"
        "int g;"
        "__attribute__((constructor(200))) void ctor_b(void) { g = 2; }"
        "__attribute__((constructor(101))) void ctor_a(void) { g = 1; }"
        "int main(void) { return g; }");
    T_ASSERT_EQ_INT((int)em.init_array.len, 16);
    T_ASSERT(em.init_prio != NULL);
    T_ASSERT_EQ_INT(em.init_prio[0], 200);
    T_ASSERT_EQ_INT(em.init_prio[1], 101);
    emit_module_free(&em);
}

static IRModule compile_src_to_ir(const char *src) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu, 1);

    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir, 0);

    token_array_free(&arr);
    tu_free(&tu);
    return ir;
}

static void test_rol8_encoding(void) {
    IRModule ir = compile_src_to_ir(
        "package main; unsigned rot(unsigned x, unsigned n) {"
        "  return (x << n) | (x >> (32u - n)); }"
        "int main() { return (int)rot(1u, 1u); }");
    int found = 0;
    for (size_t f = 0; f < ir.functions.len; f++) {
        IRFunction *fn = &ir.functions.data[f];
        for (size_t i = 0; i < fn->insts.len; i++) {
            if (fn->insts.data[i].op == IR_ROL) {
                fn->insts.data[i].width = 1;
                found = 1;
            }
        }
    }
    T_ASSERT(found);
    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em, 0);
    int saw_d2 = 0;
    for (size_t i = 0; i < em.text.len; i++) {
        if ((unsigned char)em.text.data[i] == 0xD2) saw_d2 = 1;
    }
    T_ASSERT(saw_d2);
    emit_module_free(&em);
    ir_module_free(&ir);
}

static void test_builtin_return_codegen(void) {
    EmitModule em = compile_to_code(
        "package main; int add1(int x) { return x + 1; }"
        "int wrap(int x) {"
        "  void *args = __builtin_apply_args();"
        "  __builtin_return(__builtin_apply((void (*)())add1, args, 16));"
        "} int main() { return wrap(41); }");
    T_ASSERT(find_sym(&em, "wrap") != NULL);
    T_ASSERT(find_sym(&em, "wrap")->size > 0);
    emit_module_free(&em);
}

/* ---- main ---- */

int main(void) {
    test_return_zero();
    test_return_42();
    test_return_255();
    test_prologue_present();
    test_var_codegen();
    test_arith_codegen_longer();
    test_if_codegen_has_main();
    test_multi_function_symbols();
    test_bitfield_codegen();
    test_sib_global_index();
    test_sib_local_index();
    test_vector16_uses_movups();
    test_constructor_init_array();
    test_destructor_fini_array();
    test_constructor_priority_slots();
    test_rol8_encoding();
    test_builtin_return_codegen();
    return t_finalize();
}
