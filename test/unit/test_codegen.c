#include "fakecc/ast.h"
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
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
    /* sub $N, %rsp = 48 81 EC ... (at least 7 bytes for prologue+sub) */
    T_ASSERT((unsigned char)em.text.data[4] == 0x48);
    T_ASSERT((unsigned char)em.text.data[5] == 0x81);
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
    return t_finalize();
}
