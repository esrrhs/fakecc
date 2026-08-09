#include "fakecc/regalloc.h"
#include "fakecc/cfg.h"
#include "fakecc/common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/* Register-class abstraction                                           */
/*                                                                      */
/* The allocator is run once per register class.  Each class owns a    */
/* set of allocatable registers and a bitmask describing which of those */
/* are caller-saved (and so forbidden for values live across a call).   */
/* ================================================================== */

typedef struct {
    const int *regs;           /* native register codes, indexed 0..nregs-1 */
    int        nregs;          /* number of allocatable registers */
    unsigned   caller_saved;   /* bitmask over regs[] indices */
} RegClass;

static const RegClass GP_CLASS = {
    .regs = ALLOCATABLE_REGS,
    .nregs = REG_ALLOCATABLE,
    .caller_saved = GP_CALLER_SAVED_MASK,
};

static const RegClass XMM_CLASS = {
    .regs = XMM_ALLOCATABLE_REGS,
    .nregs = REG_XMM_ALLOCATABLE,
    .caller_saved = XMM_CALLER_SAVED_MASK,
};

/* True if SSA value `v` belongs to the float class in this function.
 * When the function has no per-value float metadata (value_is_float is
 * NULL or value_meta_cap is 0 — e.g. IR hand-built by tests) every value
 * is treated as GP-class. */
static int value_is_float_class(const IRFunction *fn, int v) {
    if (v < 0) return 0;
    if (!fn->value_is_float || fn->value_meta_cap <= 0) return 0;
    if (v >= fn->value_meta_cap) return 0;
    return fn->value_is_float[v];
}

/* True if SSA value `v` is a long double (TY_FLOAT width 16).  Long doubles
 * live in x87 st0 / 16-byte stack slots, NOT in the XMM register file, so
 * they must be excluded from the XMM regalloc class (they are also excluded
 * from GP, since they are float-class). */
static int value_is_ld(const IRFunction *fn, int v) {
    if (!value_is_float_class(fn, v)) return 0;
    if (!fn->value_width || fn->value_meta_cap <= 0) return 0;
    if (v >= fn->value_meta_cap) return 0;
    return fn->value_width[v] == 16;
}

/* True if SSA value `v` belongs to the XMM-allocatable float class: float
 * or double, but NOT long double (which uses x87). */
static int value_is_xmm_float(const IRFunction *fn, int v) {
    return value_is_float_class(fn, v) && !value_is_ld(fn, v);
}

/* True if SSA value `v` belongs to the register class being allocated in
 * this run: GP (float_class 0) = non-float; XMM (float_class 1) = float but
 * not long double.  Encapsulates the long-double exclusion so the XMM run
 * leaves ld values uncolored (REG_NONE) — their home is an x87 stack slot. */
static int value_in_class(const IRFunction *fn, int v, int float_class) {
    if (float_class) return value_is_xmm_float(fn, v);
    return !value_is_float_class(fn, v);
}

/* ================================================================== */
/* Liveness analysis                                                   */
/* ================================================================== */

typedef struct {
    int def_point;         /* instruction index where defined, -1 = never */
    int *use_points;       /* sorted array of use-site instruction indices */
    size_t num_uses;
    size_t cap_uses;
    int live_start;        /* first prog point where value is live */
    int live_end;          /* last prog point where value is live */
} LiveInfo;

static void liv_init(LiveInfo *liv, int n) {
    for (int i = 0; i < n; i++) {
        liv[i].def_point = -1;
        liv[i].use_points = NULL;
        liv[i].num_uses = 0;
        liv[i].cap_uses = 0;
        liv[i].live_start = -1;
        liv[i].live_end = -1;
    }
}

static void liv_free(LiveInfo *liv, int n) {
    for (int i = 0; i < n; i++) free(liv[i].use_points);
    free(liv);
}

static void liv_add_use(LiveInfo *l, int pt) {
    if (l->num_uses >= l->cap_uses) {
        l->cap_uses = l->cap_uses ? l->cap_uses * 2 : 4;
        l->use_points = xrealloc(l->use_points, l->cap_uses * sizeof(int));
    }
    l->use_points[l->num_uses++] = pt;
}

