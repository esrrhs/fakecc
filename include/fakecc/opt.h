#ifndef FAKECC_OPT_H
#define FAKECC_OPT_H

#include "fakecc/ir.h"

/* Optimize the IR module in place. Runs mem2reg (SSA promotion), constant
 * folding, dead code elimination, peephole, and value renumbering. Inserted
 * between ir_generate and codegen in the pipeline. */
void opt(IRModule *ir);

#endif /* FAKECC_OPT_H */
