#ifndef FAKECC_MEM2REG_H
#define FAKECC_MEM2REG_H

#include "fakecc/ir.h"
#include "fakecc/cfg.h"
#include "fakecc/domtree.h"
#include "fakecc/phi.h"
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* mem2reg — SSA renaming phase                                        */
/* ------------------------------------------------------------------ */

/* Rename all promotable alloca variables.
 *
 * Performs a dominator-tree DFS, replacing LOAD→COPY and STORE→dead,
 * pushing/popping from per-alloca rename stacks, and filling φ arguments.
 *
 * Parameters:
 *   fn              — the function being transformed (insts modified in-place)
 *   cfg             — control-flow graph
 *   dt              — dominator tree
 *   alloca_slots    — array of alloca dst values
 *   num_alloca      — count of alloca slots
 *   block_phi_info  — φ nodes per block (from mem2reg_place_phis)
 *   dead            — output: calloc'd bitmap, dead[i]=1 if instruction i
 *                     should be removed. Caller must free().
 */
void mem2reg_rename(
    IRFunction *fn,
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    BlockPhiInfo *block_phi_info,
    char **dead);

/* ------------------------------------------------------------------ */
/* mem2reg — writeback phase                                           */
/* ------------------------------------------------------------------ */

/* Rebuild the flat instruction array, removing dead instructions and
 * resolving φ nodes into COPY instructions in predecessor blocks.
 *
 * For each φ `B3: v0 = merge(v1 from B1, v2 from B2)`:
 *   - In B1, before its terminator: emit `v0 = COPY v1`
 *   - In B2, before its terminator: emit `v0 = COPY v2`
 *
 * The rebuilt array replaces fn->insts.  The caller still owns
 * `block_phi_info` and `dead` and must free them after the call.
 */
void mem2reg_writeback(
    IRFunction *fn,
    const CFG *cfg,
    BlockPhiInfo *block_phi_info,
    char *dead);

/* ------------------------------------------------------------------ */
/* mem2reg — full pass (convenience entry point)                       */
/* ------------------------------------------------------------------ */

/* Promote stack-allocated variables to SSA registers.
 *
 * Runs the complete mem2reg pipeline:
 *   1. Build CFG + dominator tree
 *   2. Identify promotable alloca slots
 *   3. Place φ nodes at dominance frontiers (mem2reg_place_phis)
 *   4. Rename variables via dominator-tree DFS (mem2reg_rename)
 *   5. Resolve φ → COPY and rebuild flat IR (mem2reg_writeback)
 *
 * Returns: number of alloca variables promoted (0 if none). */
int opt_mem2reg(IRFunction *fn);

#endif /* FAKECC_MEM2REG_H */
