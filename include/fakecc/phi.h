#ifndef FAKECC_PHI_H
#define FAKECC_PHI_H

#include "fakecc/ir.h"
#include "fakecc/cfg.h"
#include "fakecc/domtree.h"
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* SSA φ nodes — internal IR, never serialized to flat IR              */
/* ------------------------------------------------------------------ */

/* One argument to a φ node: (value from predecessor block `pred`). */
typedef struct { IRValue val; int pred; } PhiArg;

/* A φ node merges values from multiple CFG predecessors into one SSA
 * value. It exists only during mem2reg and is resolved to COPY instructions
 * before the flat IR is written back. */
typedef struct {
    IRValue dst;          /* φ result — a new SSA value             */
    int alloca_slot;      /* which alloca variable this φ merges     */
    PhiArg *args;         /* dynamic array of (val, pred) pairs      */
    size_t num_args;
    size_t cap_args;
    SourceLoc loc;
} IRPhi;

/* Per-block φ storage, indexed by CFG block id.  The CFG module
 * (cfg.h) does not know about φ nodes, so we keep them separately. */
typedef struct {
    IRPhi *phis;
    size_t num_phis;
    size_t cap_phis;
} BlockPhiInfo;

/* ------------------------------------------------------------------ */
/* mem2reg — φ placement phase                                        */
/* ------------------------------------------------------------------ */

/* Place φ nodes for all promotable alloca variables at dominance frontiers.
 *
 * Parameters:
 *   cfg           — control-flow graph (num blocks, predecessors/successors)
 *   dt            — dominator tree with dominance frontiers computed
 *   alloca_slots  — array of ALLOCA instruction dst values (one per variable)
 *   num_alloca    — length of alloca_slots
 *   block_stores  — bitmap: block_stores[block_id][slot_idx] == 1 iff the
 *                   block contains a STORE to alloca_slots[slot_idx].
 *                   Caller owns this memory.
 *   next_value_id — [in/out] function's SSA id counter; φ results consume
 *                   new ids, so the function-written value is returned here.
 *
 * Returns: a malloc'd BlockPhiInfo array of length cfg->num.  For each block,
 *          the phis array lists φ nodes (if any).  Caller must free each
 *          block's phis as well as the BlockPhiInfo array itself.
 *
 * Algorithm: classic iterative worklist (Cytron et al., 1991):
 *   1. For each alloca, collect blocks with stores → initial def_blocks.
 *   2. Push def_blocks onto the worklist.
 *   3. While the worklist is non-empty: pop block X; for each Y in DF[X],
 *      if Y does not already have a φ for this alloca, insert one and
 *      add Y to def_blocks + worklist (propagating to DF of Y, etc.).
 */
BlockPhiInfo *mem2reg_place_phis(
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    const char **block_stores,
    int *next_value_id);

/* Free all memory owned by a BlockPhiInfo array returned by mem2reg_place_phis.
 * Safe to call on a zeroed-out array. */
void block_phi_info_free(BlockPhiInfo *bp, size_t num_blocks);

#endif /* FAKECC_PHI_H */
