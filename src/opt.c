#include "fakecc/opt.h"
#include "fakecc/cfg.h"
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
/* SSA φ nodes — exist only during mem2reg, never written to flat IR   */
/* ================================================================== */

/* φ node — exists only during mem2reg, never written to the flat IR. */
typedef struct { IRValue val; int pred; } PhiArg;
typedef struct {
    IRValue dst;          /* φ result value */
    int alloca_slot;      /* which alloca this φ merges */
    PhiArg *args; size_t num, cap;
    SourceLoc loc;
} IRPhi;

/* Per-block φ storage, indexed by CFG block id.  The CFG module
 * (cfg.h) does not know about φ nodes, so we keep them here. */
typedef struct {
    IRPhi *phis;
    size_t num_phis, cap_phis;
} BlockPhi;

/* Helper: add a φ to a block's phi list. */
static IRPhi *block_add_phi(BlockPhi *bp, IRValue dst, int alloca_slot, SourceLoc loc) {
    if (bp->num_phis >= bp->cap_phis) {
        bp->cap_phis = bp->cap_phis ? bp->cap_phis * 2 : 4;
        bp->phis = xrealloc(bp->phis, bp->cap_phis * sizeof(IRPhi));
    }
    IRPhi *phi = &bp->phis[bp->num_phis++];
    phi->dst = dst;
    phi->alloca_slot = alloca_slot;
    phi->args = NULL;
    phi->num = 0;
    phi->cap = 0;
    phi->loc = loc;
    return phi;
}

static void phi_add_arg(IRPhi *phi, IRValue val, int pred) {
    if (phi->num >= phi->cap) {
        phi->cap = phi->cap ? phi->cap * 2 : 4;
        phi->args = xrealloc(phi->args, phi->cap * sizeof(PhiArg));
    }
    phi->args[phi->num].val = val;
    phi->args[phi->num].pred = pred;
    phi->num++;
}

/* Does this block already have a φ for the given alloca slot? */
static int block_has_phi(BlockPhi *bp, int alloca_slot) {
    for (size_t i = 0; i < bp->num_phis; i++)
        if (bp->phis[i].alloca_slot == alloca_slot) return 1;
    return 0;
}

/* ================================================================== */
/* Dominator tree + dominance frontiers                                */
/* ================================================================== */

/* ================================================================== */
/* Dominator tree + dominance frontiers                                */
/* ================================================================== */

typedef struct {
    int *idom;        /* idom[b] = immediate dominator of b; entry = -1 */
    int **df;         /* df[b] = dominance-frontier set (array of block ids) */
    size_t *df_len;
    size_t n;
} DomTree;

/* Does block a dominate block b? (via idom chain) */
static int dominates(const DomTree *dt, int a, int b) {
    int runner = b;
    while (runner != -1) {
        if (runner == a) return 1;
        if (runner == dt->idom[runner]) break; /* safety */
        runner = dt->idom[runner];
    }
    return 0;
}

/* Compute immediate dominators via iterative dataflow.
 * idom[entry] = -1; for others, idom is the intersection of dominators
 * of all predecessors, walking up the (reverse-postorder-ish) order. */
