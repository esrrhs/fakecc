#include "fakecc/scalar_opt.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/* Internal helpers                                                     */
/* ================================================================== */

/* Look up the immediate of a CONST defining `v`, or 0 if not a known const.
 * Float CONSTs keep their payload in float_imm, not imm, so they must not be
 * reported as integer constants. */
static int64_t const_value(const IRInstArray *insts, IRValue v, int *found) {
    *found = 0;
    if (v < 0) return 0;
    for (size_t i = 0; i < insts->len; i++) {
        IRInst *inst = &insts->data[i];
        if (inst->dst == v && inst->op == IR_CONST) {
            if (inst->is_float) return 0;
            *found = 1;
            return inst->imm;
        }
    }
    return 0;
}

/* Reduce a folded result to what the hardware would leave in a value of this
 * width and signedness — codegen masks every arithmetic result the same way,
 * and a fold that skips it disagrees with the unfolded code. */
static int64_t trunc_to_width(int64_t v, int width, int is_unsigned) {
    if (width >= 8 || width <= 0) return v;
    uint64_t mask = (width == 1) ? 0xFFULL : (width == 2) ? 0xFFFFULL : 0xFFFFFFFFULL;
    uint64_t u = (uint64_t)v & mask;
    if (is_unsigned) return (int64_t)u;
    uint64_t sign = (mask >> 1) + 1;
    return (u & sign) ? (int64_t)(u | ~mask) : (int64_t)u;
}

/* A value-producing instruction is dead if its result is never used and
 * it has no side effects.  Side-effectful: RETURN, STORE, BR, CBR, LABEL,
 * ALLOCA (mem2reg-invariant marker, preserved as no-op codegen), CALL
 * (may have arbitrary side effects). */
