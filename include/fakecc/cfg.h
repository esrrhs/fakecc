#ifndef FAKECC_CFG_H
#define FAKECC_CFG_H

#include "fakecc/ir.h"
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Control-flow graph — standalone module                              */
/* ------------------------------------------------------------------ */

typedef struct {
    int id;
    int label;            /* IR_LABEL imm, or -1 for the entry block */
    size_t start, end;    /* half-open index range into IRInstArray  */
    int *preds;           /* predecessor block ids                    */
    size_t num_preds;
    int *succs;           /* successor block ids                      */
    size_t num_succs;
} CFGBlock;

typedef struct {
    CFGBlock *blocks;
    size_t num;
    int entry;            /* index of entry block (always 0)          */
} CFG;

/* Build the CFG from a flat instruction array.
 *
 * Block boundaries are: entry (implicit), and after IR_BR / IR_CBR /
 * IR_RETURN (terminators), and at IR_LABEL (leaders). A block's
 * instructions are [start, end). */
void cfg_build(CFG *g, const IRInstArray *insts);

/* Free all memory owned by the CFG (blocks, pred/succ arrays). */
void cfg_free(CFG *g);

/* Find the block whose label == target, or -1 if none. */
int  cfg_find_label(const CFG *g, int label);

/* Add an edge from block `from` to block `to`. No-op if duplicate. */
void cfg_link(CFG *g, int from, int to);

/* Compute reverse postorder (RPO) numbering for dominator-tree construction.
 * Returns a malloc'd array: rpo[block_id] = rpo_index.
 * Entry block always gets RPO 0. Caller must free. */
int *cfg_rpo(const CFG *g);

#endif /* FAKECC_CFG_H */
