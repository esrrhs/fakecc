#include "fakecc/opt.h"
#include "fakecc/cfg.h"
#include "fakecc/domtree.h"
#include "fakecc/phi.h"
#include "fakecc/mem2reg.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/* Optimization pipeline                                                */
/*                                                                      */
/* Pipeline order (per function):                                       */
/*   opt_mem2reg  →  [ opt_constfold | opt_peephole | opt_dce ]*  →     */
/*   opt_renumber                                                        */
/*                                                                      */
/* φ nodes are an INTERNAL representation used only inside opt_mem2reg; */
/* the flat IR never contains φ. After optimization the flat IR holds    */
/* only CONST/ADD/.../NEG/COPY/RETURN (and LABEL/BR/CBR for Slice 4).   */
/* ================================================================== */

/* ------------------------------------------------------------------ */
/* Small generic helpers (FakeCC style: malloc + exit(1) on failure)  */
/* ------------------------------------------------------------------ */

static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    return p;
}

static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    return q;
}

/* ================================================================== */
/* IRInstArray helpers (rebuild for write-back)                        */
/* ================================================================== */

static void inst_array_init(IRInstArray *a) {
    a->data = NULL; a->len = 0; a->cap = 0;
}

static void inst_array_push(IRInstArray *a, IRInst inst) {
    if (a->len >= a->cap) {
        a->cap = a->cap ? a->cap * 2 : 16;
        a->data = xrealloc(a->data, a->cap * sizeof(IRInst));
    }
    a->data[a->len++] = inst;
}

static void inst_array_free(IRInstArray *a) {
    free(a->data);
    a->data = NULL; a->len = 0; a->cap = 0;
}

/* ================================================================== */
/* opt_constfold — fold all-constant operands                         */
/* ================================================================== */

/* Look up the immediate of a CONST defining `v`, or 0 if not a known const. */
static int const_value(const IRInstArray *insts, IRValue v, int *found) {
    *found = 0;
    if (v < 0) return 0;
    /* A value is a constant if its single defining instruction is IR_CONST. */
    for (size_t i = 0; i < insts->len; i++) {
        IRInst *inst = &insts->data[i];
        if (inst->dst == v && inst->op == IR_CONST) {
            *found = 1;
            return inst->imm;
        }
    }
    return 0;
}

static int opt_constfold(IRFunction *fn) {
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
/* opt_dce — dead code elimination                                    */
/* ================================================================== */

/* A value-producing instruction is dead if its result is never used and
 * it has no side effects. Side-effectful: RETURN, STORE, BR, CBR. */
static int has_side_effect(IROpcode op) {
    return op == IR_RETURN || op == IR_STORE || op == IR_BR || op == IR_CBR;
}

static int opt_dce(IRFunction *fn) {
    /* mark which dst values are used as a source operand */
    char *used = xmalloc(fn->next_value_id * sizeof(char));
    memset(used, 0, fn->next_value_id * sizeof(char));
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->a >= 0) used[inst->a] = 1;
        if (inst->b >= 0) used[inst->b] = 1;
    }

    IRInstArray out;
    inst_array_init(&out);
    int changed = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (!has_side_effect(inst->op) && inst->dst >= 0 &&
            inst->dst < fn->next_value_id && !used[inst->dst]) {
            changed = 1;
            continue; /* drop it */
        }
        inst_array_push(&out, *inst);
    }
    free(used);

    if (changed) {
        inst_array_free(&fn->insts);
        fn->insts = out;
    } else {
        inst_array_free(&out);
    }
    return changed;
}

/* ================================================================== */
/* opt_peephole — local simplifications                               */
/* ================================================================== */

