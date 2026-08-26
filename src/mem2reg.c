#include "fakecc/phi.h"
#include "fakecc/cfg.h"
#include "fakecc/domtree.h"
#include "fakecc/mem2reg.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/* Block φ helpers                                                      */
/* ================================================================== */

/* Does this block already have a φ for the given alloca slot? */
static int block_has_phi_for(const BlockPhiInfo *bp, int alloca_slot) {
    for (size_t i = 0; i < bp->num_phis; i++)
        if (bp->phis[i].alloca_slot == alloca_slot) return 1;
    return 0;
}

/* Add a φ node to a block. Returns pointer to the new φ. */
static IRPhi *block_add_phi(BlockPhiInfo *bp, int alloca_slot,
                             IRValue dst, SourceLoc loc) {
    if (bp->num_phis >= bp->cap_phis) {
        bp->cap_phis = bp->cap_phis ? bp->cap_phis * 2 : 4;
        bp->phis = xrealloc(bp->phis, bp->cap_phis * sizeof(IRPhi));
    }
    IRPhi *phi = &bp->phis[bp->num_phis++];
    phi->dst = dst;
    phi->alloca_slot = alloca_slot;
    phi->args = NULL;
    phi->num_args = 0;
    phi->cap_args = 0;
    phi->loc = loc;
    return phi;
}

/* ================================================================== */
/* φ placement — iterative worklist (Cytron et al., 1991)              */
/* ================================================================== */

BlockPhiInfo *mem2reg_place_phis(
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    const char **block_stores,
    int *next_value_id)
{
    size_t n = cfg->num;

    BlockPhiInfo *bp = xmalloc(n * sizeof(BlockPhiInfo));
    memset(bp, 0, n * sizeof(BlockPhiInfo));

    if (num_alloca == 0) return bp;

    for (size_t ai = 0; ai < num_alloca; ai++) {
        int slot = alloca_slots[ai];

        /* Track which blocks are (or have become) definitions for this alloca.
         * Initially: blocks that contain a STORE to this alloca.  As φ nodes
         * are inserted the block becomes a new definition and propagates. */
        char *is_def = calloc(n, 1);
        if (!is_def) continue;

        /* Worklist — blocks whose DF still needs to be checked. */
        int *wl = NULL;
        size_t wl_len = 0, wl_cap = 0;

        /* Seed the worklist with initial def blocks (blocks with STOREs). */
        for (size_t bi = 0; bi < n; bi++) {
            if (block_stores[bi][ai]) {
                is_def[bi] = 1;
                if (wl_len >= wl_cap) {
                    wl_cap = wl_cap ? wl_cap * 2 : 4;
                    wl = xrealloc(wl, wl_cap * sizeof(int));
                }
                wl[wl_len++] = (int)bi;
            }
        }

        /* Iterative DF-based φ placement. */
        while (wl_len > 0) {
            int X = wl[--wl_len];
            for (size_t di = 0; di < dt->df_len[X]; di++) {
                int Y = dt->df[X][di];
                if (!block_has_phi_for(&bp[Y], slot)) {
                    /* Create a new SSA value for the φ result. */
                    IRValue phi_dst = (*next_value_id)++;
                    SourceLoc phi_loc = {NULL, 0, 0};
                    block_add_phi(&bp[Y], slot, phi_dst, phi_loc);

                    /* If Y is not already a def for this alloca, it
                     * becomes one now (the φ itself is a definition),
                     * and we must recursively check DF[Y]. */
                    if (!is_def[Y]) {
                        is_def[Y] = 1;
                        if (wl_len >= wl_cap) {
                            wl_cap = wl_cap ? wl_cap * 2 : 4;
                            wl = xrealloc(wl, wl_cap * sizeof(int));
                        }
                        wl[wl_len++] = Y;
                    }
                }
            }
        }

        free(wl);
        free(is_def);
    }

    return bp;
}

/* ================================================================== */
/* Cleanup                                                             */
/* ================================================================== */

void block_phi_info_free(BlockPhiInfo *bp, size_t num_blocks) {
    if (!bp) return;
    for (size_t i = 0; i < num_blocks; i++) {
        for (size_t j = 0; j < bp[i].num_phis; j++)
            free(bp[i].phis[j].args);
        free(bp[i].phis);
    }
    free(bp);
}

