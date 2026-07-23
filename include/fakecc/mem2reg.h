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

#endif /* FAKECC_MEM2REG_H */