/* Compute liveness for all SSA values in fn.
 * Returns a malloc'd LiveInfo array of length fn->next_value_id. */
static LiveInfo *compute_liveness(const IRFunction *fn) {
    int nv = fn->next_value_id;
    if (nv <= 0) return NULL;

    LiveInfo *liv = xmalloc(nv * sizeof(LiveInfo));
    liv_init(liv, nv);

    for (size_t i = 0; i < fn->insts.len; i++) {
        const IRInst *inst = &fn->insts.data[i];

        /* LABEL/BR carry no value operands.  CBR's `b` is a label id. */
        if (inst->op == IR_LABEL || inst->op == IR_BR) continue;

        /* Definition */
        if (inst->dst >= 0 && inst->dst < nv) {
            liv[inst->dst].def_point = (int)i;
        }

        /* Uses */
        if (inst->a >= 0 && inst->a < nv) liv_add_use(&liv[inst->a], (int)i);
        if (inst->op != IR_CBR &&
            inst->b >= 0 && inst->b < nv) liv_add_use(&liv[inst->b], (int)i);

        /* IR_CALL: each call_args[k] is a use. */
        if (inst->op == IR_CALL) {
            for (int k = 0; k < inst->call_nargs; k++) {
                IRValue av = inst->call_args[k];
                if (av >= 0 && av < nv) liv_add_use(&liv[av], (int)i);
            }
        }
    }

    /* Compute live_start / live_end for each value. */
    for (int v = 0; v < nv; v++) {
        if (liv[v].num_uses == 0) {
            /* Dead value — live only at its definition point. */
            liv[v].live_start = liv[v].def_point >= 0 ? liv[v].def_point : 0;
            liv[v].live_end   = liv[v].live_start;
        } else {
            liv[v].live_start = liv[v].def_point >= 0 ? liv[v].def_point
                                : liv[v].use_points[0];
            liv[v].live_end   = liv[v].use_points[liv[v].num_uses - 1];
        }
    }

    return liv;
}

/* ================================================================== */
/* Interference graph (chordal — SSA guarantees this property)         */
/* ================================================================== */

typedef struct {
    int *neighbors;    /* flattened adjacency for vertex v */
    size_t degree;     /* number of neighbors */
    size_t cap;        /* capacity of neighbors array */
} IGANode;

typedef struct {
    IGANode *nodes;    /* nodes[v] for v = 0..n-1 */
    int n;
} InterfGraph;

static void ig_init(InterfGraph *g, int n) {
    g->n = n;
    g->nodes = xmalloc(n * sizeof(IGANode));
    for (int i = 0; i < n; i++) {
        g->nodes[i].neighbors = NULL;
        g->nodes[i].degree = 0;
        g->nodes[i].cap = 0;
    }
}

static void ig_free(InterfGraph *g) {
    for (int i = 0; i < g->n; i++) free(g->nodes[i].neighbors);
    free(g->nodes);
    g->nodes = NULL;
    g->n = 0;
}

static void ig_add_edge(InterfGraph *g, int u, int v) {
    if (u == v) return;

    /* Add v to u's adjacency (avoid duplicates). */
    IGANode *un = &g->nodes[u];
    for (size_t i = 0; i < un->degree; i++)
        if (un->neighbors[i] == v) return;
    if (un->degree >= un->cap) {
        un->cap = un->cap ? un->cap * 2 : 8;
        un->neighbors = xrealloc(un->neighbors, un->cap * sizeof(int));
    }
    un->neighbors[un->degree++] = v;

    /* Add u to v's adjacency. */
    IGANode *vn = &g->nodes[v];
    if (vn->degree >= vn->cap) {
        vn->cap = vn->cap ? vn->cap * 2 : 8;
        vn->neighbors = xrealloc(vn->neighbors, vn->cap * sizeof(int));
    }
    vn->neighbors[vn->degree++] = u;
}

/* ================================================================== */
/* CFG-aware liveness + interference (correct across loop back edges)  */
/* ================================================================== */

/* Bit-set helpers over an nv-bit vector (packed into 64-bit words). */
typedef struct {
    uint64_t *w;         /* words[num_words] */
    int nv;
    int num_words;
} BitSet;

