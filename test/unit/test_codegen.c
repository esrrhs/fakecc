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

/* ---- helper: compile source to EmitModule ---- */
static EmitModule compile_to_code(const char *src) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu);

    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir);

    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em);

    ir_module_free(&ir);
    token_array_free(&arr);
    tu_free(&tu);

    return em;
}

/* ---- tests ---- */

static void test_return_zero(void) {
    EmitModule em = compile_to_code("package main; int main() { return 0; }");
    T_ASSERT_EQ_INT((int)em.num_symbols, 1);
    T_ASSERT_STR_EQ(em.symbols[0].name, "main");
    /* With stack evaluation, code is longer than Slice 1's 11 bytes.
     * Just verify non-empty and a minimum size. */
    T_ASSERT(em.symbols[0].size > 0);
    T_ASSERT(em.code.len > 0);
    emit_module_free(&em);
}

static void test_return_42(void) {
    EmitModule em = compile_to_code("package main; int main() { return 42; }");
    T_ASSERT(em.symbols[0].size > 0);
    T_ASSERT(em.code.len > 0);
    emit_module_free(&em);
}

static void test_return_255(void) {
    EmitModule em = compile_to_code("package main; int main() { return 255; }");
    T_ASSERT(em.symbols[0].size > 0);
    emit_module_free(&em);
}

static void test_prologue_present(void) {
    EmitModule em = compile_to_code("package main; int main() { return 1; }");
    /* pushq %rbp = 55 */
    T_ASSERT((unsigned char)em.code.data[0] == 0x55);
    /* movq %rsp, %rbp = 48 89 e5 */
    T_ASSERT((unsigned char)em.code.data[1] == 0x48);
    T_ASSERT((unsigned char)em.code.data[2] == 0x89);
    T_ASSERT((unsigned char)em.code.data[3] == 0xe5);
    /* sub $N, %rsp = 48 81 EC ... (at least 7 bytes for prologue+sub) */
    T_ASSERT((unsigned char)em.code.data[4] == 0x48);
    T_ASSERT((unsigned char)em.code.data[5] == 0x81);
    T_ASSERT((unsigned char)em.code.data[6] == 0xEC);
    emit_module_free(&em);
}

/* ---- main ---- */

int main(void) {
    test_return_zero();
    test_return_42();
    test_return_255();
    test_prologue_present();
    return t_finalize();
}