static int opt_peephole(IRFunction *fn) {
    int changed = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_ADD || inst->op == IR_SUB || inst->op == IR_MUL) {
            int lf, rf;
            int lv = const_value(&fn->insts, inst->a, &lf);
            int rv = const_value(&fn->insts, inst->b, &rf);
            if (inst->op == IR_ADD) {
                if (lf && lv == 0) { inst->op = IR_COPY; inst->b = -1; changed = 1; break; }
                if (rf && rv == 0) { inst->op = IR_COPY; inst->b = -1; changed = 1; break; }
            } else if (inst->op == IR_SUB) {
                if (rf && rv == 0) { inst->op = IR_COPY; inst->b = -1; changed = 1; break; }
                /* 0 - x → -x */
                if (lf && lv == 0) { inst->op = IR_NEG; inst->b = -1; changed = 1; break; }
            } else if (inst->op == IR_MUL) {
                if (lf && lv == 0) { inst->op = IR_CONST; inst->a = -1; inst->b = -1; inst->imm = 0; changed = 1; break; }
                if (rf && rv == 0) { inst->op = IR_CONST; inst->a = -1; inst->b = -1; inst->imm = 0; changed = 1; break; }
                if (lf && lv == 1) { inst->op = IR_COPY; inst->a = inst->b; inst->b = -1; changed = 1; break; }
                if (rf && rv == 1) { inst->op = IR_COPY; inst->b = -1; changed = 1; break; }
            }
        }
    }
    return changed;
}

/* ================================================================== */
/* opt_renumber — compact value ids to tighten the stack frame        */
/* ================================================================== */