static void bs_init(BitSet *b, int nv) {
    b->nv = nv;
    b->num_words = (nv + 63) / 64;
    b->w = xmalloc((size_t)b->num_words * sizeof(uint64_t));
    memset(b->w, 0, (size_t)b->num_words * sizeof(uint64_t));
}

static void bs_free(BitSet *b) {
    free(b->w);
    b->w = NULL;
}

static void bs_clear(BitSet *b) {
    memset(b->w, 0, (size_t)b->num_words * sizeof(uint64_t));
}

static int bs_test(const BitSet *b, int v) {
    return (int)((b->w[v >> 6] >> (v & 63)) & 1);
}

static void bs_set(BitSet *b, int v) {
    b->w[v >> 6] |= ((uint64_t)1 << (v & 63));
}

static void bs_clr(BitSet *b, int v) {
    b->w[v >> 6] &= ~((uint64_t)1 << (v & 63));
}

/* dst |= src. Returns 1 if dst changed. */
static int bs_or_changed(BitSet *dst, const BitSet *src) {
    int changed = 0;
    for (int i = 0; i < dst->num_words; i++) {
        uint64_t before = dst->w[i];
        uint64_t after = before | src->w[i];
        if (after != before) changed = 1;
        dst->w[i] = after;
    }
    return changed;
}

/* dst = src. */
static void bs_copy(BitSet *dst, const BitSet *src) {
    memcpy(dst->w, src->w, (size_t)dst->num_words * sizeof(uint64_t));
}

/* Iterate set bits: for each set bit, invoke fn(v). */
#define BS_FOREACH(bs, v)  \
    for (int _wi = 0; _wi < (bs)->num_words; _wi++) \
        for (uint64_t _w = (bs)->w[_wi], v; _w && ((v = _wi * 64 + __builtin_ctzll(_w)), 1); _w &= _w - 1)

/* Compute per-block use[b] and def[b] sets.
 *
 * use[b]  = values read in b before being (re)defined in b
 * def[b]  = values written anywhere in b
 *
 * Note: for a value that is both defined and used in b (e.g., x = x + 1),
 * we treat the use as "before def" iff the read appears strictly before
 * the write in the block's instruction order — the standard dataflow
 * definition of upwards-exposed uses. */
static void compute_use_def(const IRFunction *fn, const CFG *cfg,
                            BitSet *use_b, BitSet *def_b) {
    for (size_t bi = 0; bi < cfg->num; bi++) {
        bs_clear(&use_b[bi]);
        bs_clear(&def_b[bi]);
        const CFGBlock *blk = &cfg->blocks[bi];
        for (size_t i = blk->start; i < blk->end; i++) {
            const IRInst *inst = &fn->insts.data[i];
            if (inst->op == IR_LABEL || inst->op == IR_BR) continue;

            /* Uses come before def within a single instruction. */
            if (inst->a >= 0 && inst->a < use_b[bi].nv) {
                if (!bs_test(&def_b[bi], inst->a))
                    bs_set(&use_b[bi], inst->a);
            }
            if (inst->op != IR_CBR &&
                inst->b >= 0 && inst->b < use_b[bi].nv) {
                if (!bs_test(&def_b[bi], inst->b))
                    bs_set(&use_b[bi], inst->b);
            }
            if (inst->op == IR_CALL) {
                for (int k = 0; k < inst->call_nargs; k++) {
                    IRValue av = inst->call_args[k];
                    if (av >= 0 && av < use_b[bi].nv &&
                        !bs_test(&def_b[bi], av))
                        bs_set(&use_b[bi], av);
                }
            }
            if (inst->dst >= 0 && inst->dst < def_b[bi].nv) {
                bs_set(&def_b[bi], inst->dst);
            }
        }
    }
}

/* Fixed-point compute in[b] / out[b]:
 *   in[b]  = use[b] ∪ (out[b] \ def[b])
 *   out[b] = ⋃_{s ∈ succs(b)} in[s]
 *
 * Iterate blocks in reverse order until nothing changes.  This handles
 * loops naturally: the back edge carries values from body's out to head's in. */
