#include "fakecc/scalar_opt.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/* Internal helpers                                                     */
/* ================================================================== */

/* Look up the immediate of a CONST defining `v`, or 0 if not a known const. */
static int const_value(const IRInstArray *insts, IRValue v, int *found) {
    *found = 0;
    if (v < 0) return 0;
    for (size_t i = 0; i < insts->len; i++) {
        IRInst *inst = &insts->data[i];
        if (inst->dst == v && inst->op == IR_CONST) {
            *found = 1;
            return inst->imm;
        }
    }
    return 0;
}

/* A value-producing instruction is dead if its result is never used and
 * it has no side effects.  Side-effectful: RETURN, STORE, BR, CBR. */
static int has_side_effect(IROpcode op) {
    return op == IR_RETURN || op == IR_STORE || op == IR_BR || op == IR_CBR;
}

/* ================================================================== */
/* scalar_constfold — fold all-constant operands                       */
/* ================================================================== */

int scalar_constfold(IRFunction *fn) {
    int changed = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        switch (inst->op) {
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
        case IR_MOD: {
            int lf, rf;
            int lv = const_value(&fn->insts, inst->a, &lf);
            int rv = const_value(&fn->insts, inst->b, &rf);
            if (!lf || !rf) break;
            int result;
            switch (inst->op) {
            case IR_ADD: result = lv + rv; break;
            case IR_SUB: result = lv - rv; break;
            case IR_MUL: result = lv * rv; break;
            case IR_DIV: if (rv == 0) continue; result = lv / rv; break;
            case IR_MOD: if (rv == 0) continue; result = lv % rv; break;
            default: continue;
            }
            inst->op = IR_CONST;
            inst->a = -1;
            inst->b = -1;
            inst->imm = result;
            changed = 1;
            break;
        }
        case IR_NEG: {
            int f;
            int v = const_value(&fn->insts, inst->a, &f);
            if (!f) break;
            inst->op = IR_CONST;
            inst->a = -1;
            inst->b = -1;
            inst->imm = -v;
            changed = 1;
            break;
        }
        default:
            break;
        }
    }
    return changed;
}

/* ================================================================== */
/* scalar_dce — dead code elimination                                  */
/* ================================================================== */

int scalar_dce(IRFunction *fn) {
    /* Mark which dst values are used as a source operand. */
    char *used = xmalloc(fn->next_value_id * sizeof(char));
    memset(used, 0, fn->next_value_id * sizeof(char));
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->a >= 0) used[inst->a] = 1;
        if (inst->b >= 0) used[inst->b] = 1;
    }

    /* Rebuild dropping unused side-effect-free instructions. */
    IRInstArray out;
    out.data = NULL; out.len = 0; out.cap = 0;
    int changed = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (!has_side_effect(inst->op) && inst->dst >= 0 &&
            inst->dst < fn->next_value_id && !used[inst->dst]) {
            changed = 1;
            continue;
        }
        if (out.len >= out.cap) {
            out.cap = out.cap ? out.cap * 2 : 16;
            out.data = xrealloc(out.data, out.cap * sizeof(IRInst));
        }
        out.data[out.len++] = *inst;
    }
    free(used);

    if (changed) {
        free(fn->insts.data);
        fn->insts = out;
    } else {
        free(out.data);
    }
    return changed;
}

/* ================================================================== */
/* scalar_peephole — local simplifications                             */
/* ================================================================== */

int scalar_peephole(IRFunction *fn) {
    int changed = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_ADD || inst->op == IR_SUB || inst->op == IR_MUL) {
            int lf, rf;
            int lv = const_value(&fn->insts, inst->a, &lf);
            int rv = const_value(&fn->insts, inst->b, &rf);
            if (inst->op == IR_ADD) {
                if (lf && lv == 0) { inst->op = IR_COPY; inst->b = -1; changed = 1; }
                else if (rf && rv == 0) { inst->op = IR_COPY; inst->b = -1; changed = 1; }
            } else if (inst->op == IR_SUB) {
                if (rf && rv == 0) { inst->op = IR_COPY; inst->b = -1; changed = 1; }
                else if (lf && lv == 0) { inst->op = IR_NEG; inst->b = -1; changed = 1; }
            } else if (inst->op == IR_MUL) {
                if (lf && lv == 0) { inst->op = IR_CONST; inst->a = -1; inst->b = -1; inst->imm = 0; changed = 1; }
                else if (rf && rv == 0) { inst->op = IR_CONST; inst->a = -1; inst->b = -1; inst->imm = 0; changed = 1; }
                else if (lf && lv == 1) { inst->op = IR_COPY; inst->a = inst->b; inst->b = -1; changed = 1; }
                else if (rf && rv == 1) { inst->op = IR_COPY; inst->b = -1; changed = 1; }
            }
        }
    }
    return changed;
}

/* ================================================================== */
/* scalar_renumber — compact value ids to tighten the stack frame      */
/* ================================================================== */

void scalar_renumber(IRFunction *fn) {
    if (fn->next_value_id <= 0) return;
    int *map = xmalloc(fn->next_value_id * sizeof(int));
    for (int i = 0; i < fn->next_value_id; i++) map[i] = -1;

    /* Assign new ids in first-seen order. */
    int next = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->dst >= 0 && map[inst->dst] == -1)
            map[inst->dst] = next++;
        if (inst->a >= 0 && map[inst->a] == -1)
            map[inst->a] = next++;
        if (inst->b >= 0 && map[inst->b] == -1)
            map[inst->b] = next++;
    }

    /* Rewrite. */
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->dst >= 0) inst->dst = map[inst->dst];
        if (inst->a >= 0) inst->a = map[inst->a];
        if (inst->b >= 0) inst->b = map[inst->b];
    }
    fn->next_value_id = next;
    free(map);
}

/* ================================================================== */
/* scalar_cleanup — iterative fixed-point loop + renumber              */
/* ================================================================== */

void scalar_cleanup(IRFunction *fn) {
    for (;;) {
        int changed = 0;
        changed |= scalar_constfold(fn);
        changed |= scalar_peephole(fn);
        changed |= scalar_dce(fn);
        if (!changed) break;
    }
    scalar_renumber(fn);
}