/* ================================================================== */
/* IRInstArray helpers (rebuild for writeback)                          */
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

static void inst_array_free_contents(IRInstArray *a) {
    free(a->data);
    a->data = NULL; a->len = 0; a->cap = 0;
}

/* ================================================================== */
/* Writeback — resolve φ to COPYs and rebuild flat IR                  */
/* ================================================================== */

/* Which source variable does this alloca slot stand for? Returns an index
 * into fn->dbg_vars, or -1 if the slot is compiler-generated. */
static int dbg_var_for_slot(const IRFunction *fn, int slot) {
    for (size_t i = 0; i < fn->num_dbg_vars; i++)
        if (fn->dbg_vars[i].alloca_ssa == slot) return (int)i;
    return -1;
}

/* Turn instruction `inst` into a marker saying "source variable `var` now
 * lives in SSA value `val`". */
static void make_dbg_value(IRInst *inst, int var, IRValue val) {
    inst->op = IR_DBG_VALUE;
    inst->dst = -1;
    inst->a = val;
    inst->b = -1;
    inst->imm = var;
    inst->call_name = NULL;
    inst->call_nargs = 0;
}

void mem2reg_writeback(
    IRFunction *fn,
    const CFG *cfg,
    BlockPhiInfo *block_phi_info,
    char *dead,
    int want_debug)
{
    IRInstArray out;
    inst_array_init(&out);

    for (size_t bi = 0; bi < cfg->num; bi++) {
        const CFGBlock *blk = &cfg->blocks[bi];

        /* A φ merges several definitions of one source variable, so on entry
         * to this block the variable lives in the φ result.  Record that. */
        if (want_debug) {
            for (size_t phi_i = 0; phi_i < block_phi_info[bi].num_phis; phi_i++) {
                IRPhi *phi = &block_phi_info[bi].phis[phi_i];
                int var = dbg_var_for_slot(fn, phi->alloca_slot);
                if (var < 0) continue;
                IRInst marker;
                memset(&marker, 0, sizeof(marker));
                marker.width = 8;
                marker.loc = phi->loc;
                make_dbg_value(&marker, var, phi->dst);
                inst_array_push(&out, marker);
            }
        }

        /* Identify the terminator (last instruction if BR/CBR/RETURN). */
        size_t term_idx = blk->end;
        if (blk->end > blk->start) {
            IROpcode last_op = fn->insts.data[blk->end - 1].op;
            if (last_op == IR_BR || last_op == IR_CBR || last_op == IR_RETURN)
                term_idx = blk->end - 1;
        }

        /* Emit non-dead body instructions (everything before the terminator). */
        for (size_t i = blk->start; i < term_idx; i++) {
            if (!dead[i])
                inst_array_push(&out, fn->insts.data[i]);
        }

        /* Emit φ-resolution COPYs for each successor.
         * φ(B3: v0 = merge(v1 from B1, v2 from B2)) becomes:
         *   in B1: v0 = COPY v1
         *   in B2: v0 = COPY v2
         * COPYs are placed before the terminator. */
        for (size_t si = 0; si < blk->num_succs; si++) {
            int s = blk->succs[si];
            for (size_t phi_i = 0; phi_i < block_phi_info[s].num_phis; phi_i++) {
                IRPhi *phi = &block_phi_info[s].phis[phi_i];
                /* Find the argument coming from this predecessor block. */
                for (size_t ai = 0; ai < phi->num_args; ai++) {
                    if (phi->args[ai].pred == (int)bi) {
                        IRInst copy;
                        copy.op = IR_COPY;
                        copy.dst = phi->dst;
                        copy.a = phi->args[ai].val;
                        copy.b = -1;
                        copy.imm = 0;
                        copy.loc = phi->loc;
                        copy.call_name = NULL;
                        copy.call_nargs = 0;
                        copy.width = (phi->dst < fn->value_meta_cap && fn->value_width) ? fn->value_width[phi->dst] : 8;
                        copy.is_unsigned = (phi->dst < fn->value_meta_cap && fn->value_is_unsigned) ? fn->value_is_unsigned[phi->dst] : 0;
                        copy.is_float = (phi->dst < fn->value_meta_cap && fn->value_is_float) ? fn->value_is_float[phi->dst] : 0;
                        inst_array_push(&out, copy);
                        break;
                    }
                }
            }
        }

        /* Emit the terminator (if it exists and isn't dead). */
        if (term_idx < blk->end && !dead[term_idx])
            inst_array_push(&out, fn->insts.data[term_idx]);
    }

    /* Replace the function's instruction array. */
    inst_array_free_contents(&fn->insts);
    fn->insts = out;
}