static void compute_live_in_out(const CFG *cfg,
                                const BitSet *use_b, const BitSet *def_b,
                                BitSet *in_b, BitSet *out_b) {
    int changed = 1;
    BitSet tmp;
    bs_init(&tmp, use_b[0].nv);

    while (changed) {
        changed = 0;
        /* Process in reverse block order — decent starting heuristic for
         * forward CFGs since terminators come last. */
        for (int bi = (int)cfg->num - 1; bi >= 0; bi--) {
            const CFGBlock *blk = &cfg->blocks[bi];

            /* out[b] = union of in[s] for each succ s */
            bs_clear(&out_b[bi]);
            for (size_t si = 0; si < blk->num_succs; si++) {
                bs_or_changed(&out_b[bi], &in_b[blk->succs[si]]);
            }

            /* in[b] = use[b] ∪ (out[b] \ def[b]) */
            bs_copy(&tmp, &out_b[bi]);
            /* tmp = out \ def */
            for (int wi = 0; wi < tmp.num_words; wi++) {
                tmp.w[wi] &= ~def_b[bi].w[wi];
            }
            /* tmp |= use */
            for (int wi = 0; wi < tmp.num_words; wi++) {
                tmp.w[wi] |= use_b[bi].w[wi];
            }

            /* Did in[bi] change? */
            for (int wi = 0; wi < tmp.num_words; wi++) {
                if (tmp.w[wi] != in_b[bi].w[wi]) {
                    changed = 1;
                    break;
                }
            }
            bs_copy(&in_b[bi], &tmp);
        }
    }
    bs_free(&tmp);
}

/* Build interference graph by walking each block backwards, maintaining
 * the live-set.  At each instruction, dst interferes with every value
 * currently live-out (i.e., in `live` before removing dst).  Then remove
 * dst, add uses.
 *
 * `forbid_mask[v]` (output, k bits wide): OR of color indices this value
 * must NOT be colored to.  Currently set only for values live across an
 * IR_CALL — which cannot occupy a caller-saved register. */
static void build_interf_graph_cfg(const IRFunction *fn, const CFG *cfg,
                                    int nv, InterfGraph *g,
                                    int *forbid_mask,
                                    int float_class,
                                    const RegClass *cls) {
    ig_init(g, nv);
    for (int v = 0; v < nv; v++) forbid_mask[v] = 0;

    BitSet *use_b = xmalloc(cfg->num * sizeof(BitSet));
    BitSet *def_b = xmalloc(cfg->num * sizeof(BitSet));
    BitSet *in_b  = xmalloc(cfg->num * sizeof(BitSet));
    BitSet *out_b = xmalloc(cfg->num * sizeof(BitSet));
    for (size_t bi = 0; bi < cfg->num; bi++) {
        bs_init(&use_b[bi], nv);
        bs_init(&def_b[bi], nv);
        bs_init(&in_b[bi], nv);
        bs_init(&out_b[bi], nv);
    }

    compute_use_def(fn, cfg, use_b, def_b);
    compute_live_in_out(cfg, use_b, def_b, in_b, out_b);

    /* Walk blocks, maintain a live-set (start = out[b]), step backward. */
    BitSet live;
    bs_init(&live, nv);

    for (size_t bi = 0; bi < cfg->num; bi++) {
        const CFGBlock *blk = &cfg->blocks[bi];
        bs_copy(&live, &out_b[bi]);

        /* Walk instructions backwards. */
        for (size_t i = blk->end; i > blk->start; i--) {
            const IRInst *inst = &fn->insts.data[i - 1];
            if (inst->op == IR_LABEL || inst->op == IR_BR) continue;

            /* Before defining dst — if this is a CALL, every value that
             * is live AFTER the call (i.e. in `live` right now, minus dst
             * itself, since dst is being redefined) must survive the call.
             * A caller-saved register would be clobbered, so forbid those
             * colors for such values. */
            if (inst->op == IR_CALL) {
                BS_FOREACH(&live, over) {
                    if ((int)over != inst->dst &&
                        value_in_class(fn, (int)over, float_class))
                        forbid_mask[over] |= cls->caller_saved;
                }
            }

            /* dst interferes with every currently-live value — but only
             * values in the target class form edges in this graph. */
            if (inst->dst >= 0 && inst->dst < nv &&
                value_in_class(fn, inst->dst, float_class)) {
                BS_FOREACH(&live, other) {
                    if (!value_in_class(fn, (int)other, float_class))
                        continue;
                    if ((int)other != inst->dst) {
                        ig_add_edge(g, inst->dst, (int)other);
                    }
                }
                bs_clr(&live, inst->dst);
            }

            /* Uses become live *before* this instruction — but only
             * track uses that belong to the target class. */
            if (inst->a >= 0 && inst->a < nv &&
                value_in_class(fn, inst->a, float_class))
                bs_set(&live, inst->a);
            if (inst->op != IR_CBR &&
                inst->b >= 0 && inst->b < nv &&
                value_in_class(fn, inst->b, float_class))
                bs_set(&live, inst->b);
            if (inst->op == IR_CALL) {
                for (int k = 0; k < inst->call_nargs; k++) {
                    IRValue av = inst->call_args[k];
                    if (av >= 0 && av < nv &&
                        value_in_class(fn, av, float_class))
                        bs_set(&live, av);
                }
            }
        }
    }

    bs_free(&live);
    for (size_t bi = 0; bi < cfg->num; bi++) {
        bs_free(&use_b[bi]);
        bs_free(&def_b[bi]);
        bs_free(&in_b[bi]);
        bs_free(&out_b[bi]);
    }
    free(use_b); free(def_b); free(in_b); free(out_b);
}


