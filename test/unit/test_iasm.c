#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/iasm.h"
#include "test_framework.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static IAsmOperand make_reg(int reg, int width) {
    IAsmOperand op;
    memset(&op, 0, sizeof op);
    op.kind = 0;
    op.reg = reg;
    op.width = width;
    return op;
}

static void test_valid_operand_index(void) {
    Buffer text;
    EmitModule em;
    IAsmOperand ops[2];
    buffer_init(&text);
    emit_module_init(&em);
    ops[0] = make_reg(0, 8);
    ops[1] = make_reg(1, 8);
    fakecc_clear_error();
    T_ASSERT_EQ_INT(iasm_emit_x86(&text, &em, "movq %0, %1", ops, 2, "t.c", 1, 1), 0);
    T_ASSERT(!fakecc_had_error());
    emit_module_free(&em);
    buffer_free(&text);
}

/* Logged UBSan site: idx = idx * 10 + digit overflowed on 1111111111 * 10. */
static void test_overflowing_operand_index(void) {
    Buffer text;
    EmitModule em;
    IAsmOperand ops[1];
    buffer_init(&text);
    emit_module_init(&em);
    ops[0] = make_reg(0, 8);
    fakecc_clear_error();
    fakecc_set_quiet(1);
    int rc = iasm_emit_x86(&text, &em, "movq %1111111111, %0", ops, 1, "t.c", 4, 1);
    fakecc_set_quiet(0);
    T_ASSERT_EQ_INT(rc, -1);
    T_ASSERT(!fakecc_had_error());
    emit_module_free(&em);
    buffer_free(&text);
    fakecc_clear_error();
}

static void test_out_of_range_operand_index(void) {
    Buffer text;
    EmitModule em;
    IAsmOperand ops[1];
    buffer_init(&text);
    emit_module_init(&em);
    ops[0] = make_reg(0, 8);
    fakecc_clear_error();
    fakecc_set_quiet(1);
    int rc = iasm_emit_x86(&text, &em, "movq %1, %0", ops, 1, "t.c", 2, 1);
    fakecc_set_quiet(0);
    T_ASSERT_EQ_INT(rc, -1);
    T_ASSERT(!fakecc_had_error());
    emit_module_free(&em);
    buffer_free(&text);
    fakecc_clear_error();
}

/* Digit string just above INT_MAX must not wrap in signed int accumulation. */
static void test_index_beyond_int_max(void) {
    Buffer text;
    EmitModule em;
    IAsmOperand ops[1];
    buffer_init(&text);
    emit_module_init(&em);
    ops[0] = make_reg(0, 8);
    fakecc_clear_error();
    fakecc_set_quiet(1);
    int rc = iasm_emit_x86(&text, &em, "movq %2147483648, %0", ops, 1, "t.c", 3, 1);
    fakecc_set_quiet(0);
    T_ASSERT_EQ_INT(rc, -1);
    T_ASSERT(!fakecc_had_error());
    emit_module_free(&em);
    buffer_free(&text);
    fakecc_clear_error();
    (void)INT_MAX;
}

int main(void) {
    test_valid_operand_index();
    test_overflowing_operand_index();
    test_out_of_range_operand_index();
    test_index_beyond_int_max();
    return t_finalize();
}