static void domtree_build(DomTree *dt, const CFG *g) {
    size_t n = g->num;
    dt->n = n;
    dt->idom = xmalloc(n * sizeof(int));
    for (size_t i = 0; i < n; i++) dt->idom[i] = -1;
    /* entry dominates itself; mark with a sentinel distinct from -1 */
    dt->idom[g->entry] = (int)g->entry;

    /* intersect(a, b): walk up idom chains in lockstep until they meet */
    /* (Cooper-Harvey-Kennedy intersect, with RPO order approximated by id) */

    int changed = 1;
    while (changed) {
        changed = 0;
        for (size_t bi = 0; bi < n; bi++) {
            int b = (int)bi;
            if (b == g->entry) continue;
            CFGBlock *blk = &g->blocks[bi];
            /* pick first processed predecessor */
            int new_idom = -1;
            for (size_t i = 0; i < blk->num_preds; i++) {
                int p = blk->preds[i];
                if (dt->idom[p] != -1) {
                    new_idom = p;
                    break;
                }
            }
            if (new_idom == -1) continue; /* no processed predecessor yet */
            for (size_t i = 0; i < blk->num_preds; i++) {
                int p = blk->preds[i];
                if (p == new_idom) continue;
                if (dt->idom[p] != -1) {
                    /* intersect new_idom and p */
                    int f1 = new_idom, f2 = p;
                    while (f1 != f2) {
                        while (f1 > f2) f1 = dt->idom[f1];
                        while (f2 > f1) f2 = dt->idom[f2];
                    }
                    new_idom = f1;
                }
            }
            if (dt->idom[b] != new_idom) {
                dt->idom[b] = new_idom;
                changed = 1;
            }
        }
    }
    /* entry's idom is itself conventionally; set to -1 for "no parent" */
    dt->idom[g->entry] = -1;

    /* dominance frontiers */
    dt->df = xmalloc(n * sizeof(int *));
    dt->df_len = xmalloc(n * sizeof(size_t));
    for (size_t i = 0; i < n; i++) {
        dt->df[i] = NULL;
        dt->df_len[i] = 0;
    }
    for (size_t bi = 0; bi < n; bi++) {
        CFGBlock *b = &g->blocks[bi];
        if (b->num_preds < 2) continue;
        for (size_t i = 0; i < b->num_preds; i++) {
            int runner = b->preds[i];
            while (runner != -1 && runner != dt->idom[bi] && runner != (int)bi) {
                /* add bi to DF[runner] if not present */
                int found = 0;
                for (size_t k = 0; k < dt->df_len[runner]; k++)
                    if (dt->df[runner][k] == (int)bi) { found = 1; break; }
                if (!found) {
                    size_t cap = (dt->df_len[runner] + 4) & ~(size_t)3;
                    if (cap <= dt->df_len[runner]) cap = dt->df_len[runner] + 4;
                    dt->df[runner] = xrealloc(dt->df[runner], cap * sizeof(int));
                    dt->df[runner][dt->df_len[runner]++] = (int)bi;
                }
                runner = dt->idom[runner];
            }
        }
    }
}

