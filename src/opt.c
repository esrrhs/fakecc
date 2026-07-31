#include "fakecc/opt.h"
#include "fakecc/mem2reg.h"
#include "fakecc/scalar_opt.h"
#include "fakecc/regalloc.h"

/* Optimization pipeline (per function):                                */
/*   opt_mem2reg  →  scalar_cleanup  →  reg_alloc                       */
/*                                                                      */
/* reg_alloc uses CFG-aware liveness that is correct across loop back    */
/* edges; all functions — straight-line or with control flow — go       */
/* through register allocation. */

void opt(IRModule *ir) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        IRFunction *fn = &ir->functions.data[i];
        opt_mem2reg(fn);
        scalar_cleanup(fn);
        fn->ra = reg_alloc(fn);
    }
}
