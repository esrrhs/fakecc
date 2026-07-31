#include "fakecc/opt.h"
#include "fakecc/mem2reg.h"
#include "fakecc/scalar_opt.h"
#include "fakecc/regalloc.h"

/* Optimization pipeline                                                */
/*                                                                      */
/* Pipeline order (per function):                                       */
/*   opt_mem2reg  →  scalar_cleanup  →  reg_alloc                       */
/*                                                                      */
/*   The chordal SSA regalloc assumes straight-line (loop-free) IR      */
/*   after mem2reg lowers φ to copies.  Once control flow (if/while)    */
/*   is present, φ-resolution introduces multi-def values and cyclic    */
/*   live ranges the interval-based liveness cannot see, so we fall    */
/*   back to the stack-slot codegen path for functions with labels.   */

static int has_control_flow(const IRFunction *fn) {
    for (size_t i = 0; i < fn->insts.len; i++) {
        IROpcode op = fn->insts.data[i].op;
        if (op == IR_LABEL || op == IR_BR || op == IR_CBR) return 1;
    }
    return 0;
}

void opt(IRModule *ir) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        IRFunction *fn = &ir->functions.data[i];
        opt_mem2reg(fn);
        scalar_cleanup(fn);
        if (!has_control_flow(fn)) {
            fn->ra = reg_alloc(fn);
        }
    }
}
