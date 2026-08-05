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

/* Registers available for allocation (GP file).
 *
 * Excluded (reserved for codegen scratch / ABI / frame):
 *   RAX  — return value + comparison/arith staging
 *   RCX  — 2nd operand staging + shift count
 *   RDX  — division high / comparison output staging
 *   RSP  — stack pointer
 *   RBP  — frame pointer
 *   R14/R15 — reserved for future extension
 *
 * By keeping RAX/RCX/RDX out of the allocation pool we guarantee that
 * codegen's two-source register staging can never collide with an
 * allocated value's home register. */
#define REG_ALLOCATABLE  9

/* The subset of allocatable registers that the allocator actually uses. */
static const int ALLOCATABLE_REGS[REG_ALLOCATABLE] = {
    REG_RSI, REG_RDI,
    REG_R8,  REG_R9,  REG_R10, REG_R11,
    REG_RBX, REG_R12, REG_R13
};

/* Caller-saved mask over ALLOCATABLE_REGS indices: RSI/RDI/R8/R9/R10/R11
 * are indices 0..5. Values live across a call may not occupy these. */
#define GP_CALLER_SAVED_MASK  0x3Fu

/* ------------------------------------------------------------------ */
/* XMM register file (SSE/AVX scalar)                                  */
/*                                                                      */
/* Encoded 0-15 matching native XMM ModRM register codes (bits 0-2 in  */
/* ModRM, bit 3 via REX.B / REX.R for xmm8-xmm15). All XMM registers   */
/* are caller-saved under SysV AMD64, so the forbid-mask forbids every  */
/* allocatable XMM color for values live across a call — they spill.    */
/*                                                                      */
/* XMM14/XMM15 are reserved as codegen scratch (analogous to RAX/RCX   */
/* for the GP file): the allocator never hands them out, and codegen    */
/* freely clobbers them when staging SSE operations.                     */
/* ------------------------------------------------------------------ */

#define REG_XMM_ALLOCATABLE  14

/* Allocatable XMM registers: xmm0 .. xmm13. */
static const int XMM_ALLOCATABLE_REGS[REG_XMM_ALLOCATABLE] = {
    /* xmm0 .. xmm13 */
     0,  1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13
};

/* Scratch XMM registers used by codegen for staging (never allocated). */
#define XMM_SCRATCH0  14
#define XMM_SCRATCH1  15

/* Every allocatable XMM register is caller-saved → forbid all colors. */
#define XMM_CALLER_SAVED_MASK  ((1u << REG_XMM_ALLOCATABLE) - 1u)

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

/* Allocate GP registers for all non-float SSA values in the function.
 *
 * Runs liveness analysis, builds the SSA interference graph (chordal)
 * over GP-class values, computes a perfect elimination ordering via MCS,
 * and greedily colors the graph.  Values that cannot be colored are
 * spilled to the stack.
 *
 * The function's IR is NOT modified — only a mapping from value ids to
 * registers is produced.  Codegen reads this mapping to emit efficient
 * register-to-register instructions.
 *
 * Returns: a malloc'd RAResult.  Must be freed by ra_result_free().
 *          Returns NULL if the function has no instructions.  */
RAResult *reg_alloc(const IRFunction *fn);

/* Allocate XMM registers for all float SSA values in the function.
 *
 * Same algorithm as reg_alloc but restricted to values flagged
 * value_is_float==1, colored from the XMM register file.  GP-class
 * values are ignored (they never occupy XMM regs).  The result is
 * stored separately (fn->ra_xmm) and consulted by codegen for every
 * is_float instruction.
 *
 * Returns NULL if the function has no float SSA values. */
RAResult *reg_alloc_xmm(const IRFunction *fn);

/* Free a RAResult returned by reg_alloc() or reg_alloc_xmm(). */
void ra_result_free(RAResult *ra);

#endif /* FAKECC_REGALLOC_H */
