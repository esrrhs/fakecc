#include "fakecc/phi.h"
#include "fakecc/cfg.h"
#include "fakecc/domtree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/* Small generic helpers (FakeCC style: malloc + exit(1) on failure)   */
/* ================================================================== */

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