/* ================================================================== */
/* MCS — Maximum Cardinality Search                                    */
/*                                                                      */
/* For chordal graphs, the REVERSE of the MCS order is a Perfect        */
/* Elimination Ordering.  We return the order in which vertices should  */
/* be COLORED (i.e., reverse MCS = PEO).                                */
/* ================================================================== */

static int *compute_mcs_order(const InterfGraph *g) {
    int n = g->n;
    int *order = xmalloc(n * sizeof(int));
    int *weight = xmalloc(n * sizeof(int));
    int *picked = xmalloc(n * sizeof(int));

    memset(weight, 0, n * sizeof(int));
    memset(picked, 0, n * sizeof(int));

    /* Build order from last-coloring to first-coloring.
     * pos goes n-1 → 0, storing vertices to be colored last → first. */
    for (int pos = n - 1; pos >= 0; pos--) {
        int best = -1, best_w = -1;
        for (int v = 0; v < n; v++) {
            if (!picked[v] && weight[v] > best_w) {
                best = v;
                best_w = weight[v];
            }
        }
        /* If all remaining vertices have weight 0, pick any unpicked one. */
        if (best < 0) {
            for (int v = 0; v < n; v++) {
                if (!picked[v]) { best = v; break; }
            }
        }

        picked[best] = 1;
        /* "best" was the highest-weight vertex → it goes late in coloring order.
         * We place it at position pos, which goes from n-1 down to 0.
         * So order[0] = first to color (last MCS pick), order[n-1] = last to color. */
        order[pos] = best;

        /* Increment weights of unpicked neighbors. */
        for (size_t j = 0; j < g->nodes[best].degree; j++) {
            int w = g->nodes[best].neighbors[j];
            if (!picked[w]) weight[w]++;
        }
    }

    free(weight);
    free(picked);
    return order;
}

/* ================================================================== */
/* Loop-nesting depth (used for spill-cost estimation)                  */
/* ================================================================== */

/* Walk up the dominator tree from a block to compute a simple loop-depth
 * heuristic: a backedge in the CFG means the target block is a loop header,
 * and every block dominated by it is inside the loop.  For this minimal
 * allocator we just check whether any edge from b goes to a dominator of b
 * (a backedge), which implies b is inside a loop. */
static int estimate_loop_depth(int block_id, const CFG *cfg) {
    int depth = 0;
    /* Simple heuristic: if a block has a successor that strictly dominates it,
     * it is inside a loop.  Each such level adds 1 to depth. */
    const CFGBlock *b = &cfg->blocks[block_id];
    for (size_t s = 0; s < b->num_succs; s++) {
        int succ = b->succs[s];
        for (size_t p = 0; p < cfg->blocks[succ].num_preds; p++) {
            /* Check if succ is also a predecessor of another block that
             * reaches back to block_id → backedge heuristic */
            if (succ <= block_id) { depth++; break; }
        }
    }
    return depth > 0 ? 1 : 0;  /* Binary: in-loop or not-in-loop */
}