/* ================================================================== */
/* Rename — dominator-tree DFS (Cytron et al., 1991, Section 5)        */
/* ================================================================== */

/* φ helper — add an argument (value from predecessor `pred`) */
static void phi_add_arg(IRPhi *phi, IRValue val, int pred) {
    if (phi->num_args >= phi->cap_args) {
        phi->cap_args = phi->cap_args ? phi->cap_args * 2 : 4;
        phi->args = xrealloc(phi->args, phi->cap_args * sizeof(PhiArg));
    }
    phi->args[phi->num_args].val = val;
    phi->args[phi->num_args].pred = pred;
    phi->num_args++;
}

/* Per-alloca renaming stack */
typedef struct {
    IRValue *vals;  /* stack of reaching values */
    size_t len, cap;
} RenameStack;

static void rstack_init(RenameStack *s) { s->vals = NULL; s->len = 0; s->cap = 0; }

static void rstack_push(RenameStack *s, IRValue v) {
    if (s->len >= s->cap) {
        s->cap = s->cap ? s->cap * 2 : 4;
        s->vals = xrealloc(s->vals, s->cap * sizeof(IRValue));
    }
    s->vals[s->len++] = v;
}

static IRValue rstack_top(const RenameStack *s) {
    return s->len > 0 ? s->vals[s->len - 1] : -1;
}

static void rstack_pop(RenameStack *s) {
    if (s->len > 0) s->len--;
}

static void rstack_free(RenameStack *s) {
    free(s->vals);
    s->vals = NULL;
    s->len = 0;
    s->cap = 0;
}

/* Find alloca index for a given slot value, or (size_t)-1 if not found. */
static size_t find_alloca_slot(const int *alloca_slots, size_t num_alloca,
                                int slot) {
    for (size_t i = 0; i < num_alloca; i++)
        if (alloca_slots[i] == slot) return i;
    return (size_t)-1;
}

/* Recursive dominator-tree DFS for SSA renaming.  On entry to a block we
 * push its φ-results and rewrite LOADs/STOREs; we fill φ-args for each
 * successor (the stack top is the reaching value at this block); then we
 * recurse into dominator-tree children; on exit we pop exactly this block's
 * contributions.  Because children run with the parent's pushes still on
 * the stack and siblings are processed only after the parent fully pops,
 * each block sees precisely the reaching definitions on the dominator path
 * from the entry — never stale pushes from a previously-visited sibling. */
static void mem2reg_rename_dfs(
    int b,
    const IRFunction *fn,
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    BlockPhiInfo *block_phi_info,
    char *d,
    RenameStack *stacks,
    int want_debug)
{
    const CFGBlock *blk = &cfg->blocks[b];

    /* Record stack sizes so we can pop exactly this block's contributions. */
    size_t *entry_sizes = xmalloc(num_alloca * sizeof(size_t));
    for (size_t ai = 0; ai < num_alloca; ai++)
        entry_sizes[ai] = stacks[ai].len;

    /* Push φ results. */
    for (size_t phi_i = 0; phi_i < block_phi_info[b].num_phis; phi_i++) {
        IRPhi *phi = &block_phi_info[b].phis[phi_i];
        size_t ai = find_alloca_slot(alloca_slots, num_alloca, phi->alloca_slot);
        if (ai != (size_t)-1)
            rstack_push(&stacks[ai], phi->dst);
    }

    /* Process instructions. */
    for (size_t i = blk->start; i < blk->end; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->op == IR_LOAD) {
            size_t ai = find_alloca_slot(alloca_slots, num_alloca, inst->a);
            if (ai != (size_t)-1) {
                IRValue reaching = rstack_top(&stacks[ai]);
                if (reaching >= 0) {
                    inst->op = IR_COPY;
                    inst->a = reaching;
                } else {
                    inst->op = IR_CONST;
                    inst->a = -1; inst->b = -1; inst->imm = 0;
                }
            }
        } else if (inst->op == IR_STORE) {
            size_t ai = find_alloca_slot(alloca_slots, num_alloca, inst->a);
            if (ai != (size_t)-1) {
                IRValue stored = inst->b;
                rstack_push(&stacks[ai], stored);
                /* The store itself is gone, but under -g we keep a marker in
                 * its place so the variable's new home is known from here on. */
                int var = want_debug ? dbg_var_for_slot(fn, inst->a) : -1;
                if (var >= 0)
                    make_dbg_value(inst, var, stored);
                else
                    d[i] = 1;
            }
        }
    }

    /* Fill φ-args for successors (stack top = reaching value at this block). */
    for (size_t si = 0; si < blk->num_succs; si++) {
        int s = blk->succs[si];
        for (size_t phi_i = 0; phi_i < block_phi_info[s].num_phis; phi_i++) {
            IRPhi *phi = &block_phi_info[s].phis[phi_i];
            size_t ai = find_alloca_slot(alloca_slots, num_alloca, phi->alloca_slot);
            if (ai != (size_t)-1) {
                IRValue val = rstack_top(&stacks[ai]);
                if (val < 0) val = 0;
                phi_add_arg(phi, val, b);
            }
        }
    }

    /* Recurse into dominator-tree children. */
    for (size_t i = 0; i < dt->n; i++) {
        if (dt->idom[i] == b)
            mem2reg_rename_dfs((int)i, fn, cfg, dt, alloca_slots, num_alloca,
                               block_phi_info, d, stacks, want_debug);
    }

    /* Pop this block's contributions. */
    for (size_t ai = 0; ai < num_alloca; ai++)
        while (stacks[ai].len > entry_sizes[ai])
            rstack_pop(&stacks[ai]);
    free(entry_sizes);
}

