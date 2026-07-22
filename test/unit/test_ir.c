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
    /* Now: IR_CONST v0=0; IR_RETURN v0 */
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 2);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].imm, 0);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[1].op, (int)IR_RETURN);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[1].a, 0);
    ir_module_free(&ir);
}

static void test_return_42(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 42; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 2);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].imm, 42);
    ir_module_free(&ir);
}

static void test_return_255(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 255; }");
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].imm, 255);
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

/* ---- Slice 2: expression IR tests ---- */

static void test_add_ir(void) {
    /* return 1+2; → CONST v0=1; CONST v1=2; ADD v2=v0+v1; RETURN v2 */
    IRModule ir = compile_to_ir("package main; int main() { return 1 + 2; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 4);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].imm, 1);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[1].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[1].imm, 2);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[2].op, (int)IR_ADD);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[3].op, (int)IR_RETURN);
    ir_module_free(&ir);
}

static void test_neg_ir(void) {
    /* return -5; → CONST v0=5; NEG v1=v0; RETURN v1 */
    IRModule ir = compile_to_ir("package main; int main() { return -5; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 3);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].imm, 5);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[1].op, (int)IR_NEG);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[2].op, (int)IR_RETURN);
    ir_module_free(&ir);
}

static void test_mul_ir(void) {
    /* return 2*3; → CONST v0=2; CONST v1=3; MUL v2=v0*v1; RETURN v2 */
    IRModule ir = compile_to_ir("package main; int main() { return 2 * 3; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[2].op, (int)IR_MUL);
    ir_module_free(&ir);
}

static void test_div_mod_ir(void) {
    /* return 17%5; → ... MOD ...; RETURN */
    IRModule ir = compile_to_ir("package main; int main() { return 17 % 5; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[2].op, (int)IR_MOD);
    ir_module_free(&ir);

    IRModule ir2 = compile_to_ir("package main; int main() { return 20 / 4; }");
    T_ASSERT_EQ_INT((int)ir2.functions.data[0].insts.data[2].op, (int)IR_DIV);
    ir_module_free(&ir2);
}

/* ---- main ---- */

int main(void) {
    test_return_zero();
    test_return_42();
    test_return_255();
    test_function_name_propagated();
    test_source_loc_propagated();
    test_add_ir();
    test_neg_ir();
    test_mul_ir();
    test_div_mod_ir();
    return t_finalize();
}