static int has_side_effect(IROpcode op) {
    return op == IR_RETURN || op == IR_STORE || op == IR_STORE_PTR ||
           op == IR_BR || op == IR_CBR || op == IR_LABEL ||
           op == IR_ALLOCA || op == IR_CALL || op == IR_PARAM ||
           op == IR_VADD || op == IR_VSUB || op == IR_VMUL || op == IR_VDIV ||
           op == IR_VBAND || op == IR_VBOR || op == IR_VBXOR;
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
            int64_t lv = const_value(&fn->insts, inst->a, &lf);
            int64_t rv = const_value(&fn->insts, inst->b, &rf);
            if (!lf || !rf) break;
            if (inst->is_float) break;
            int w = inst->width ? inst->width : 4;
            int uns = inst->is_unsigned ? 1 : 0;
            /* Fold in the operand domain: an unsigned divide folded as a signed
             * one gives a different answer than the code it replaces. */
            uint64_t lu = (uint64_t)trunc_to_width(lv, w, uns);
            uint64_t ru = (uint64_t)trunc_to_width(rv, w, uns);
            int64_t ls = trunc_to_width(lv, w, uns), rs = trunc_to_width(rv, w, uns);
            /* Avoid UB in constant folding: shift by negative or >= width,
             * and division/modulo by zero. */
            if (inst->op == IR_SHL || inst->op == IR_SHR) {
                if (rs < 0 || rs >= w * 8) continue;
            }
            int64_t result;
            switch (inst->op) {
            case IR_ADD:  result = (int64_t)(lu + ru); break;
            case IR_SUB:  result = (int64_t)(lu - ru); break;
            case IR_MUL:  result = (int64_t)(lu * ru); break;
            case IR_DIV:
                if (rs == 0) continue;
                if (uns) result = (int64_t)(lu / ru);
                else { if (rs == -1) continue; result = ls / rs; }
                break;
            case IR_MOD:
                if (rs == 0) continue;
                if (uns) result = (int64_t)(lu % ru);
                else { if (rs == -1) continue; result = ls % rs; }
                break;
            case IR_BAND: result = (int64_t)(lu & ru); break;
            case IR_BOR:  result = (int64_t)(lu | ru); break;
            case IR_BXOR: result = (int64_t)(lu ^ ru); break;
            case IR_SHL:  result = (int64_t)(lu << rs); break;
            case IR_SHR:  result = uns ? (int64_t)(lu >> rs) : (ls >> rs); break;
            default: continue;
            }
            inst->op = IR_CONST;
            inst->a = -1;
            inst->b = -1;
            inst->imm = trunc_to_width(result, w, uns);
            changed = 1;
            break;
        }
        case IR_NEG:
        case IR_BNOT: {
            int f;
            int64_t v = const_value(&fn->insts, inst->a, &f);
            if (!f) break;
            if (inst->is_float) break;
            int w = inst->width ? inst->width : 4;
            int uns = inst->is_unsigned ? 1 : 0;
            uint64_t uv = (uint64_t)trunc_to_width(v, w, uns);
            int is_neg = (inst->op == IR_NEG);
            inst->op = IR_CONST;
            inst->a = -1;
            inst->b = -1;
            inst->imm = trunc_to_width((int64_t)(is_neg ? -uv : ~uv), w, uns);
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
        /* A debug marker must not keep a value alive, or -g would suppress
         * dead-code elimination and change the generated code. */
        if (inst->op == IR_DBG_VALUE) continue;
        if (inst->a >= 0 && inst->a < fn->next_value_id) used[inst->a] = 1;
        if (inst->op != IR_CBR && inst->op != IR_CALL &&
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
        if (inst->op == IR_ADD || inst->op == IR_SUB || inst->op == IR_MUL ||
            inst->op == IR_BAND || inst->op == IR_BOR || inst->op == IR_BXOR) {
            if (inst->is_float) continue;
            int lf, rf;
            int64_t lv = const_value(&fn->insts, inst->a, &lf);
            int64_t rv = const_value(&fn->insts, inst->b, &rf);
            if (inst->a >= 0 && inst->a == inst->b) {
                if (inst->op == IR_SUB || inst->op == IR_BXOR) {
                    inst->op = IR_CONST; inst->a = -1; inst->b = -1; inst->imm = 0; changed = 1; continue;
                }
            }
            if (inst->op == IR_ADD) {
                if (lf && lv == 0) { inst->op = IR_COPY; inst->a = inst->b; inst->b = -1; changed = 1; }
                else if (rf && rv == 0) { inst->op = IR_COPY; inst->b = -1; changed = 1; }
            } else if (inst->op == IR_SUB) {
                if (rf && rv == 0) { inst->op = IR_COPY; inst->b = -1; changed = 1; }
                else if (lf && lv == 0) { inst->op = IR_NEG; inst->a = inst->b; inst->b = -1; changed = 1; }
            } else if (inst->op == IR_MUL) {
                if (lf && lv == 0) { inst->op = IR_CONST; inst->a = -1; inst->b = -1; inst->imm = 0; changed = 1; }
                else if (rf && rv == 0) { inst->op = IR_CONST; inst->a = -1; inst->b = -1; inst->imm = 0; changed = 1; }
                else if (lf && lv == 1) { inst->op = IR_COPY; inst->a = inst->b; inst->b = -1; changed = 1; }
                else if (rf && rv == 1) { inst->op = IR_COPY; inst->b = -1; changed = 1; }
            } else if (inst->op == IR_BAND) {
                if (lf && lv == 0) { inst->op = IR_CONST; inst->a = -1; inst->b = -1; inst->imm = 0; changed = 1; }
                else if (rf && rv == 0) { inst->op = IR_CONST; inst->a = -1; inst->b = -1; inst->imm = 0; changed = 1; }
            } else if (inst->op == IR_BOR) {
                if (lf && lv == 0) { inst->op = IR_COPY; inst->a = inst->b; inst->b = -1; changed = 1; }
                else if (rf && rv == 0) { inst->op = IR_COPY; inst->b = -1; changed = 1; }
            } else if (inst->op == IR_BXOR) {
                if (lf && lv == 0) { inst->op = IR_COPY; inst->a = inst->b; inst->b = -1; changed = 1; }
                else if (rf && rv == 0) { inst->op = IR_COPY; inst->b = -1; changed = 1; }
            }
        }
    }
    return changed;
}

