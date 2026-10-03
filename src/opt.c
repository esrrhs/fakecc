#include "fakecc/opt.h"
#include "fakecc/common.h"
#include "fakecc/ir.h"
#include "fakecc/mem2reg.h"
#include "fakecc/scalar_opt.h"
#include "fakecc/regalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Pull all IR_DBG_VALUE markers out of fn->insts into `out` (realloc'd,
 * *nout entries).  Compacts fn->insts in place to remove them.  Each
 * marker records `pos` = the number of real (non-marker) instructions that
 * precede it, so it can be put back at the exact same spot afterwards.
 * Must run AFTER scalar_cleanup, whose scalar_renumber already remaps each
 * marker's value ref to the compacted id space. */
static void extract_markers(IRFunction *fn, ExtractedMarker **out, int *nout) {
    *out = NULL;
    *nout = 0;
    int pos = 0;
    size_t w = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IRInst *in = &fn->insts.data[i];
        if (in->op != IR_DBG_VALUE) {
            fn->insts.data[w++] = *in;
            pos++;
            continue;
        }
        *out = xrealloc(*out, ((size_t)*nout + 1) * sizeof(ExtractedMarker));
        (*out)[*nout].var = (int)in->imm;
        (*out)[*nout].value = in->a;
        (*out)[*nout].pos = pos;
        (*nout)++;
    }
    fn->insts.len = w;
}

/* Re-insert extracted markers at the exact positions they were extracted
 * from.  `pos` is the number of real instructions preceding each marker, so
 * we walk the (marker-free) array with a real-instruction counter and drop
 * each marker back where it belongs.  Must run AFTER regalloc so the
 * instruction layout the allocator saw is byte-for-byte what codegen emits
 * (the markers themselves emit no code). */
static void reinsert_markers(IRFunction *fn, ExtractedMarker *markers, int nmarkers) {
    IRInstArray out;
    out.len = 0;
    out.cap = fn->insts.len + (size_t)nmarkers;
    out.data = xmalloc(out.cap * sizeof(IRInst));
    int mi = 0; /* next marker to insert */
    int real = 0; /* real instructions emitted so far */
    for (size_t i = 0; i < fn->insts.len; i++) {
        /* Emit any markers whose position matches the current real count. */
        while (mi < nmarkers && markers[mi].pos == real) {
            IRInst marker;
            memset(&marker, 0, sizeof(marker));
            marker.op = IR_DBG_VALUE;
            marker.dst = -1;
            marker.imm = markers[mi].var;
            marker.a = markers[mi].value;
            if (out.len >= out.cap) { out.cap *= 2; out.data = xrealloc(out.data, out.cap * sizeof(IRInst)); }
            out.data[out.len++] = marker;
            mi++;
        }
        if (out.len >= out.cap) { out.cap *= 2; out.data = xrealloc(out.data, out.cap * sizeof(IRInst)); }
        out.data[out.len++] = fn->insts.data[i];
        real++;
    }
    /* Trailing markers (position == total real instruction count). */
    while (mi < nmarkers && markers[mi].pos == real) {
        IRInst marker;
        memset(&marker, 0, sizeof(marker));
        marker.op = IR_DBG_VALUE;
        marker.dst = -1;
        marker.imm = markers[mi].var;
        marker.a = markers[mi].value;
        if (out.len >= out.cap) { out.cap *= 2; out.data = xrealloc(out.data, out.cap * sizeof(IRInst)); }
        out.data[out.len++] = marker;
        mi++;
    }
    free(fn->insts.data);
    fn->insts = out;
}

void opt(IRModule *ir, int opt_level, int want_debug) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        IRFunction *fn = &ir->functions.data[i];
        if (opt_level == 0)
            pin_scalar_allocas(fn);
        else
            opt_mem2reg(fn, want_debug);
        if (want_debug) {
            /* scalar_cleanup's scalar_renumber already skips IR_DBG_VALUE
             * markers for id assignment and remaps their value refs, so it
             * runs correctly with the markers present.  We extract them
             * AFTER cleanup (so their recorded positions are post-cleanup =
             * final) and re-insert them AFTER regalloc, at those exact
             * positions.  The markers emit no machine code, so putting them
             * back after the allocator ran neither perturbs register choices
             * nor changes the emitted .text — keeping -g orthogonal while
             * still feeding location-list data to codegen. */
            scalar_cleanup(fn);
            ExtractedMarker *markers = NULL;
            int nmarkers = 0;
            extract_markers(fn, &markers, &nmarkers);
            fn->ra = reg_alloc(fn);
            fn->ra_xmm = reg_alloc_xmm(fn);
            if (nmarkers > 0)
                reinsert_markers(fn, markers, nmarkers);
            free(markers);
        } else {
            scalar_cleanup(fn);
            fn->ra = reg_alloc(fn);
            fn->ra_xmm = reg_alloc_xmm(fn);
        }
    }
}
