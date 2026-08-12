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
 * it has no side effects.  Side-effectful: RETURN, STORE, BR, CBR, LABEL,
 * ALLOCA (mem2reg-invariant marker, preserved as no-op codegen), CALL
 * (may have arbitrary side effects). */
static int has_side_effect(IROpcode op) {
    return op == IR_RETURN || op == IR_STORE || op == IR_STORE_PTR ||
           op == IR_BR || op == IR_CBR || op == IR_LABEL ||
           op == IR_ALLOCA || op == IR_CALL;
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
        case IR_MOD:
        case IR_BAND:
        case IR_BOR:
        case IR_BXOR:
        case IR_SHL:
        case IR_SHR: {
            int lf, rf;
            int lv = const_value(&fn->insts, inst->a, &lf);
            int rv = const_value(&fn->insts, inst->b, &rf);
            if (!lf || !rf) break;
            /* Avoid UB in constant folding: shift by negative or >= width,
             * and division/modulo by zero. */
            if (inst->op == IR_SHL || inst->op == IR_SHR) {
                if (rv < 0 || rv >= 32) continue;
            }
            int result;
            switch (inst->op) {
            case IR_ADD:  result = lv + rv; break;
            case IR_SUB:  result = lv - rv; break;
            case IR_MUL:  result = lv * rv; break;
            case IR_DIV:  if (rv == 0) continue; result = lv / rv; break;
            case IR_MOD:  if (rv == 0) continue; result = lv % rv; break;
            case IR_BAND: result = lv & rv; break;
            case IR_BOR:  result = lv | rv; break;
            case IR_BXOR: result = lv ^ rv; break;
            case IR_SHL:  result = lv << rv; break;
            case IR_SHR:  result = lv >> rv; break;
            default: continue;
            }
            inst->op = IR_CONST;
            inst->a = -1;
            inst->b = -1;
            inst->imm = result;
            changed = 1;
            break;
        }
        case IR_NEG:
        case IR_BNOT: {
            int f;
            int v = const_value(&fn->insts, inst->a, &f);
            if (!f) break;
            int is_neg = (inst->op == IR_NEG);
            inst->op = IR_CONST;
            inst->a = -1;
            inst->b = -1;
            inst->imm = is_neg ? -v : ~v;
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
    /* Mark which dst values are used as a source operand.
     * IR_CBR's `b` field is a label id, not a value; IR_LABEL/IR_BR have
     * no value operands at all.  IR_CALL has additional args in call_args. */
    char *used = xmalloc(fn->next_value_id * sizeof(char));
    memset(used, 0, fn->next_value_id * sizeof(char));
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_LABEL || inst->op == IR_BR) continue;
        if (inst->a >= 0 && inst->a < fn->next_value_id) used[inst->a] = 1;
        if (inst->op != IR_CBR &&
            inst->b >= 0 && inst->b < fn->next_value_id) used[inst->b] = 1;
        if (inst->op == IR_CALL) {
            for (int k = 0; k < inst->call_nargs; k++) {
                int v = inst->call_args[k];
                if (v >= 0 && v < fn->next_value_id) used[v] = 1;
            }
            /* Indirect calls use call_callee — keep it alive. */
            if (inst->call_callee >= 0 && inst->call_callee < fn->next_value_id)
                used[inst->call_callee] = 1;
        }
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

    /* Assign new ids in first-seen order.
     * Note: IR_CBR's `b` field is a label id (not a value); IR_LABEL/IR_BR
     * carry no value operands.  IR_CALL has additional value uses in
     * call_args.  Skip these when walking. */
    int next = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_LABEL || inst->op == IR_BR) continue;
        if (inst->dst >= 0 && map[inst->dst] == -1)
            map[inst->dst] = next++;
        if (inst->a >= 0 && map[inst->a] == -1)
            map[inst->a] = next++;
        if (inst->op != IR_CBR && inst->b >= 0 && map[inst->b] == -1)
            map[inst->b] = next++;
        if (inst->op == IR_CALL) {
            if (inst->call_callee >= 0 && map[inst->call_callee] == -1)
                map[inst->call_callee] = next++;
            for (int k = 0; k < inst->call_nargs; k++) {
                int v = inst->call_args[k];
                if (v >= 0 && map[v] == -1) map[v] = next++;
            }
        }
    }

    /* Rewrite (skipping label/branch operand fields). */
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_LABEL || inst->op == IR_BR) continue;
        if (inst->dst >= 0) inst->dst = map[inst->dst];
        if (inst->a >= 0) inst->a = map[inst->a];
        if (inst->op != IR_CBR && inst->b >= 0) inst->b = map[inst->b];
        if (inst->op == IR_CALL) {
            if (inst->call_callee >= 0)
                inst->call_callee = map[inst->call_callee];
            for (int k = 0; k < inst->call_nargs; k++) {
                int v = inst->call_args[k];
                if (v >= 0) inst->call_args[k] = map[v];
            }
        }
    }

    /* Remap per-value metadata (width / signedness / float) through the same
     * map so codegen reads correct type info for the compacted ids. */
    if (fn->value_meta_cap > 0) {
        int *old_width = fn->value_width;
        int *old_unsigned = fn->value_is_unsigned;
        int *old_float = fn->value_is_float;
        /* The metadata arrays only cover value ids [0, value_meta_cap).
         * mem2reg (and other passes) can raise next_value_id past that cap
         * by emitting φ values whose metadata is never set, so the old
         * arrays may be shorter than next_value_id.  Grow them (zero-filled)
         * before the copy below, otherwise the copy reads out of bounds and
         * strews heap garbage through the new arrays — which later misleads
         * regalloc's classifier and crashes codegen. */
        if (fn->next_value_id > fn->value_meta_cap) {
            int old_cap = fn->value_meta_cap;
            int grow = old_cap ? old_cap : 16;
            while (grow < fn->next_value_id) grow *= 2;
            old_width = xrealloc(old_width, grow * sizeof(int));
            old_unsigned = xrealloc(old_unsigned, grow * sizeof(int));
            old_float = xrealloc(old_float, grow * sizeof(int));
            for (int i = old_cap; i < grow; i++) {
                old_width[i] = 4;
                old_unsigned[i] = 0;
                old_float[i] = 0;
            }
            fn->value_meta_cap = grow;
        }
        fn->value_width = xmalloc(next * sizeof(int));
        fn->value_is_unsigned = xmalloc(next * sizeof(int));
        fn->value_is_float = xmalloc(next * sizeof(int));
        fn->value_meta_cap = next;
        for (int nv = 0; nv < next; nv++) {
            fn->value_width[nv] = 4;
            fn->value_is_unsigned[nv] = 0;
            fn->value_is_float[nv] = 0;
        }
        for (int ov = 0; ov < fn->next_value_id; ov++) {
            int nv = map[ov];
            if (nv < 0) continue;
            fn->value_width[nv] = old_width[ov];
            fn->value_is_unsigned[nv] = old_unsigned[ov];
            fn->value_is_float[nv] = old_float[ov];
        }
        free(old_width);
        free(old_unsigned);
        free(old_float);
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
