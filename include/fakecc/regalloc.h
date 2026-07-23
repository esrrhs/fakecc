#ifndef FAKECC_REGALLOC_H
#define FAKECC_REGALLOC_H

#include "fakecc/ir.h"

/* ------------------------------------------------------------------ */
/* x86-64 register encoding                                            */
/*                                                                      */
/* The register numbers below serve double duty:                        */
/*   - As indices into the allocator (0..REG_ALLOCATABLE-1)             */
/*   - As x86-64 ModRM register codes (bits 0-2) for the low 8 regs,    */
/*     with bits 3+ used to set REX.B / REX.R for high 8 regs (r8-r15).*/
/* ------------------------------------------------------------------ */

typedef enum {
    REG_RAX = 0,
    REG_RCX = 1,
    REG_RDX = 2,
    REG_RBX = 3,
    REG_RSP = 4,       /* reserved — never allocated */
    REG_RBP = 5,       /* reserved — frame pointer */
    REG_RSI = 6,
    REG_RDI = 7,
    REG_R8  = 8,
    REG_R9  = 9,
    REG_R10 = 10,
    REG_R11 = 11,
    REG_R12 = 12,
    REG_R13 = 13,
    REG_R14 = 14,
    REG_R15 = 15,
    REG_NONE = -1,
} Reg;

/* Registers available for allocation.
 * Excludes REG_RSP (stack pointer), REG_RBP (frame pointer),
 * REG_R14/R15 (reserved for future extension). */
#define REG_ALLOCATABLE  12

/* The subset of allocatable registers that the allocator actually uses. */
static const int ALLOCATABLE_REGS[REG_ALLOCATABLE] = {
    REG_RAX, REG_RCX, REG_RDX, REG_RSI, REG_RDI,
    REG_R8,  REG_R9,  REG_R10, REG_R11,
    REG_RBX, REG_R12, REG_R13
};

/* ------------------------------------------------------------------ */
/* Register allocation result                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    int *reg;              /* reg[v] = allocated register, or REG_NONE if spilled */
    int *spill_slot;       /* spill[v] = spill slot index (0, 1, ...), meaningful
                              only when reg[v] == REG_NONE */
    int num_spill_slots;   /* total number of spill slots needed */
    int num_values;        /* fn->next_value_id — number of SSA values */
    int stack_size;        /* total stack allocation (16-byte aligned) for spills */
} RAResult;

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

/* Allocate registers for all SSA values in the function.
 *
 * Runs liveness analysis, builds the SSA interference graph (chordal),
 * computes a perfect elimination ordering via MCS, and greedily colors
 * the graph.  Values that cannot be colored are spilled to the stack.
 *
 * The function's IR is NOT modified — only a mapping from value ids to
 * registers is produced.  Codegen reads this mapping to emit efficient
 * register-to-register instructions.
 *
 * Returns: a malloc'd RAResult.  Must be freed by ra_result_free().
 *          Returns NULL if the function has no instructions.  */
RAResult *reg_alloc(const IRFunction *fn);

/* Free a RAResult returned by reg_alloc().  Safe to call with NULL. */
void ra_result_free(RAResult *ra);

#endif /* FAKECC_REGALLOC_H */