/* ================================================================== */
/* scalar_renumber — compact value ids to tighten the stack frame      */
/* ================================================================== */

void scalar_renumber(IRFunction *fn) {
    int max_vid = fn->next_value_id;
    for (size_t i = 0; i < fn->insts.len; i++) {
        const IRInst *inst = &fn->insts.data[i];
        if (inst->dst >= max_vid) max_vid = inst->dst + 1;
        if (inst->a >= max_vid) max_vid = inst->a + 1;
        if (inst->op != IR_CBR && inst->b >= max_vid) max_vid = inst->b + 1;
        if (inst->op == IR_CALL) {
            if (inst->call_callee >= max_vid) max_vid = inst->call_callee + 1;
            for (int k = 0; k < inst->call_nargs; k++) {
                if (k < IR_CALL_MAX_ARGS && inst->call_args[k] >= max_vid)
                    max_vid = inst->call_args[k] + 1;
            }
        }
    }
    if (max_vid <= 0) return;
    int *map = xmalloc((size_t)max_vid * sizeof(int));
    for (int i = 0; i < max_vid; i++) map[i] = -1;

    /* Assign new ids in first-seen order.
     * Note: IR_CBR's `b` field is a label id (not a value); IR_LABEL/IR_BR
     * carry no value operands.  IR_CALL has additional value uses in
     * call_args.  Skip these when walking. */
    int next = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_LABEL || inst->op == IR_BR) continue;
        /* Debug markers observe ids, they never introduce them: giving one a
         * fresh id would inflate the frame and change codegen under -g. */
        if (inst->op == IR_DBG_VALUE) continue;
        if (inst->dst >= 0 && inst->dst < max_vid && map[inst->dst] == -1)
            map[inst->dst] = next++;
        if (inst->a >= 0 && inst->a < max_vid && map[inst->a] == -1)
            map[inst->a] = next++;
        if (inst->op != IR_CBR && inst->b >= 0 && inst->b < max_vid && map[inst->b] == -1)
            map[inst->b] = next++;
        if (inst->op == IR_CALL) {
            if (inst->call_callee >= 0 && inst->call_callee < max_vid && map[inst->call_callee] == -1)
                map[inst->call_callee] = next++;
            for (int k = 0; k < inst->call_nargs; k++) {
                if (k >= IR_CALL_MAX_ARGS) break;
                int v = inst->call_args[k];
                if (v >= 0 && v < max_vid && map[v] == -1)
                    map[v] = next++;
            }
        }
    }

    /* Rewrite (skipping label/branch operand fields). */
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_LABEL || inst->op == IR_BR) continue;
        if (inst->op == IR_DBG_VALUE) {
            /* map[] is -1 when the definition was optimized away; the
             * variable then has no location over this range. */
            if (inst->a >= 0 && inst->a < max_vid) inst->a = map[inst->a];
            continue;
        }
        if (inst->dst >= 0 && inst->dst < max_vid) inst->dst = map[inst->dst];
        if (inst->a >= 0 && inst->a < max_vid) inst->a = map[inst->a];
        if (inst->op != IR_CBR && inst->b >= 0 && inst->b < max_vid) inst->b = map[inst->b];
        if (inst->op == IR_CALL) {
            if (inst->call_callee >= 0 && inst->call_callee < max_vid)
                inst->call_callee = map[inst->call_callee];
            for (int k = 0; k < inst->call_nargs; k++) {
                if (k >= IR_CALL_MAX_ARGS) break;
                int v = inst->call_args[k];
                if (v >= 0 && v < max_vid) inst->call_args[k] = map[v];
            }
        }
    }

    /* Stack-resident debug variables point at their IR_ALLOCA id, which the
     * compaction above moved.  A -1 here means the alloca was promoted away,
     * in which case IR_DBG_VALUE markers describe the variable instead. */
    for (size_t i = 0; i < fn->num_dbg_vars; i++) {
        int slot = fn->dbg_vars[i].alloca_ssa;
        fn->dbg_vars[i].alloca_ssa =
            (slot >= 0 && slot < max_vid) ? map[slot] : -1;
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
