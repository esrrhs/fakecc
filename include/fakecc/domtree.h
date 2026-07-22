#ifndef FAKECC_DOMTREE_H
#define FAKECC_DOMTREE_H

#include "fakecc/cfg.h"
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Dominator tree + dominance frontiers                                */
/* ------------------------------------------------------------------ */

typedef struct {
    int *idom;        /* idom[b] = immediate dominator of b; entry = -1 */
    int **df;         /* df[b] = dominance-frontier set (array of block ids) */
    size_t *df_len;   /* length of each df set */
    size_t n;          /* number of blocks */
} DomTree;

/* Build dominator tree + dominance frontiers from a CFG.
 * Uses the Cooper-Harvey-Kennedy iterative algorithm with RPO numbering
 * for the intersect step. */
void domtree_build(DomTree *dt, const CFG *cfg);

/* Does block 'a' dominate block 'b'? O(depth) walk up the idom chain. */
int domtree_dominates(const DomTree *dt, int a, int b);

/* Free all memory owned by the dominator tree. */
void domtree_free(DomTree *dt);

#endif /* FAKECC_DOMTREE_H */
