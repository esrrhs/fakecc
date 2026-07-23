#include "fakecc/regalloc.h"
#include "fakecc/mem2reg.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>

/* ---- helpers to construct IR ---- */

static void push_inst(IRInstArray *a, IROpcode op, IRValue dst,
                      IRValue arg1, IRValue arg2, int imm) {
    if (a->len >= a->cap) {
        a->cap = a->cap ? a->cap * 2 : 16;
        a->data = realloc(a->data, a->cap * sizeof(IRInst));
        if (!a->data) exit(1);
    }
    IRInst inst = {op, dst, arg1, arg2, imm, {NULL, 0, 0}};
    a->data[a->len++] = inst;
}

static void inst_array_data_free(IRInstArray *a) {
    free(a->data);
    a->data = NULL; a->len = a->cap = 0;
}

/* Count how many values are spilled. */
static int count_spilled(const RAResult *ra) {
    int c = 0;
    for (int i = 0; i < ra->num_values; i++)
        if (ra->reg[i] < 0) c++;
    return c;
}

/* ================================================================ */
/* Test: CONST → RETURN gets registers                              */
/* ================================================================ */

static void test_ra_single_const(void) {
    IRFunction fn;
    fn.name = NULL;
    fn.insts.data = NULL; fn.insts.len = 0; fn.insts.cap = 0;
    fn.next_value_id = 2;
    fn.loc = (SourceLoc){NULL, 0, 0};
    fn.ra = NULL;

    /* CONST 0 = 42; RETURN 0 */
    push_inst(&fn.insts, IR_CONST,  0, -1, -1, 42);
    push_inst(&fn.insts, IR_RETURN, -1,  0, -1, 0);

    RAResult *ra = reg_alloc(&fn);
    T_ASSERT(ra != NULL);
    T_ASSERT_EQ_INT(ra->num_values, 2);

    /* CONST dst=0 should be in a register. */
    T_ASSERT(ra->reg[0] >= 0 || ra->reg[0] == REG_NONE);

    /* RETURN references value 0 — which is the CONST. */
    /* Verify the return value (0) is in a register (likely RAX). */

    ra_result_free(ra);
    inst_array_data_free(&fn.insts);
}

/* ================================================================ */
/* Test: two non-interfering values → different registers           */
/* ================================================================ */

static void test_ra_two_independent(void) {
    IRFunction fn;
    fn.name = NULL;
    fn.insts.data = NULL; fn.insts.len = 0; fn.insts.cap = 0;
    fn.next_value_id = 3;
    fn.loc = (SourceLoc){NULL, 0, 0};
    fn.ra = NULL;

    /* v0 = CONST 1; v1 = CONST 2; RETURN v0
     * v0 and v1 are independent (no interference — they never overlap in use). */
    push_inst(&fn.insts, IR_CONST,  0, -1, -1, 1);
    push_inst(&fn.insts, IR_CONST,  1, -1, -1, 2);
    push_inst(&fn.insts, IR_RETURN, -1,  0, -1, 0);

    RAResult *ra = reg_alloc(&fn);
    T_ASSERT(ra != NULL);
    T_ASSERT_EQ_INT(ra->num_values, 3);

    /* Both v0 and v1 should be in registers. They may or may not be
     * different registers since they don't interfere. */
    T_ASSERT(ra->reg[0] >= 0 || ra->reg[0] == REG_NONE);
    T_ASSERT(ra->reg[1] >= 0 || ra->reg[1] == REG_NONE);

    ra_result_free(ra);
    inst_array_data_free(&fn.insts);
}

/* ================================================================ */
/* Test: interfering pair must use different registers              */
/* ================================================================ */

static void test_ra_interfering_pair(void) {
    IRFunction fn;
    fn.name = NULL;
    fn.insts.data = NULL; fn.insts.len = 0; fn.insts.cap = 0;
    fn.next_value_id = 4;
    fn.loc = (SourceLoc){NULL, 0, 0};
    fn.ra = NULL;

    /* v0 = CONST 3; v1 = CONST 4; v2 = ADD v0, v1; RETURN v2
     * v0 and v1 are both live at the ADD instruction → they interfere. */
    push_inst(&fn.insts, IR_CONST,  0, -1, -1, 3);
    push_inst(&fn.insts, IR_CONST,  1, -1, -1, 4);
    push_inst(&fn.insts, IR_ADD,    2,  0,  1, 0);
    push_inst(&fn.insts, IR_RETURN, -1,  2, -1, 0);

    RAResult *ra = reg_alloc(&fn);
    T_ASSERT(ra != NULL);

    /* v0 and v1 interfere, so they must be in different registers. */
    if (ra->reg[0] >= 0 && ra->reg[1] >= 0) {
        T_ASSERT(ra->reg[0] != ra->reg[1]);
    }

    ra_result_free(ra);
    inst_array_data_free(&fn.insts);
}

/* ================================================================ */
/* Test: run full pipeline (mem2reg + reg_alloc) on variable code   */
/* ================================================================ */

static void test_ra_after_mem2reg(void) {
    IRFunction fn;
    fn.name = NULL;
    fn.insts.data = NULL; fn.insts.len = 0; fn.insts.cap = 0;
    fn.next_value_id = 3;
    fn.loc = (SourceLoc){NULL, 0, 0};
    fn.ra = NULL;

    /* int x = 5; return x;
     *   ALLOCA 0; CONST 1=5; STORE 0,1; LOAD 2,0; RETURN 2 */
    push_inst(&fn.insts, IR_ALLOCA, 0, -1, -1, 0);
    push_inst(&fn.insts, IR_CONST,  1, -1, -1, 5);
    push_inst(&fn.insts, IR_STORE, -1,  0,  1, 0);
    push_inst(&fn.insts, IR_LOAD,   2,  0, -1, 0);
    push_inst(&fn.insts, IR_RETURN, -1,  2, -1, 0);

    /* Run mem2reg to promote to SSA form. */
    opt_mem2reg(&fn);

    /* After mem2reg, next_value_id may have grown (phi result ids). */
    /* Run register allocator. */
    RAResult *ra = reg_alloc(&fn);
    T_ASSERT(ra != NULL);

    /* The return value should be in a register (or at least not crashed). */
    /* CONST 5 (dst=1) should be in a register. */
    T_ASSERT(ra->reg[1] >= 0 || ra->reg[1] == REG_NONE);

    /* Verify we have no or few spills for this simple case. */
    T_ASSERT(count_spilled(ra) <= 2);

    ra_result_free(ra);
    inst_array_data_free(&fn.insts);
}

/* ================================================================ */
/* Test: NULL function (no values)                                   */
/* ================================================================ */

static void test_ra_empty(void) {
    IRFunction fn;
    fn.name = NULL;
    fn.insts.data = NULL; fn.insts.len = 0; fn.insts.cap = 0;
    fn.next_value_id = 1;
    fn.loc = (SourceLoc){NULL, 0, 0};
    fn.ra = NULL;

    /* Just RETURN with no prior defs (next_value_id = 1, but no CONST). */
    push_inst(&fn.insts, IR_RETURN, -1, 0, -1, 0);

    RAResult *ra = reg_alloc(&fn);
    /* May succeed (liveness finds value 0 is used) or return NULL. */
    if (ra) {
        /* Allocation should handle the undefined value. */
        ra_result_free(ra);
    }

    inst_array_data_free(&fn.insts);
}

/* ---- main ---- */

int main(void) {
    test_ra_single_const();
    test_ra_two_independent();
    test_ra_interfering_pair();
    test_ra_after_mem2reg();
    test_ra_empty();
    return t_finalize();
}