void mem2reg_rename(
    IRFunction *fn,
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    BlockPhiInfo *block_phi_info,
    char **dead,
    int want_debug)
{
    size_t ninst = fn->insts.len;

    /* Allocate dead bitmap (zero = keep, 1 = remove). */
    *dead = calloc(ninst, 1);
    if (!*dead) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    char *d = *dead;

    /* Mark ALLOCA instructions dead — but only ones actually being promoted
     * (pinned allocas must stay for codegen to see their alloca_bytes). */
    for (size_t i = 0; i < ninst; i++) {
        if (fn->insts.data[i].op != IR_ALLOCA) continue;
        int slot = fn->insts.data[i].dst;
        int is_promotable = 0;
        for (size_t ai = 0; ai < num_alloca; ai++)
            if (alloca_slots[ai] == slot) { is_promotable = 1; break; }
        if (is_promotable) d[i] = 1;
    }

    if (num_alloca == 0) return;

    /* ---- 1. Initialize rename stacks ---- */
    RenameStack *stacks = xmalloc(num_alloca * sizeof(RenameStack));
    for (size_t ai = 0; ai < num_alloca; ai++) rstack_init(&stacks[ai]);

    /* ---- 2. Rename via dominator-tree DFS (recursive, backtrack popping) ---- */
    mem2reg_rename_dfs(cfg->entry, fn, cfg, dt, alloca_slots, num_alloca,
                       block_phi_info, d, stacks, want_debug);

    /* ---- 3. Cleanup ---- */
    for (size_t ai = 0; ai < num_alloca; ai++) rstack_free(&stacks[ai]);
    free(stacks);
}

/* ================================================================== */
/* opt_mem2reg — full mem2reg pass (φ placement → rename → writeback)  */
/*                                                                      */
/* Classic algorithm:                                                    */
/*   1. Build CFG + dominator tree + dominance frontiers                */
/*   2. For each promotable alloca:                                     */
/*        a. Insert φ at DF of blocks containing stores                  */
/*        b. Rename: dominator-tree DFS, replace LOAD with COPY,        */
/*           mark STORE/ALLOCA dead, push/pop reaching values            */
/*   3. Resolve φ → COPY in predecessors (mem2reg_writeback)            */
/*   4. Rebuild instruction array without dead instructions             */
/*                                                                      */
/* Returns: number of alloca variables promoted.                        */
/* ================================================================== */