/* Find the CFG block containing instruction at index inst_idx. */
static int find_block_for_inst(const CFG *cfg, int inst_idx) {
    for (size_t bi = 0; bi < cfg->num; bi++) {
        if ((size_t)inst_idx >= cfg->blocks[bi].start &&
            (size_t)inst_idx < cfg->blocks[bi].end)
            return (int)bi;
    }
    return 0;
}

/* ================================================================== */
/* Spill-cost estimation                                               */
/* ================================================================== */

static int compute_spill_cost(int v, const LiveInfo *liv, const CFG *cfg) {
    const LiveInfo *l = &liv[v];
    int cost = 0;
    for (size_t i = 0; i < l->num_uses; i++) {
        int blk = find_block_for_inst(cfg, l->use_points[i]);
        cost += 1 + estimate_loop_depth(blk, cfg) * 10;
    }
    /* Add cost at def point too. */
    if (l->def_point >= 0) {
        int blk = find_block_for_inst(cfg, l->def_point);
        cost += 1 + estimate_loop_depth(blk, cfg) * 10;
    }
    /* Prevent zero cost (even unused values need some cost). */
    return cost > 0 ? cost : 1;
}

/* ================================================================== */
/* Greedy coloring with spill heuristic                                */
/* ================================================================== */

static void greedy_color(const InterfGraph *g, const int *order,
                         const LiveInfo *liv, const CFG *cfg,
                         const int *forbid_mask,
                         int *colors, int *spill_slots, int *num_spills,
                         const RegClass *cls) {
    int n = g->n;
    int k = cls->nregs;

    /* Track spill costs for eviction decisions. */
    int *spill_cost = xmalloc(n * sizeof(int));
    for (int v = 0; v < n; v++)
        spill_cost[v] = compute_spill_cost(v, liv, cfg);

    /* color = -1 means "not yet assigned / spilled pending decision"
     * We use -2 to mean "spilled" */
    for (int v = 0; v < n; v++) colors[v] = -1;

    int next_spill = 0;

    for (int i = 0; i < n; i++) {
        int v = order[i];
        if (g->nodes[v].degree == 0 && liv[v].num_uses == 0 && liv[v].def_point < 0) {
            /* Unused value — don't waste a register. */
            colors[v] = -2;
            spill_slots[v] = next_spill++;
            continue;
        }

        /* Find which colors are used by already-colored neighbors, plus
         * any colors this value has been forbidden from. */
        int used = forbid_mask[v];
        for (size_t j = 0; j < g->nodes[v].degree; j++) {
            int w = g->nodes[v].neighbors[j];
            if (colors[w] >= 0 && colors[w] < k)
                used |= (1 << colors[w]);
        }

        /* Find first free register. */
        int c;
        for (c = 0; c < k; c++)
            if (!(used & (1 << c))) break;

        if (c < k) {
            colors[v] = c;
        } else {
            /* All k registers are in use by neighbors (including those `v` is
             * forbidden from).  Among neighbors, prefer to evict one whose
             * register `v` is actually allowed to take — i.e. a color NOT set
             * in forbid_mask[v] (a callee-saved reg for a value live across a
             * call).  Evicting a neighbor only to hand `v` a forbidden
             * caller-saved reg would clobber `v` at the next call, so that
             * victim is useless; fall through to spilling `v` instead. */
            int victim = -1, victim_cost = 0x7fffffff;
            for (size_t j = 0; j < g->nodes[v].degree; j++) {
                int w = g->nodes[v].neighbors[j];
                if (colors[w] < 0 || colors[w] >= k) continue;
                if (forbid_mask[v] & (1 << colors[w])) continue; /* v can't use it */
                if (spill_cost[w] < victim_cost) {
                    victim = w;
                    victim_cost = spill_cost[w];
                }
            }

            if (victim >= 0 && victim_cost < spill_cost[v]) {
                /* Evict victim, give its register to v. */
                colors[v] = colors[victim];
                colors[victim] = -2;
                spill_slots[victim] = next_spill++;
            } else {
                /* Spill v itself. */
                colors[v] = -2;
                spill_slots[v] = next_spill++;
            }
        }
    }

    *num_spills = next_spill;
    free(spill_cost);
}

