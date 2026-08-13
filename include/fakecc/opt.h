#ifndef FAKECC_OPT_H
#define FAKECC_OPT_H

#include "fakecc/ir.h"

/* Optimize the IR module in place. Runs mem2reg (SSA promotion), constant
 * folding, dead code elimination, peephole, and value renumbering. Inserted
 * between ir_generate and codegen in the pipeline.
 *
 * `opt_level` 0 skips mem2reg, leaving scalars in their stack slots (like
 * `gcc -O0`); 1 and above run the full pipeline.
 *
 * `want_debug` only controls whether IR_DBG_VALUE markers tracking source
 * variables are emitted.  It must never change the generated code: debug
 * info describes whatever the optimizer decided, exactly as gcc keeps -g
 * orthogonal to -O. */
void opt(IRModule *ir, int opt_level, int want_debug);

#endif /* FAKECC_OPT_H */