static void domtree_free(DomTree *dt) {
    for (size_t i = 0; i < dt->n; i++) free(dt->df[i]);
    free(dt->df);
    free(dt->df_len);
    free(dt->idom);
    dt->idom = NULL;
    dt->df = NULL;
    dt->df_len = NULL;
    dt->n = 0;
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

/* Per-alloca renaming stack */
typedef struct {
    IRValue *vals;  /* stack of reaching values */
    size_t len, cap;
} RenameStack;

static void rstack_init(RenameStack *s) { s->vals = NULL; s->len = 0; s->cap = 0; }
static void rstack_push(RenameStack *s, IRValue v) {
    if (s->len >= s->cap) { s->cap = s->cap ? s->cap * 2 : 4; s->vals = xrealloc(s->vals, s->cap * sizeof(IRValue)); }
    s->vals[s->len++] = v;
}
static IRValue rstack_top(RenameStack *s) { return s->len > 0 ? s->vals[s->len - 1] : -1; }
static void rstack_pop(RenameStack *s) { if (s->len > 0) s->len--; }
static void rstack_free(RenameStack *s) { free(s->vals); s->vals = NULL; s->len = 0; s->cap = 0; }

/* Dominator-tree DFS order (children = blocks whose idom == parent) */
static void domtree_children(const DomTree *dt, int parent, int **children,
                              size_t *nchildren, size_t *ccap) {
    *nchildren = 0;
    for (size_t i = 0; i < dt->n; i++) {
        if (dt->idom[i] == parent) {
            if (*nchildren >= *ccap) {
                *ccap = *ccap ? *ccap * 2 : 8;
                *children = xrealloc(*children, *ccap * sizeof(int));
            }
            (*children)[(*nchildren)++] = (int)i;
        }
    }
}

static void opt_mem2reg(IRFunction *fn) {
    (void)dominates; /* used in future dominator queries */
    CFG cfg;
    cfg_build(&cfg, &fn->insts);

    /* Per-block φ storage — the CFG module does not carry φ nodes. */
    BlockPhi *block_phis = xmalloc(cfg.num * sizeof(BlockPhi));
    memset(block_phis, 0, cfg.num * sizeof(BlockPhi));

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

    /* dead[] marks instructions to skip during write-back */
    char *dead = xmalloc(insts->len * sizeof(char));
    memset(dead, 0, insts->len * sizeof(char));

    /* Mark each ALLOCA dead initially; it survives only if not promotable */
    for (size_t i = 0; i < insts->len; i++)
        if (insts->data[i].op == IR_ALLOCA) dead[i] = 1;

    /* ---- 2. φ placement ---- */
    /* For each alloca, find blocks with stores, then place φ at DF. */
    for (size_t ai = 0; ai < num_alloca; ai++) {
        int slot = alloca_slots[ai];
        /* blocks containing a store to this alloca */
        int *def_blocks = NULL; size_t num_def = 0, cap_def = 0;
        for (size_t i = 0; i < insts->len; i++) {
            if (insts->data[i].op == IR_STORE && insts->data[i].a == slot) {
                /* find which block this instruction is in */
                for (size_t bi = 0; bi < cfg.num; bi++) {
                    if (i >= cfg.blocks[bi].start && i < cfg.blocks[bi].end) {
                        int found = 0;
                        for (size_t k = 0; k < num_def; k++) if (def_blocks[k] == (int)bi) { found = 1; break; }
                        if (!found) {
                            if (num_def >= cap_def) { cap_def = cap_def ? cap_def * 2 : 4; def_blocks = xrealloc(def_blocks, cap_def * sizeof(int)); }
                            def_blocks[num_def++] = (int)bi;
                        }
                        break;
                    }
                }
            }
        }
        /* worklist: for each def block, add its DF members that don't have a φ yet */
        int *wl = NULL; size_t wl_len = 0, wl_cap = 0;
        for (size_t d = 0; d < num_def; d++) {
            if (wl_len >= wl_cap) { wl_cap = wl_cap ? wl_cap * 2 : 4; wl = xrealloc(wl, wl_cap * sizeof(int)); }
            wl[wl_len++] = def_blocks[d];
        }
        while (wl_len > 0) {
            int X = wl[--wl_len];
            for (size_t di = 0; di < dt.df_len[X]; di++) {
                int Y = dt.df[X][di];
                if (!block_has_phi(&block_phis[Y], slot)) {
                    /* create a new SSA value for the φ result */
                    IRValue phi_dst = fn->next_value_id++;
                    /* find a source loc from the alloca */
                    SourceLoc phi_loc = {NULL, 0, 0};
                    for (size_t i = 0; i < insts->len; i++)
                        if (insts->data[i].op == IR_ALLOCA && insts->data[i].dst == slot)
                            { phi_loc = insts->data[i].loc; break; }
                    block_add_phi(&block_phis[Y], phi_dst, slot, phi_loc);
                    /* If Y has predecessors that aren't in def_blocks (i.e. Y
                     * is reachable from non-def blocks), add Y to worklist
                     * so φ propagates further. Simplified: always add. */
                    int already = 0;
                    for (size_t k = 0; k < num_def; k++) if (def_blocks[k] == Y) { already = 1; break; }
                    if (!already) {
                        if (num_def >= cap_def) { cap_def = cap_def ? cap_def * 2 : 4; def_blocks = xrealloc(def_blocks, cap_def * sizeof(int)); }
                        def_blocks[num_def++] = Y;
                        if (wl_len >= wl_cap) { wl_cap = wl_cap ? wl_cap * 2 : 4; wl = xrealloc(wl, wl_cap * sizeof(int)); }
                        wl[wl_len++] = Y;
                    }
                }
            }
        }
        free(wl);
        free(def_blocks);
    }

    /* ---- 3. Rename (dominator-tree DFS) ---- */
    /* stacks[ai] = rename stack for alloca_slots[ai] */
    RenameStack *stacks = xmalloc(num_alloca * sizeof(RenameStack));
    for (size_t ai = 0; ai < num_alloca; ai++) rstack_init(&stacks[ai]);

    /* We need a writable copy of instructions (for in-place LOAD→COPY/CONST) */
    /* Since we already have fn->insts which is writable, we modify in-place. */
    /* But we access via `insts` pointer which was set before. After in-place
     * modifications, the data is still valid. */
    /* Let's just work directly with fn->insts.data from here. */

    /* Recursive DFS over dominator tree */
    int *dt_children = NULL; size_t dt_nch = 0, dt_cch = 0;

    /* We implement DFS iteratively to avoid stack overflow */
    /* Stack of (block_id, child_index) pairs */
    typedef struct { int block; size_t child_idx; } DFSFrame;
    DFSFrame *dfs = NULL; size_t dfs_len = 0, dfs_cap = 0;

    /* Push entry block */
    #define DFS_PUSH(b, ci) do { \
        if (dfs_len >= dfs_cap) { dfs_cap = dfs_cap ? dfs_cap * 2 : 32; \
            dfs = xrealloc(dfs, dfs_cap * sizeof(DFSFrame)); } \
        dfs[dfs_len].block = (b); dfs[dfs_len].child_idx = (ci); dfs_len++; \
    } while(0)

    /* We need to track pops per block to undo stack pushes. Use a simple
     * approach: for each block, record how many values were pushed onto each
     * alloca's rename stack, then pop that many on exit. */
    /* push_count[bi][ai] = number of pushes for alloca ai in block bi */
    size_t **push_count = xmalloc(cfg.num * sizeof(size_t *));
    for (size_t bi = 0; bi < cfg.num; bi++) {
        push_count[bi] = xmalloc(num_alloca * sizeof(size_t));
        memset(push_count[bi], 0, num_alloca * sizeof(size_t));
    }

    /* Iterative DFS: we process each block, then push its children.
     * On return from a block (when all children done), we pop its pushes.
     * Use a two-phase approach: first visit all blocks in dom-tree preorder
     * (collecting the order), then process each block in that order.
     * Simpler: recursive emulation with explicit stack. */

    /* Phase A: compute dom-tree preorder */
    int *preorder = NULL; size_t n_preorder = 0, cap_preorder = 0;
    DFS_PUSH(cfg.entry, 0);
    while (dfs_len > 0) {
        DFSFrame top = dfs[--dfs_len];
        int b = top.block;
        if (top.child_idx == 0) {
            /* first visit: record in preorder */
            if (n_preorder >= cap_preorder) { cap_preorder = cap_preorder ? cap_preorder * 2 : 16; preorder = xrealloc(preorder, cap_preorder * sizeof(int)); }
            preorder[n_preorder++] = b;
        }
        /* get children */
        domtree_children(&dt, b, &dt_children, &dt_nch, &dt_cch);
        if (top.child_idx < dt_nch) {
            /* push self with next child index, then push the child */
            DFS_PUSH(b, top.child_idx + 1);
            DFS_PUSH(dt_children[top.child_idx], 0);
        }
    }
    free(dfs); dfs = NULL; dfs_len = 0; dfs_cap = 0;

    /* Phase B: process blocks in preorder — rename variables */
    for (size_t pi = 0; pi < n_preorder; pi++) {
        int b = preorder[pi];
        CFGBlock *blk = &cfg.blocks[b];

        /* Process φ nodes at block start: push φ results onto stacks */
        for (size_t phi_i = 0; phi_i < block_phis[b].num_phis; phi_i++) {
            IRPhi *phi = &block_phis[b].phis[phi_i];
            /* find which alloca this φ is for */
            size_t ai;
            for (ai = 0; ai < num_alloca; ai++)
                if (alloca_slots[ai] == phi->alloca_slot) break;
            if (ai < num_alloca) {
                rstack_push(&stacks[ai], phi->dst);
                push_count[b][ai]++;
            }
        }

        /* Process instructions in this block */
        for (size_t i = blk->start; i < blk->end; i++) {
            IRInst *inst = &fn->insts.data[i];
            if (inst->op == IR_LOAD) {
                /* Is this a load from a promotable alloca? */
                size_t ai;
                for (ai = 0; ai < num_alloca; ai++)
                    if (alloca_slots[ai] == inst->a) break;
                if (ai < num_alloca) {
                    IRValue reaching = rstack_top(&stacks[ai]);
                    if (reaching >= 0) {
                        /* Replace LOAD with COPY dst=reaching */
                        inst->op = IR_COPY;
                        inst->a = reaching;
                    } else {
                        /* Uninitialized read (UB in C): replace with CONST 0 */
                        inst->op = IR_CONST;
                        inst->a = -1;
                        inst->b = -1;
                        inst->imm = 0;
                    }
                }
            } else if (inst->op == IR_STORE) {
                size_t ai;
                for (ai = 0; ai < num_alloca; ai++)
                    if (alloca_slots[ai] == inst->a) break;
                if (ai < num_alloca) {
                    rstack_push(&stacks[ai], inst->b);
                    push_count[b][ai]++;
                    dead[i] = 1;  /* store is dead (value now in SSA) */
                }
            }
        }

        /* Fill φ args for each successor: for each successor S, for each φ
         * in S, set the arg from this predecessor to the current stack top. */
        for (size_t si = 0; si < blk->num_succs; si++) {
            int s = blk->succs[si];
            for (size_t phi_i = 0; phi_i < block_phis[s].num_phis; phi_i++) {
                IRPhi *phi = &block_phis[s].phis[phi_i];
                size_t ai;
                for (ai = 0; ai < num_alloca; ai++)
                    if (alloca_slots[ai] == phi->alloca_slot) break;
                if (ai < num_alloca) {
                    IRValue val = rstack_top(&stacks[ai]);
                    if (val < 0) val = 0; /* undef → 0 */
                    phi_add_arg(phi, val, b);
                }
            }
        }
    }

    /* Pop stacks in reverse preorder (LIFO) to restore state */
    for (int pi = (int)n_preorder - 1; pi >= 0; pi--) {
        int b = preorder[pi];
        /* Pop φ pushes */
        for (size_t phi_i = 0; phi_i < block_phis[b].num_phis; phi_i++) {
            IRPhi *phi = &block_phis[b].phis[phi_i];
            size_t ai;
            for (ai = 0; ai < num_alloca; ai++)
                if (alloca_slots[ai] == phi->alloca_slot) break;
            if (ai < num_alloca) rstack_pop(&stacks[ai]);
        }
        /* Pop store pushes: we need to know how many stores were in this block.
         * Use push_count which tracked total pushes (φ + store). We already
         * popped φ. Pop the remaining = push_count - num_phis_for_this_alloca. */
        /* Actually, push_count[b][ai] counts all pushes (φ + store).
         * We already popped the φ pushes (one per φ). Pop the rest. */
        for (size_t ai = 0; ai < num_alloca; ai++) {
            /* Count φ pushes for this alloca in this block */
            size_t phi_pushes = 0;
            for (size_t phi_i = 0; phi_i < block_phis[b].num_phis; phi_i++)
                if (block_phis[b].phis[phi_i].alloca_slot == alloca_slots[ai]) phi_pushes++;
            size_t store_pushes = push_count[b][ai] - phi_pushes;
            for (size_t s = 0; s < store_pushes; s++) rstack_pop(&stacks[ai]);
        }
    }
    free(preorder);
    for (size_t bi = 0; bi < cfg.num; bi++) free(push_count[bi]);
    free(push_count);

    /* ---- 4. Write-back: rebuild instruction array ---- */
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
                    for (size_t ai = 0; ai < phi->num; ai++) {
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
    for (size_t ai = 0; ai < num_alloca; ai++) rstack_free(&stacks[ai]);
    free(stacks);
    for (size_t bi = 0; bi < cfg.num; bi++) {
        for (size_t j = 0; j < block_phis[bi].num_phis; j++)
            free(block_phis[bi].phis[j].args);
        free(block_phis[bi].phis);
    }
    free(block_phis);
    free(alloca_slots);
    free(dead);
    free(dt_children);
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
