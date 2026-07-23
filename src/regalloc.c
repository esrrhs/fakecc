#include "fakecc/regalloc.h"
#include "fakecc/cfg.h"
#include "fakecc/common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

        /* Definition */
        if (inst->dst >= 0 && inst->dst < nv) {
            liv[inst->dst].def_point = (int)i;
        }

        /* Uses */
        if (inst->a >= 0 && inst->a < nv) liv_add_use(&liv[inst->a], (int)i);
        if (inst->b >= 0 && inst->b < nv) liv_add_use(&liv[inst->b], (int)i);
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

/* Build the interference graph from liveness information.
 * Two values interfere if their live ranges overlap. */
static void build_interf_graph(const LiveInfo *liv, int n, InterfGraph *g) {
    ig_init(g, n);

    for (int v1 = 0; v1 < n; v1++) {
        if (liv[v1].def_point < 0 && liv[v1].num_uses == 0) continue; /* unused */
        for (int v2 = v1 + 1; v2 < n; v2++) {
            if (liv[v2].def_point < 0 && liv[v2].num_uses == 0) continue;

            /* No overlap? */
            if (liv[v1].live_end < liv[v2].live_start) continue;
            if (liv[v2].live_end < liv[v1].live_start) continue;

            /* Overlapping live ranges → interfere. */
            ig_add_edge(g, v1, v2);
        }
    }
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
                         int *colors, int *spill_slots, int *num_spills) {
    int n = g->n;
    int k = REG_ALLOCATABLE;

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

        /* Find which colors are used by already-colored neighbors. */
        int used = 0; /* bitmask of used registers (up to REG_ALLOCATABLE bits) */
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
            /* All k registers are in use by neighbors.
             * Spill the neighbor with the lowest spill cost, then take its reg. */
            int victim = -1, victim_cost = 0x7fffffff;
            for (size_t j = 0; j < g->nodes[v].degree; j++) {
                int w = g->nodes[v].neighbors[j];
                if (colors[w] >= 0 && colors[w] < k && spill_cost[w] < victim_cost) {
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
/* reg_alloc — top-level                                               */
/* ================================================================== */

RAResult *reg_alloc(const IRFunction *fn) {
    int nv = fn->next_value_id;
    if (nv <= 0) return NULL;

    LiveInfo *liv = compute_liveness(fn);
    if (!liv) return NULL;

    /* Build CFG (for spill-cost loop-depth heuristic). */
    CFG cfg;
    cfg_build(&cfg, &fn->insts);

    /* Build interference graph. */
    InterfGraph g;
    build_interf_graph(liv, nv, &g);

    /* Compute MCS ordering (reverse = PEO for chordal graph). */
    int *order = compute_mcs_order(&g);

    /* Greedy coloring (color = index into ALLOCATABLE_REGS). */
    int *colors = xmalloc(nv * sizeof(int));
    int *spill_slots = xmalloc(nv * sizeof(int));
    memset(spill_slots, 0, nv * sizeof(int));
    int num_spills = 0;
    greedy_color(&g, order, liv, &cfg, colors, spill_slots, &num_spills);

    /* Map color indices (0..REG_ALLOCATABLE-1) to actual x86-64
     * register encodings that codegen uses for ModRM. */
    for (int v = 0; v < nv; v++) {
        if (colors[v] >= 0 && colors[v] < REG_ALLOCATABLE)
            colors[v] = ALLOCATABLE_REGS[colors[v]];
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

void ra_result_free(RAResult *ra) {
    if (!ra) return;
    free(ra->reg);
    free(ra->spill_slot);
    free(ra);
}
