#include "fakecc/ast.h"
#include "fakecc/common.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>

/* ---- helper: lex + parse + sema + ir_generate ---- */
static IRModule compile_to_ir(const char *src) {
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

    token_array_free(&arr);
    tu_free(&tu);

    return ir;
}

/* ---- tests ---- */

static void test_return_zero(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 0; }");
    T_ASSERT_EQ_INT((int)ir.functions.len, 1);
    T_ASSERT_STR_EQ(ir.functions.data[0].name, "main");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 1);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_RETURN);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].value, 0);
    ir_module_free(&ir);
}

static void test_return_42(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 42; }");
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].value, 42);
    ir_module_free(&ir);
}

static void test_return_255(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 255; }");
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].value, 255);
    ir_module_free(&ir);
}

static void test_function_name_propagated(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 1; }");
    T_ASSERT_STR_EQ(ir.functions.data[0].name, "main");
    ir_module_free(&ir);
}

static void test_source_loc_propagated(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 42; }");
    T_ASSERT(ir.functions.data[0].insts.data[0].loc.line > 0);
    ir_module_free(&ir);
}

/* ---- main ---- */

int main(void) {
    test_return_zero();
    test_return_42();
    test_return_255();
    test_function_name_propagated();
    test_source_loc_propagated();
    return t_finalize();
}
