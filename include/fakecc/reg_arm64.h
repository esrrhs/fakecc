#ifndef FAKECC_REG_ARM64_H
#define FAKECC_REG_ARM64_H

/* AArch64 (A64), Darwin calling-convention register numbering.
 *
 * Register codes match the hardware encoding used in instruction fields
 * (0..31), so the allocator's output feeds the A64 encoder directly.
 *
 * GP allocatable set (24), ordered with caller-saved colors first so the
 * allocator's caller-saved bitmask stays contiguous:
 *   colors 0..13  x2..x15  caller-saved arguments/temporaries
 *   colors 14..23 x19..x28 callee-saved
 *
 * Deliberately excluded:
 *   x0        result register / argument word 0
 *   x1        second result (long long pair, struct return metadata), scratch
 *   x16, x17  linker veneer / intra-call scratch (reserved for the linker
 *             and the code generator, like R14/R15 on the x86 backend)
 *   x18       platform register (Apple ABI: reserved)
 *   x29 (FP), x30 (LR), x31 (SP/zero register)
 *
 * Note: x8 IS allocatable inside a body — it only carries the indirect-
 * result address at aggregate-return call boundaries (caller-saved,
 * like x0..x7), which codegen loads explicitly around those calls.
 *
 * SIMD allocatable set (30):
 *   colors 0..7   v0..v7    caller-saved arguments/results
 *   colors 8..21  v16..v29  caller-saved temporaries
 *   colors 22..29 v8..v15   callee-saved (the prologue saves the whole Q
 *                           register when used, covering 16-byte values)
 *   v30, v31 reserved as code-generator scratch.
 */

#include "fakecc/regalloc.h"

typedef enum {
    A64_X0  = 0,  A64_X1  = 1,  A64_X2  = 2,  A64_X3  = 3,
    A64_X4  = 4,  A64_X5  = 5,  A64_X6  = 6,  A64_X7  = 7,
    A64_X8  = 8,  A64_X9  = 9,  A64_X10 = 10, A64_X11 = 11,
    A64_X12 = 12, A64_X13 = 13, A64_X14 = 14, A64_X15 = 15,
    A64_X16 = 16, A64_X17 = 17, A64_X18 = 18,
    A64_X19 = 19, A64_X20 = 20, A64_X21 = 21, A64_X22 = 22,
    A64_X23 = 23, A64_X24 = 24, A64_X25 = 25, A64_X26 = 26,
    A64_X27 = 27, A64_X28 = 28,
    A64_FP  = 29, A64_LR  = 30, A64_SP  = 31,
    A64_GP_NONE = -1,
} A64GpReg;

/* W registers share hardware numbers with X (32-bit view). */
#define A64_W0  A64_X0
#define A64_W1  A64_X1
#define A64_W2  A64_X2
#define A64_W3  A64_X3
#define A64_W4  A64_X4
#define A64_W5  A64_X5
#define A64_W6  A64_X6
#define A64_W7  A64_X7
#define A64_W8  A64_X8
#define A64_W9  A64_X9
#define A64_W10 A64_X10
#define A64_W11 A64_X11
#define A64_W12 A64_X12
#define A64_W13 A64_X13
#define A64_W14 A64_X14
#define A64_W15 A64_X15
#define A64_W16 A64_X16
#define A64_W17 A64_X17
#define A64_W18 A64_X18
#define A64_W19 A64_X19
#define A64_W20 A64_X20
#define A64_W21 A64_X21
#define A64_W22 A64_X22
#define A64_W23 A64_X23
#define A64_W24 A64_X24
#define A64_W25 A64_X25
#define A64_W26 A64_X26
#define A64_W27 A64_X27
#define A64_W28 A64_X28

typedef enum {
    A64_V0  = 0,  A64_V1  = 1,  A64_V2  = 2,  A64_V3  = 3,
    A64_V4  = 4,  A64_V5  = 5,  A64_V6  = 6,  A64_V7  = 7,
    A64_V8  = 8,  A64_V9  = 9,  A64_V10 = 10, A64_V11 = 11,
    A64_V12 = 12, A64_V13 = 13, A64_V14 = 14, A64_V15 = 15,
    A64_V16 = 16, A64_V17 = 17, A64_V18 = 18, A64_V19 = 19,
    A64_V20 = 20, A64_V21 = 21, A64_V22 = 22, A64_V23 = 23,
    A64_V24 = 24, A64_V25 = 25, A64_V26 = 26, A64_V27 = 27,
    A64_V28 = 28, A64_V29 = 29, A64_V30 = 30, A64_V31 = 31,
    A64_V_NONE = -1,
} A64VecReg;

#define A64_GP_NREGS          24
#define A64_GP_CALLER_MASK    0x00003FFFu  /* colors 0..13 (x2..x15) */
#define A64_VEC_NREGS         30
#define A64_VEC_CALLER_MASK   0x003FFFFFu  /* colors 0..21 */

/* Allocatable register codes in allocator color order. */
extern const int a64_gp_allocatable[A64_GP_NREGS];
extern const int a64_vec_allocatable[A64_VEC_NREGS];

/* GP/SIMD register class descriptors for the arm64-macos backend. */
const RaRegClasses *ra_classes_arm64(void);

#endif /* FAKECC_REG_ARM64_H */
