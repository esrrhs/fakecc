#include "fakecc/opt.h"
#include "fakecc/mem2reg.h"
#include "fakecc/scalar_opt.h"
#include "fakecc/regalloc.h"

/* Optimization pipeline (per function):                                */
/*   [opt_mem2reg]  →  scalar_cleanup  →  reg_alloc                     */
/*                                                                      */
/* Only -O0 skips mem2reg and pins scalars to the stack.  -g is         */
/* orthogonal: it adds IR_DBG_VALUE markers that no other pass observes.*/

static void pin_scalar_allocas(IRFunction *fn) {
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *inst = &fn->insts.data[i];
        if (inst->op != IR_ALLOCA) continue;
        if (inst->alloca_bytes > 0) continue;
        int w = inst->width > 0 ? inst->width : 4;
        if (w < 8 && !inst->is_float) {
            /* Keep natural width but align stack slot to at least width. */
            inst->alloca_bytes = w;
        } else {
            inst->alloca_bytes = w;
        }
        if (inst->alloca_bytes < 1) inst->alloca_bytes = 8;
    }
}

void opt(IRModule *ir, int opt_level, int want_debug) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        IRFunction *fn = &ir->functions.data[i];
        if (opt_level == 0)
            pin_scalar_allocas(fn);
        else
            opt_mem2reg(fn, want_debug);
        scalar_cleanup(fn);
        fn->ra = reg_alloc(fn);
        fn->ra_xmm = reg_alloc_xmm(fn);
    }
}
