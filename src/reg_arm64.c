#include "fakecc/reg_arm64.h"

/* Allocatable GP registers in allocator color order: caller-saved first
 * (x2..x15), then callee-saved (x19..x28).
 *
 * x8 is allocatable: it only carries the indirect-result address at call
 * boundaries (caller-saved, like x0-x7), so codegen loads it explicitly
 * around aggregate-return calls; inside a body it is an ordinary scratch
 * register. */
const int a64_gp_allocatable[A64_GP_NREGS] = {
    A64_X2,  A64_X3,  A64_X4,  A64_X5,
    A64_X6,  A64_X7,  A64_X8,
    A64_X9,  A64_X10, A64_X11, A64_X12,
    A64_X13, A64_X14, A64_X15,
    A64_X19, A64_X20, A64_X21, A64_X22,
    A64_X23, A64_X24, A64_X25, A64_X26,
    A64_X27, A64_X28,
};

/* Allocatable SIMD registers in color order: caller v0..v7, v16..v29,
 * then callee v8..v15. */
const int a64_vec_allocatable[A64_VEC_NREGS] = {
    A64_V0,  A64_V1,  A64_V2,  A64_V3,
    A64_V4,  A64_V5,  A64_V6,  A64_V7,
    A64_V16, A64_V17, A64_V18, A64_V19,
    A64_V20, A64_V21, A64_V22, A64_V23,
    A64_V24, A64_V25, A64_V26, A64_V27,
    A64_V28, A64_V29,
    A64_V8,  A64_V9,  A64_V10, A64_V11,
    A64_V12, A64_V13, A64_V14, A64_V15,
};

const RaRegClasses *ra_classes_arm64(void) {
    static const RaRegClasses classes = {
        .gp = {
            a64_gp_allocatable,
            A64_GP_NREGS,
            A64_GP_CALLER_MASK,
            8,    /* GP spill slot: 8 bytes (frame kept 16-byte aligned) */
            0,    /* no x87-style long-double exclusion on arm64 */
        },
        .simd = {
            a64_vec_allocatable,
            A64_VEC_NREGS,
            A64_VEC_CALLER_MASK,
            16,   /* one NEON Q register per SIMD spill slot */
            0,
        },
    };
    return &classes;
}
