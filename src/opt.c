#include "fakecc/opt.h"
#include "fakecc/mem2reg.h"
#include "fakecc/scalar_opt.h"
#include "fakecc/regalloc.h"

/* Optimization pipeline                                                */
/*                                                                      */
/* Pipeline order (per function):                                       */
/*   opt_mem2reg  →  scalar_cleanup  →  reg_alloc                       */
/*                                                                      */
/*   scalar_cleanup runs constfold, peephole, and dce iteratively       */
/*   to a fixed point, then renumbers value ids.                        */
/*   reg_alloc uses SSA chordal-graph optimal coloring to assign        */
/*   x86-64 registers; codegen reads the result to emit efficient       */
/*   register-to-register instructions.                                 */

void opt(IRModule *ir) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        IRFunction *fn = &ir->functions.data[i];
        opt_mem2reg(fn);
        scalar_cleanup(fn);
        fn->ra = reg_alloc(fn);
    }
}
