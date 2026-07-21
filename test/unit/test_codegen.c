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

/* ---- helpers: read little-endian int32 from buffer ---- */
static int32_t read_le32(const char *buf, size_t offset) {
    int32_t val;
    memcpy(&val, buf + offset, 4);
    return val;
}

/* ---- tests ---- */

static void test_return_zero(void) {
    EmitModule em = compile_to_code("package main; int main() { return 0; }");
    T_ASSERT_EQ_INT((int)em.num_symbols, 1);
    T_ASSERT_STR_EQ(em.symbols[0].name, "main");
    /* Layout: push(1) mov(3) movl(5) pop(1) ret(1) = 11 bytes */
    T_ASSERT_EQ_INT((int)em.symbols[0].size, 11);
    /* movl $0, %eax: opcode b8 at offset 4, immediate at offset 5 */
    int32_t val = read_le32(em.code.data, 5);
    T_ASSERT_EQ_INT(val, 0);
    emit_module_free(&em);
}

static void test_return_42(void) {
    EmitModule em = compile_to_code("package main; int main() { return 42; }");
    int32_t val = read_le32(em.code.data, 5);
    T_ASSERT_EQ_INT(val, 42);
    emit_module_free(&em);
}

static void test_return_255(void) {
    EmitModule em = compile_to_code("package main; int main() { return 255; }");
    int32_t val = read_le32(em.code.data, 5);
    T_ASSERT_EQ_INT(val, 255);
    emit_module_free(&em);
}

static void test_prologue_epilogue(void) {
    EmitModule em = compile_to_code("package main; int main() { return 1; }");
    /* pushq %rbp */
    T_ASSERT((unsigned char)em.code.data[0] == 0x55);
    /* movq %rsp, %rbp */
    T_ASSERT((unsigned char)em.code.data[1] == 0x48);
    T_ASSERT((unsigned char)em.code.data[2] == 0x89);
    T_ASSERT((unsigned char)em.code.data[3] == 0xe5);
    /* movl $1, %eax at offset 4 */
    T_ASSERT((unsigned char)em.code.data[4] == 0xb8);
    /* popq %rbp at offset 9 */
    T_ASSERT((unsigned char)em.code.data[9] == 0x5d);
    /* ret at offset 10 */
    T_ASSERT((unsigned char)em.code.data[10] == 0xc3);
    emit_module_free(&em);
}

/* ---- main ---- */

int main(void) {
    test_return_zero();
    test_return_42();
    test_return_255();
    test_prologue_epilogue();
    return t_finalize();
}