static void opt_renumber(IRFunction *fn) {
    if (fn->next_value_id <= 0) return;
    int *map = xmalloc(fn->next_value_id * sizeof(int));
    for (int i = 0; i < fn->next_value_id; i++) map[i] = -1;

    /* assign new ids in first-seen order */
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

    /* rewrite */
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
/* opt_mem2reg — promote alloca/load/store to SSA registers           */
/*                                                                      */
/* Classic algorithm:                                                    */
/*   1. Build CFG + dominator tree + dominance frontiers                */
/*   2. For each promotable alloca:                                     */
/*        a. Insert φ at DF of blocks containing stores                  */
/*        b. Rename: dominator-tree DFS, replace LOAD with COPY,        */
/*           mark STORE/ALLOCA dead, push/pop reaching values            */
/*   3. Resolve φ → COPY in predecessors                               */
/*   4. Rebuild instruction array without dead instructions             */
/* ================================================================== */

static void opt_mem2reg(IRFunction *fn) {
    (void)domtree_dominates; /* used in future dominator queries */
    CFG cfg;
    cfg_build(&cfg, &fn->insts);

    /* Per-block φ storage — the CFG module does not carry φ nodes. */
    BlockPhiInfo *block_phis = xmalloc(cfg.num * sizeof(BlockPhiInfo));
    memset(block_phis, 0, cfg.num * sizeof(BlockPhiInfo));

    DomTree dt;
    domtree_build(&dt, &cfg);

    const IRInstArray *insts = &fn->insts;

    /* ---- 1. Identify promotable allocas ---- */
    /* Collect all ALLOCA dst values. Currently every alloca is promotable
     * (no address-taking in the language). */
    int *alloca_slots = NULL; size_t num_alloca = 0, cap_alloca = 0;
    for (size_t i = 0; i < insts->len; i++) {
        if (insts->data[i].op == IR_ALLOCA) {
            if (num_alloca >= cap_alloca) {
                cap_alloca = cap_alloca ? cap_alloca * 2 : 8;
                alloca_slots = xrealloc(alloca_slots, cap_alloca * sizeof(int));
            }
            alloca_slots[num_alloca++] = insts->data[i].dst;
        }
    }
    if (num_alloca == 0) { free(block_phis); domtree_free(&dt); cfg_free(&cfg); return; }

    /* ---- 2. φ placement ---- */
    /* Build block_stores bitmap: block_stores[bi][ai] = has store? */
    char **block_stores = xmalloc(cfg.num * sizeof(char *));
    for (size_t bi = 0; bi < cfg.num; bi++) {
        block_stores[bi] = xmalloc(num_alloca * sizeof(char));
        memset(block_stores[bi], 0, num_alloca * sizeof(char));
    }
    for (size_t i = 0; i < insts->len; i++) {
        if (insts->data[i].op != IR_STORE) continue;
        for (size_t ai = 0; ai < num_alloca; ai++) {
            if (insts->data[i].a == alloca_slots[ai]) {
                for (size_t bi = 0; bi < cfg.num; bi++) {
                    if (i >= cfg.blocks[bi].start && i < cfg.blocks[bi].end) {
                        block_stores[bi][ai] = 1;
                        break;
                    }
                }
                break;
            }
        }
    }

    /* Call mem2reg_place_phis to insert φ nodes at dominance frontiers. */
    BlockPhiInfo *bp = mem2reg_place_phis(
        &cfg, &dt, alloca_slots, num_alloca,
        (const char **)block_stores, &fn->next_value_id);

    /* Free block_stores (no longer needed). */
    for (size_t bi = 0; bi < cfg.num; bi++) free(block_stores[bi]);
    free(block_stores);

    /* Replace block_phis with the result from mem2reg_place_phis. */
    free(block_phis);
    block_phis = bp;

    /* ---- 3. Rename (delegated to mem2reg module) ---- */
    char *dead = NULL;
    mem2reg_rename(fn, &cfg, &dt, alloca_slots, num_alloca, block_phis, &dead);
    /* For each block in order:
     *   - emit non-dead instructions
     *   - before the block's terminator, emit φ-resolution COPYs
     *     (for each successor S with φs, emit COPYs for the predecessor B)
     *   - emit the terminator
     */
    {
        IRInstArray out;
        inst_array_init(&out);
        for (size_t bi = 0; bi < cfg.num; bi++) {
            CFGBlock *blk = &cfg.blocks[bi];
            /* Identify the terminator index (last inst if BR/CBR/RETURN) */
            size_t term_idx = blk->end; /* no terminator (fall-through) */
            if (blk->end > blk->start) {
                IROpcode last_op = fn->insts.data[blk->end - 1].op;
                if (last_op == IR_BR || last_op == IR_CBR || last_op == IR_RETURN)
                    term_idx = blk->end - 1;
            }

            /* Emit body (non-dead, before terminator) */
            for (size_t i = blk->start; i < term_idx; i++) {
                if (!dead[i]) inst_array_push(&out, fn->insts.data[i]);
            }

            /* Emit φ-resolution COPYs for each successor */
            for (size_t si = 0; si < blk->num_succs; si++) {
                int s = blk->succs[si];
                for (size_t phi_i = 0; phi_i < block_phis[s].num_phis; phi_i++) {
                    IRPhi *phi = &block_phis[s].phis[phi_i];
                    /* Find the arg for predecessor b */
                    for (size_t ai = 0; ai < phi->num_args; ai++) {
                        if (phi->args[ai].pred == (int)bi) {
                            IRInst copy;
                            copy.op = IR_COPY;
                            copy.dst = phi->dst;
                            copy.a = phi->args[ai].val;
                            copy.b = -1;
                            copy.imm = 0;
                            copy.loc = phi->loc;
                            inst_array_push(&out, copy);
                            break;
                        }
                    }
                }
            }

            /* Emit terminator */
            if (term_idx < blk->end && !dead[term_idx])
                inst_array_push(&out, fn->insts.data[term_idx]);
            /* Also emit any instructions after terminator in this block
             * (shouldn't happen with well-formed IR, but handle gracefully) */
        }
        inst_array_free(&fn->insts);
        fn->insts = out;
    }

    /* Cleanup */
    for (size_t bi = 0; bi < cfg.num; bi++) {
        for (size_t j = 0; j < block_phis[bi].num_phis; j++)
            free(block_phis[bi].phis[j].args);
        free(block_phis[bi].phis);
    }
    free(block_phis);
    free(alloca_slots);
    free(dead);
    domtree_free(&dt);
    cfg_free(&cfg);
}

/* ================================================================== */
/* opt — top-level driver                                             */
/* ================================================================== */

void opt(IRModule *ir) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        IRFunction *fn = &ir->functions.data[i];

        /* mem2reg runs once to promote alloca/load/store to SSA form. */
        opt_mem2reg(fn);

        /* iterative clean-up passes run to a fixed point. */
        for (;;) {
            int changed = 0;
            changed |= opt_constfold(fn);
            changed |= opt_peephole(fn);
            changed |= opt_dce(fn);
            if (!changed) break;
        }
        opt_renumber(fn);
    }
}
