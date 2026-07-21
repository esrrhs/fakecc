#include "fakecc/ast.h"
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>

/* ---- helper: compile source to assembly string ---- */
static char *compile_to_asm(const char *src) {
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

    Buffer asm_buf;
    buffer_init(&asm_buf);
    codegen(&ir, &asm_buf);

    /* null-terminate for string searches */
    buffer_append(&asm_buf, "\0", 1);

    ir_module_free(&ir);
    token_array_free(&arr);
    tu_free(&tu);

    return asm_buf.data;  /* caller must free */
}

/* ---- tests ---- */

static void test_return_zero(void) {
    char *asm_out = compile_to_asm("package main; int main() { return 0; }");
    T_ASSERT(strstr(asm_out, "movl $0, %eax") != NULL);
    T_ASSERT(strstr(asm_out, ".globl main") != NULL);
    T_ASSERT(strstr(asm_out, "main:") != NULL);
    T_ASSERT(strstr(asm_out, "pushq %rbp") != NULL);
    T_ASSERT(strstr(asm_out, "popq %rbp") != NULL);
    T_ASSERT(strstr(asm_out, "ret") != NULL);
    free(asm_out);
}

static void test_return_42(void) {
    char *asm_out = compile_to_asm("package main; int main() { return 42; }");
    T_ASSERT(strstr(asm_out, "movl $42, %eax") != NULL);
    free(asm_out);
}

static void test_return_255(void) {
    char *asm_out = compile_to_asm("package main; int main() { return 255; }");
    T_ASSERT(strstr(asm_out, "movl $255, %eax") != NULL);
    free(asm_out);
}

/* ---- main ---- */

int main(void) {
    test_return_zero();
    test_return_42();
    test_return_255();
    return t_finalize();
}