static void mem2reg_ensure_value_meta(IRFunction *fn, int v) {
    if (!fn || v < fn->value_meta_cap) return;
    int old_cap = fn->value_meta_cap;
    int new_cap = old_cap ? old_cap * 2 : 64;
    while (new_cap <= v) new_cap *= 2;
    fn->value_width = xrealloc(fn->value_width, new_cap * sizeof(int));
    fn->value_is_unsigned = xrealloc(fn->value_is_unsigned, new_cap * sizeof(int));
    if (fn->value_is_float) {
        fn->value_is_float = xrealloc(fn->value_is_float, new_cap * sizeof(int));
        for (int i = old_cap; i < new_cap; i++) fn->value_is_float[i] = 0;
    }
    for (int i = old_cap; i < new_cap; i++) {
        fn->value_width[i] = 4;
        fn->value_is_unsigned[i] = 0;
    }
    fn->value_meta_cap = new_cap;
}

int opt_mem2reg(IRFunction *fn, int want_debug)
{
    CFG cfg;
    cfg_build(&cfg, &fn->insts);

    DomTree dt;
    domtree_build(&dt, &cfg);

    const IRInstArray *insts = &fn->insts;

    /* ---- 1. Identify promotable alloca slots ----
     * Pin (skip) any alloca whose id appears as an operand of IR_ADDR,
     * IR_LOAD_PTR, or IR_STORE_PTR, OR whose declared size is > 0 (arrays). */
    char *pinned = xmalloc(fn->next_value_id * sizeof(char));
    memset(pinned, 0, fn->next_value_id * sizeof(char));
    for (size_t i = 0; i < insts->len; i++) {
        const IRInst *ii = &insts->data[i];
        if (ii->op == IR_ALLOCA && ii->alloca_bytes > 0 && ii->dst >= 0)
            pinned[ii->dst] = 1;
        if (ii->op == IR_ADDR && ii->a >= 0 && ii->a < fn->next_value_id)
            pinned[ii->a] = 1;
    }
    int *alloca_slots = NULL;
    size_t num_alloca = 0, cap_alloca = 0;
    for (size_t i = 0; i < insts->len; i++) {
        if (insts->data[i].op == IR_ALLOCA) {
            int slot = insts->data[i].dst;
            if (slot >= 0 && pinned[slot]) continue;
            if (num_alloca >= cap_alloca) {
                cap_alloca = cap_alloca ? cap_alloca * 2 : 8;
                alloca_slots = xrealloc(alloca_slots, cap_alloca * sizeof(int));
            }
            alloca_slots[num_alloca++] = slot;
        }
    }
    free(pinned);

    if (num_alloca == 0) {
        free(alloca_slots);
        domtree_free(&dt);
        cfg_free(&cfg);
        return 0;
    }

    /* ---- 2. Build block_stores bitmap ---- */
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

    /* ---- 3. φ placement ---- */
    BlockPhiInfo *bp = mem2reg_place_phis(
        &cfg, &dt, alloca_slots, num_alloca,
        (const char **)block_stores, &fn->next_value_id);

    /* Free block_stores (no longer needed). */
    for (size_t bi = 0; bi < cfg.num; bi++) free(block_stores[bi]);
    free(block_stores);

    /* Propagate value metadata for all φ destination SSA values */
    for (size_t bi = 0; bi < cfg.num; bi++) {
        for (size_t phi_i = 0; phi_i < bp[bi].num_phis; phi_i++) {
            IRPhi *phi = &bp[bi].phis[phi_i];
            int slot = phi->alloca_slot;
            IRValue phi_dst = phi->dst;
            mem2reg_ensure_value_meta(fn, phi_dst);
            if (slot >= 0 && slot < fn->value_meta_cap) {
                if (fn->value_width) fn->value_width[phi_dst] = fn->value_width[slot];
                if (fn->value_is_unsigned) fn->value_is_unsigned[phi_dst] = fn->value_is_unsigned[slot];
                if (fn->value_is_float) fn->value_is_float[phi_dst] = fn->value_is_float[slot];
            }
        }
    }

    /* ---- 4. Rename ---- */
    char *dead = NULL;
    mem2reg_rename(fn, &cfg, &dt, alloca_slots, num_alloca, bp, &dead,
                   want_debug);

    /* ---- 5. Writeback (rebuild flat IR, resolve φ → COPY) ---- */
    mem2reg_writeback(fn, &cfg, bp, dead, want_debug);

    /* ---- 6. Cleanup ---- */
    block_phi_info_free(bp, cfg.num);
    free(alloca_slots);
    free(dead);
    domtree_free(&dt);
    cfg_free(&cfg);

    return (int)num_alloca;
}
