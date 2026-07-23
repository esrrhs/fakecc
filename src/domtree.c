#include "fakecc/domtree.h"
#include "fakecc/common.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void domtree_build(DomTree *dt, const CFG *cfg) {
    size_t n = cfg->num;
    dt->n = n;
    dt->idom = xmalloc(n * sizeof(int));
    for (size_t i = 0; i < n; i++) dt->idom[i] = -1;

    /* Entry dominates itself — mark with a sentinel distinct from -1,
     * so it is recognised as "processed" during the dataflow iteration. */
    dt->idom[cfg->entry] = (int)cfg->entry;

    /* --- RPO numbering for the intersect step ---
     * The Cooper-Harvey-Kennedy intersect walks up idom chains
     * comparing RPO numbers: while (rpo[f1] > rpo[f2]) f1 = idom[f1].
     * This is correct as long as rpo[entry] == 0 and idom nodes always
     * have strictly smaller RPO numbers than their children. */
    int *rpo = cfg_rpo(cfg);
    if (!rpo) {
        /* cfg_rpo failed — fall back to block-id comparison (likely
         * correct for simple CFGs, but not guaranteed). */
        rpo = xmalloc(n * sizeof(int));
        for (size_t i = 0; i < n; i++) rpo[i] = (int)i;
    }

    /* --- Iterative dataflow --- */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (size_t bi = 0; bi < n; bi++) {
            int b = (int)bi;
            if (b == cfg->entry) continue;
            CFGBlock *blk = &cfg->blocks[bi];

            /* Pick the first processed predecessor as the initial guess. */
            int new_idom = -1;
            for (size_t i = 0; i < blk->num_preds; i++) {
                int p = blk->preds[i];
                if (dt->idom[p] != -1) {
                    new_idom = p;
                    break;
                }
            }
            if (new_idom == -1) continue; /* no processed predecessor yet */

            /* Intersect with the remaining processed predecessors. */
            for (size_t i = 0; i < blk->num_preds; i++) {
                int p = blk->preds[i];
                if (p == new_idom) continue;
                if (dt->idom[p] == -1) continue;

                /* Intersect: find the lowest common ancestor on the dom tree.
                 * Uses RPO numbering so we can walk upward reliably. */
                int f1 = new_idom, f2 = p;
                while (f1 != f2) {
                    while (rpo[f1] > rpo[f2]) f1 = dt->idom[f1];
                    while (rpo[f2] > rpo[f1]) f2 = dt->idom[f2];
                }
                new_idom = f1;
            }

            if (dt->idom[b] != new_idom) {
                dt->idom[b] = new_idom;
                changed = 1;
            }
        }
    }

    free(rpo);

    /* Entry's idom is conventionally -1 (no parent). */
    dt->idom[cfg->entry] = -1;

    /* --- Dominance frontiers ---
     * For each block b with ≥ 2 predecessors: for each predecessor p,
     * walk up the idom chain from p toward idom[b], adding b to
     * DF[runner] for each intermediate node. */
    dt->df = xmalloc(n * sizeof(int *));
    dt->df_len = xmalloc(n * sizeof(size_t));
    for (size_t i = 0; i < n; i++) {
        dt->df[i] = NULL;
        dt->df_len[i] = 0;
    }

    for (size_t bi = 0; bi < n; bi++) {
        CFGBlock *b = &cfg->blocks[bi];
        if (b->num_preds < 2) continue;
        for (size_t i = 0; i < b->num_preds; i++) {
            int runner = b->preds[i];
            while (runner != -1 && runner != dt->idom[bi] && runner != (int)bi) {
                /* Add (int)bi to df[runner] if not already present */
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

int domtree_dominates(const DomTree *dt, int a, int b) {
    int runner = b;
    while (runner != -1) {
        if (runner == a) return 1;
        if (runner == dt->idom[runner]) break; /* safety: self-loop */
        runner = dt->idom[runner];
    }
    return 0;
}

void domtree_free(DomTree *dt) {
    for (size_t i = 0; i < dt->n; i++) free(dt->df[i]);
    free(dt->df);
    free(dt->df_len);
    free(dt->idom);
    dt->idom = NULL;
    dt->df = NULL;
    dt->df_len = NULL;
    dt->n = 0;
}