/* ================================================================== */
/* Coalescing — identifies but does NOT modify redundant COPYs.
 * Codegen uses the RAResult to skip COPYs where src and dst share
 * the same register. */
static void coalesce_copies(IRFunction *fn, int *colors) {
    (void)fn;
    (void)colors;
    /* No IR modification.  Codegen checks ra->reg[dst] == ra->reg[a]
     * for IR_COPY instructions and skips them when coalesced. */
}

/* ================================================================== */
/* Shared per-class allocation                                         */
/* ================================================================== */

/* Run the full allocation pipeline for one register class.  Values not
 * belonging to `float_class` are ignored (they belong to the other
 * class's run).  Returns NULL when the function has no values of the
 * target class — codegen treats a NULL result as "nothing of this class
 * to allocate". */
static RAResult *ra_alloc_class(const IRFunction *fn, int float_class,
                                const RegClass *cls) {
    int nv = fn->next_value_id;
    if (nv <= 0) return NULL;

    /* Short-circuit: if the function has no values of this class, don't
     * bother building a graph.  Keeps ra_xmm NULL for int-only code. */
    {
        int any = 0;
        for (int v = 0; v < nv; v++) {
            if (value_in_class(fn, v, float_class)) { any = 1; break; }
        }
        if (!any) return NULL;
    }

    LiveInfo *liv = compute_liveness(fn);
    if (!liv) return NULL;

    /* Build CFG (for spill-cost loop-depth heuristic AND CFG-aware liveness). */
    CFG cfg;
    cfg_build(&cfg, &fn->insts);

    /* Build interference graph using CFG-aware backward walk — restricted
     * to values of the target class.  Correct across loop back edges. */
    InterfGraph g;
    int *forbid_mask = xmalloc(nv * sizeof(int));
    build_interf_graph_cfg(fn, &cfg, nv, &g, forbid_mask,
                            float_class, cls);

    /* Compute MCS ordering (reverse = PEO for chordal graph). */
    int *order = compute_mcs_order(&g);

    /* Greedy coloring (color = index into cls->regs). */
    int *colors = xmalloc(nv * sizeof(int));
    int *spill_slots = xmalloc(nv * sizeof(int));
    memset(spill_slots, 0, nv * sizeof(int));
    int num_spills = 0;
    greedy_color(&g, order, liv, &cfg, forbid_mask,
                 colors, spill_slots, &num_spills, cls);
    free(forbid_mask);

    /* Map color indices (0..cls->nregs-1) to actual x86-64 register
     * encodings that codegen uses for ModRM.  Values not in this class
     * get REG_NONE (their real home is the other class's result). */
    for (int v = 0; v < nv; v++) {
        if (!value_in_class(fn, v, float_class)) {
            colors[v] = REG_NONE;
        } else if (colors[v] >= 0 && colors[v] < cls->nregs) {
            colors[v] = cls->regs[colors[v]];
        } else {
            /* Spilled: keep color as-is (negative), spill_slot valid. */
        }
    }

    /* Coalesce COPYs with same-register src/dst. */
    coalesce_copies((IRFunction *)fn, colors);

    /* Build result. */
    RAResult *ra = xmalloc(sizeof(RAResult));
    ra->reg = colors;
    ra->spill_slot = spill_slots;
    ra->num_spill_slots = num_spills;
    ra->num_values = nv;

    /* Calculate stack size for spill slots, 16-byte aligned. */
    int slots = num_spills;
    if (slots % 2 != 0) slots++;  /* 2 slots = 16 bytes → alignment */
    ra->stack_size = 8 * slots;

    /* Cleanup. */
    free(order);
    ig_free(&g);
    liv_free(liv, nv);
    cfg_free(&cfg);

    return ra;
}

/* ================================================================== */
/* Top-level entry points                                               */
/* ================================================================== */

RAResult *reg_alloc(const IRFunction *fn) {
    return ra_alloc_class(fn, 0, &GP_CLASS);
}

RAResult *reg_alloc_xmm(const IRFunction *fn) {
    return ra_alloc_class(fn, 1, &XMM_CLASS);
}

void ra_result_free(RAResult *ra) {
    if (!ra) return;
    free(ra->reg);
    free(ra->spill_slot);
    free(ra);
}
