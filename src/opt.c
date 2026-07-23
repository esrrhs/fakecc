#include "fakecc/opt.h"
#include "fakecc/mem2reg.h"
#include "fakecc/scalar_opt.h"

/* Optimization pipeline                                                */
/*                                                                      */
/* Pipeline order (per function):                                       */
/*   opt_mem2reg  →  scalar_cleanup                                     */
/*                                                                      */
/*   scalar_cleanup runs constfold, peephole, and dce iteratively       */
/*   to a fixed point, then renumbers value ids.                        */

void opt(IRModule *ir) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        IRFunction *fn = &ir->functions.data[i];
        opt_mem2reg(fn);
        scalar_cleanup(fn);
    }
}
