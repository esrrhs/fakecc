#ifndef FAKECC_SCALAR_OPT_H
#define FAKECC_SCALAR_OPT_H

#include "fakecc/ir.h"

/* ------------------------------------------------------------------ */
/* Scalar optimization passes (operate on flat IR).                     */
/*                                                                      */
/* These passes are designed to run after mem2reg has promoted stack    */
/* variables to SSA registers.  They form the iterative "cleanup"       */
/* phase of the optimization pipeline.                                  */
/* ------------------------------------------------------------------ */

/* Constant folding: replace ADD/SUB/MUL/DIV/MOD/NEG with CONST when
 * both operands (or the single operand for NEG) are known constants.
 * Returns 1 if any substitution was made. */
int scalar_constfold(IRFunction *fn);

/* Dead code elimination: remove instructions whose result is never
 * used and which have no side effects.  Returns 1 if any instruction
 * was removed. */
int scalar_dce(IRFunction *fn);

/* Peephole: local algebraic simplifications (x+0→x, x*0→0, x*1→x,
 * 0-x→-x, etc.).  Returns 1 if any substitution was made. */
int scalar_peephole(IRFunction *fn);

/* Value renumbering: compact SSA value ids to tighten the stack frame.
 * Does not return a "changed" flag — always rewrites. */
void scalar_renumber(IRFunction *fn);

/* Run constfold, dce, and peephole iteratively to a fixed point,
 * then renumber values.  This is the recommended convenience entry
 * point for the scalar optimization phase. */
void scalar_cleanup(IRFunction *fn);

#endif /* FAKECC_SCALAR_OPT_H */
