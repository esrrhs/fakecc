#include "fakecc/cg64.h"
#include "fakecc/a64.h"
#include "fakecc/macho.h"
#include "fakecc/reg_arm64.h"
#include "fakecc/common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/* arm64 (Darwin AAPCS64) instruction selector.                        */
/*                                                                      */
/* Frame:  [fp+0]    saved x29/x30                                      */
/*         [fp+16..] callee-saved register pair saves                  */
/*         [fp-...]  locals: pinned allocas, GP spills, call area      */
/* x16/x17 (IP0/IP1) are scratch; the allocator never assigns them.   */
/* ================================================================== */

#define SCR0 A64_X16
#define SCR1 A64_X17

typedef struct {
    A64Asm           *as;
    const IRModule   *ir;
    const IRFunction *fn;
    const RAResult   *ra;
    int              *def;         /* SSA value -> defining inst index  */
    int              *alloca_off;  /* ALLOCA SSA id -> fp-relative off  */
    int              *spill_off;   /* spill slot -> fp-relative offset  */
    int              *vlabels;     /* IR label id -> a64 label id       */
    int              *fn_label;    /* function index -> a64 label id    */
    int               frame_locals;
    int               call_area;
    int               save_total;  /* fp/lr + callee-saved saves bytes  */
    /* Module globals: section (G_RO/G_DATA/G_BSS) and in-section offset. */
    int              *gsect;
    size_t           *goff;
    /* ADRP+ADD pairs targeting globals; patched once the final section
     * placement (which depends on the total text length) is known. */
    struct GFix { uint32_t at; int gidx; } *gfix;
    size_t            ngfix, capgfix;
    /* Pointer slots inside global initializers.  Resolved into dyld
     * rebases once every section base is known. */
    struct PFix {
        int         gidx;
        int         slot_off;
        const char *sym;
        int         addend;
    }                *pfix;
    size_t            npfix, cappfix;
} C64;

enum { G_RO = 1, G_DATA = 2, G_BSS = 3 };

static void c64_die(C64 *c, const IRInst *s, const char *what) {
    const char *f = c->fn->loc.file ? c->fn->loc.file : "<arm64>";
    int line = s ? s->loc.line : c->fn->loc.line;
    die_at(f, line, s ? s->loc.col : 0,
           "arm64 backend: %s not supported yet", what);
}

/* ── Value locations ──────────────────────────────────────────────── */

static int vw(C64 *c, IRValue v) {
    if (v >= 0 && c->fn->value_width &&
        v < c->fn->next_value_id && c->fn->value_width[v])
        return c->fn->value_width[v];
    return 8;
}

/* The allocator stores NATIVE hardware register codes in ra->reg[]
 * (regalloc maps palette colors through a64_gp_allocatable once; see
 * test_regalloc's A64_X19..A64_X28 assertion).  Indexing the palette
 * table a second time here used to be a no-op only for low colors and
 * ran out of bounds (into the adjacent SIMD table) for colors >= 20,
 * aliasing distinct values onto x0..x4 and misclassifying saves. */
static int home_reg(C64 *c, IRValue v) {
    if (!c->ra || v < 0 || v >= c->ra->num_values) return -1;
    int hw = c->ra->reg[v];
    return hw >= 0 ? hw : -1;
}

static int spill_off(C64 *c, IRValue v) {
    if (!c->ra || v < 0 || v >= c->ra->num_values || c->ra->reg[v] >= 0)
        return 0;
    return c->spill_off[c->ra->spill_slot[v]];
}

/* Forward declarations: frame/base memory accessors (defined below). */
static void emit_load(C64 *c, int rt, int base, int width, int uns);
static void emit_store(C64 *c, int rt, int base, int width);
static void frame_load(C64 *c, int rt, int off, int width, int uns);
static void frame_store(C64 *c, int valrt, int off, int width);

/* Destination register for v: its home, or SCR0 when spilled. */
static int dst_reg(C64 *c, IRValue v) {
    int r = home_reg(c, v);
    return r >= 0 ? r : SCR0;
}

static void commit(C64 *c, IRValue v, int srcreg) {
    if (home_reg(c, v) >= 0) return;
    frame_store(c, srcreg, spill_off(c, v), 8);
}

static void emit_mov_imm_w(A64Asm *a, int rd, int64_t imm, int is64) {
    if (is64) {
        a64_mov_imm64(a, rd, (uint64_t)imm);
        return;
    }
    uint32_t w = (uint32_t)imm;
    a64_movz(a, rd, w & 0xFFFF, 0, 0);
    if (w >> 16) a64_movk(a, rd, w >> 16, 1, 0);
}

/* Load operand v into a register; `other` is the register already
 * holding the other operand (scratch choice must not collide). */
static int load_op(C64 *c, IRValue v, int other) {
    int scratch = (other == SCR0) ? SCR1 : SCR0;
    const IRInst *d = &c->fn->insts.data[c->def[v]];
    if (d->op == IR_CONST) {
        int hr = home_reg(c, v);
        if (hr >= 0) return hr;  /* materialized at its CONST definition */
        emit_mov_imm_w(c->as, scratch, d->imm, vw(c, v) == 8);
        return scratch;
    }
    int r = home_reg(c, v);
    if (r >= 0) return r;
    frame_load(c, scratch, spill_off(c, v), 8, 1);
    return scratch;
}

static int load_ptrv(C64 *c, IRValue v, int other) {
    int r = home_reg(c, v);
    if (r >= 0) return r;
    int scratch = (other == SCR0) ? SCR1 : SCR0;
    frame_load(c, scratch, spill_off(c, v), 8, 1);
    return scratch;
}

/* SP is not encodable in ORR (Rm/Rn=31 means XZR): move via ADD #0. */
static void mov_sp_like(A64Asm *a, int rd, int rn) {
    a64_add_imm12(a, rd, rn, 0, 0, 1, 0);
}

/* rd = fp + off (off usually negative).  The offset is materialized into
 * rd itself when it does not fit imm12, so no second scratch is needed
 * (callers may have data in x16/x17). */
static void emit_fp_addr(C64 *c, int rd, int off) {
    if (off == 0) {
        a64_mov_reg(c->as, rd, A64_FP, 1);
    } else if (off < 0 && off >= -4095) {
        a64_sub_imm12(c->as, rd, A64_FP, (unsigned)(-off), 0, 1, 0);
    } else if (off > 0 && off <= 4095) {
        a64_add_imm12(c->as, rd, A64_FP, (unsigned)off, 0, 1, 0);
    } else {
        emit_mov_imm_w(c->as, rd, off, 1);
        a64_add_reg(c->as, rd, A64_FP, rd, A64_LSL, 0, 1, 0);
    }
}

/* fp-relative loads/stores.  LDUR/STUR cover the imm9 window.  Outside
 * it the address is built in the destination itself and then loaded
 * (`ldr rt, [rt]`): the other scratch often already holds the first
 * operand of a binary op, and reusing it as an address base drops that
 * value (seen as a wrong sum once spill slots sit below fp-256). */
static void frame_load(C64 *c, int rt, int off, int width, int uns) {
    A64Asm *a = c->as;
    if (off >= -256 && off <= 255) {
        switch (width) {
        case 1:
            if (uns)
                a64_ldur8(a, rt, A64_FP, off);
            else
                a64_ldursb64(a, rt, A64_FP, off);
            break;
        case 2:
            if (uns)
                a64_ldur16(a, rt, A64_FP, off);
            else
                a64_ldursh64(a, rt, A64_FP, off);
            break;
        case 4:
            if (uns)
                a64_ldur32(a, rt, A64_FP, off);
            else
                a64_ldursw(a, rt, A64_FP, off);
            break;
        default: a64_ldur64(a, rt, A64_FP, off); break;
        }
        return;
    }
    emit_fp_addr(c, rt, off);
    emit_load(c, rt, rt, width, uns);
}

static void frame_store(C64 *c, int valrt, int off, int width) {
    A64Asm *a = c->as;
    if (off >= -256 && off <= 255) {
        switch (width) {
        case 1: a64_stur8(a, valrt, A64_FP, off); break;
        case 2: a64_stur16(a, valrt, A64_FP, off); break;
        case 4: a64_stur32(a, valrt, A64_FP, off); break;
        default: a64_stur64(a, valrt, A64_FP, off); break;
        }
        return;
    }
    int at = (valrt == SCR0) ? SCR1 : SCR0;
    emit_fp_addr(c, at, off);
    emit_store(c, valrt, at, width);
}

static void emit_load(C64 *c, int rt, int base, int width, int uns) {
    switch (width) {
    case 1:
        if (uns) a64_ldr8(c->as, rt, base, 0);
        else a64_ldrsb64(c->as, rt, base, 0);
        break;
    case 2:
        if (uns) a64_ldr16(c->as, rt, base, 0);
        else a64_ldrsh64(c->as, rt, base, 0);
        break;
    case 4:
        if (uns) a64_ldr32(c->as, rt, base, 0);
        else a64_ldrsw(c->as, rt, base, 0);
        break;
    default:
        a64_ldr64(c->as, rt, base, 0);
        break;
    }
}

static void emit_store(C64 *c, int rt, int base, int width) {
    switch (width) {
    case 1: a64_str8(c->as, rt, base, 0); break;
    case 2: a64_str16(c->as, rt, base, 0); break;
    case 4: a64_str32(c->as, rt, base, 0); break;
    default: a64_str64(c->as, rt, base, 0); break;
    }
}

/* ── Calls ────────────────────────────────────────────────────────── */

static int find_global_idx(const IRModule *ir, const char *name) {
    for (size_t k = 0; k < ir->globals.len; k++)
        if (strcmp(ir->globals.data[k].name, name) == 0)
            return (int)k;
    return -1;
}

static int find_function(const IRModule *ir, const char *name, int *idx_out) {
    for (size_t i = 0; i < ir->functions.len; i++)
        if (strcmp(ir->functions.data[i].name, name) == 0) {
            *idx_out = (int)i;
            return 0;
        }
    return -1;
}

static void emit_syscall(C64 *c, const IRInst *s);

/* Freestanding memcpy/memmove/memset.  Struct assignment above 64 bytes
 * lowers to a call, and the Darwin runtime is not linked yet (T16).
 * A user-defined function of the same name still wins.  The three
 * arguments are already in x0/x1/x2; the original destination is
 * returned in x0.  Byte loops stay correct for odd sizes and (for
 * memmove) overlapping ranges. */
static int emit_mem_builtin(C64 *c, const char *name) {
    int is_memcpy = strcmp(name, "memcpy") == 0;
    int is_memmove = strcmp(name, "memmove") == 0;
    int is_memset = strcmp(name, "memset") == 0;
    if (!is_memcpy && !is_memmove && !is_memset) return 0;
    int defined = 0;
    if (find_function(c->ir, name, &defined) == 0) return 0;

    A64Asm *a = c->as;
    int Lfwd = a64_new_label(a);
    int Lback = a64_new_label(a);
    int Lback_loop = a64_new_label(a);
    int Ldone = a64_new_label(a);

    a64_mov_reg(a, SCR1, A64_X0, 1);                 /* remember dst */
    if (is_memmove) {
        a64_cmp_reg(a, A64_X0, A64_X1, 1);
        a64_bcond(a, A64_CS, Lback);                 /* dst >= src: backward */
    }
    a64_bind(a, Lfwd);
    a64_cbz(a, A64_X2, Ldone, 1);
    if (is_memset) {
        a64_str8(a, A64_X1, A64_X0, 0);
    } else {
        a64_ldr8(a, SCR0, A64_X1, 0);
        a64_str8(a, SCR0, A64_X0, 0);
        a64_add_imm12(a, A64_X1, A64_X1, 1, 0, 1, 0);
    }
    a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
    a64_sub_imm12(a, A64_X2, A64_X2, 1, 0, 1, 0);
    a64_b(a, Lfwd);
    if (is_memmove) {
        a64_bind(a, Lback);
        a64_add_reg(a, A64_X0, A64_X0, A64_X2, A64_LSL, 0, 1, 0);
        a64_add_reg(a, A64_X1, A64_X1, A64_X2, A64_LSL, 0, 1, 0);
        a64_bind(a, Lback_loop);
        a64_cbz(a, A64_X2, Ldone, 1);
        a64_sub_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
        a64_sub_imm12(a, A64_X1, A64_X1, 1, 0, 1, 0);
        a64_ldr8(a, SCR0, A64_X1, 0);
        a64_str8(a, SCR0, A64_X0, 0);
        a64_sub_imm12(a, A64_X2, A64_X2, 1, 0, 1, 0);
        a64_b(a, Lback_loop);
    }
    a64_bind(a, Ldone);
    a64_mov_reg(a, A64_X0, SCR1, 1);
    return 1;
}

static void emit_call(C64 *c, const IRInst *s) {
    A64Asm *a = c->as;
    int n = s->call_nargs;

    /* The raw-syscall intrinsic has its own x16/x0..x5 convention;
     * dispatch BEFORE the generic argument shuffle below, which would
     * move the number/arguments into x0..x7 and clobber source homes
     * that the syscall lowering needs to re-read. */
    if (s->call_name && strcmp(s->call_name, "__syscall") == 0) {
        emit_syscall(c, s);
        return;
    }

    int nreg = n < 8 ? n : 8;
    int nstack = n > 8 ? n - 8 : 0;

    /* Outgoing stack args live in the reserved call zone at the very
     * bottom of the frame: [sp+0 .. sp+call_area).  sp stays 16-aligned
     * throughout the function, so no per-call adjustment is needed. */
    for (int i = 0; i < nstack; i++) {
        IRValue av = s->call_args[8 + i];
        int r = load_op(c, av, -1);
        a64_str64(a, r, A64_SP, 8 * i);
    }

    /* Lift an indirect target into x17 (IP1), which no allocation owns. */
    int tgt = -1;
    if (s->call_callee >= 0) {
        int r = load_ptrv(c, s->call_callee, -1);
        a64_mov_reg(c->as, SCR1, r, 1);
        tgt = SCR1;
    }

    /* Register-argument moves with cycle breaking.
     *   kind 0: source lives in register src[i]
     *   kind 1: source is a spill slot / CONST (materialized into xi) */
    int src[8], kind[8], done[8];
    for (int i = 0; i < nreg; i++) {
        done[i] = 0;
        IRValue av = s->call_args[i];
        const IRInst *d = &c->fn->insts.data[c->def[av]];
        int r = d->op == IR_CONST ? -1 : home_reg(c, av);
        if (r >= 0) { kind[i] = 0; src[i] = r; }
        else { kind[i] = 1; src[i] = -1; }
    }

    int remaining = nreg;
    while (remaining > 0) {
        int picked = -1;
        for (int i = 0; i < nreg; i++) {
            if (done[i]) continue;
            /* Picking i writes x_i; blocked while x_i is still needed as a
             * register source (kind 0) of another pending move. */
            int blocked = 0;
            for (int j = 0; j < nreg; j++)
                if (!done[j] && j != i && kind[j] == 0 &&
                    src[j] == A64_X0 + i) { blocked = 1; break; }
            if (!blocked) { picked = i; break; }
        }
        if (picked < 0) {
            /* Pure register cycle: lift one source through x16. */
            for (int i = 0; i < nreg; i++)
                if (!done[i] && kind[i] == 0) {
                    a64_mov_reg(c->as, SCR0, src[i], 1);
                    src[i] = SCR0;
                    break;
                }
            continue;
        }
        int i = picked;
        IRValue av = s->call_args[i];
        int is64 = vw(c, av) == 8;
        if (kind[i] == 0) {
            if (src[i] != A64_X0 + i)
                a64_mov_reg(c->as, A64_X0 + i, src[i], is64);
        } else {
            const IRInst *d = &c->fn->insts.data[c->def[av]];
            if (d->op == IR_CONST) {
                emit_mov_imm_w(c->as, A64_X0 + i, d->imm, is64);
            } else {
                int so = spill_off(c, av);
                frame_load(c, A64_X0 + i, so, is64 ? 8 : 4, is64 ? 1 : 0);
            }
        }
        done[i] = 1;
        remaining--;
    }

    if (tgt >= 0) {
        a64_blr(c->as, tgt);
    } else if (!(s->call_name && emit_mem_builtin(c, s->call_name))) {
        int fi = 0;
        if (find_function(c->ir, s->call_name, &fi) != 0)
            die_at(s->loc.file ? s->loc.file : c->fn->loc.file,
                   s->loc.line, s->loc.col,
                   "arm64 backend: call to undefined function '%s'",
                   s->call_name);
        a64_bl(c->as, c->fn_label[fi]);
    }

    if (s->dst >= 0) {
        int d = dst_reg(c, s->dst);
        if (d != A64_X0)
            a64_mov_reg(c->as, d, A64_X0, s->width == 8);
        commit(c, s->dst, d);
    }
}

/* Raw Darwin syscall: number in x16, arguments in x0..x5, svc #0x80,
 * result in x0.  The IR shape mirrors the x86 backend's __syscall
 * intrinsic: call_args[0] is the number, call_args[1..6] the arguments.
 * (errno/carry handling arrives with the T13 builtin pass; this slice
 * exists so freestanding code can do write/exit without libSystem.) */
static void emit_syscall(C64 *c, const IRInst *s) {
    A64Asm *a = c->as;
    int n = s->call_nargs;
    if (n < 1) c64_die(c, s, "__syscall without number");
    int nreg = n - 1;
    if (nreg > 6) nreg = 6;

    /* Cycle-safe moves of args[1..] into x0..x5.  x17 is the only
     * scratch (x16 is reserved for the number); mirrors emit_call's
     * permutation algorithm. */
    int src[6], kind[6], done[6];
    for (int i = 0; i < nreg; i++) {
        IRValue av = s->call_args[1 + i];
        const IRInst *d = &c->fn->insts.data[c->def[av]];
        int r = d->op == IR_CONST ? -1 : home_reg(c, av);
        if (r >= 0) { kind[i] = 0; src[i] = r; }
        else { kind[i] = 1; src[i] = -1; }
        done[i] = 0;
    }
    int remaining = nreg;
    while (remaining > 0) {
        int picked = -1;
        for (int i = 0; i < nreg; i++) {
            if (done[i]) continue;
            int blocked = 0;
            for (int j = 0; j < nreg; j++)
                if (!done[j] && j != i && kind[j] == 0 &&
                    src[j] == A64_X0 + i) { blocked = 1; break; }
            if (!blocked) { picked = i; break; }
        }
        if (picked < 0) {
            for (int i = 0; i < nreg; i++)
                if (!done[i] && kind[i] == 0) {
                    a64_mov_reg(c->as, SCR1, src[i], 1);
                    src[i] = SCR1;
                    break;
                }
            continue;
        }
        int i = picked;
        IRValue av = s->call_args[1 + i];
        int is64 = vw(c, av) == 8;
        if (kind[i] == 0) {
            if (src[i] != A64_X0 + i)
                a64_mov_reg(c->as, A64_X0 + i, src[i], is64);
        } else {
            const IRInst *d = &c->fn->insts.data[c->def[av]];
            if (d->op == IR_CONST) {
                emit_mov_imm_w(c->as, A64_X0 + i, d->imm, is64);
            } else {
                int so = spill_off(c, av);
                frame_load(c, A64_X0 + i, so, is64 ? 8 : 4, is64 ? 1 : 0);
            }
        }
        done[i] = 1;
        remaining--;
    }

    /* Number last so the argument moves can use x16 as scratch freely. */
    IRValue nv = s->call_args[0];
    const IRInst *nd = &c->fn->insts.data[c->def[nv]];
    if (nd->op == IR_CONST) {
        emit_mov_imm_w(a, A64_X16, nd->imm, 1);
    } else {
        int sr = load_op(c, nv, SCR0);
        a64_mov_reg(a, A64_X16, sr, 1);
    }
    a64_svc(a, 0x80);

    if (s->dst >= 0) {
        int d = dst_reg(c, s->dst);
        if (d != A64_X0)
            a64_mov_reg(a, d, A64_X0, s->width == 8);
        commit(c, s->dst, d);
    }
}

/* ── Binary operations ────────────────────────────────────────────── */

static int cmp_cond(const IRInst *s) {
    switch (s->op) {
    case IR_EQ: return A64_EQ;
    case IR_NE: return A64_NE;
    case IR_LT: return s->is_unsigned ? A64_CC : A64_LT;
    case IR_LE: return s->is_unsigned ? A64_LS : A64_LE;
    case IR_GT: return s->is_unsigned ? A64_HI : A64_GT;
    case IR_GE: return s->is_unsigned ? A64_CS : A64_GE;
    default:    return A64_EQ;
    }
}

static void emit_binop(C64 *c, const IRInst *s) {
    A64Asm *a = c->as;
    int d = dst_reg(c, s->dst);
    int is64 = s->width == 8;

    if (s->op == IR_EQ || s->op == IR_NE || s->op == IR_LT ||
        s->op == IR_LE || s->op == IR_GT || s->op == IR_GE) {
        int ra = load_op(c, s->a, -1);
        int rb = load_op(c, s->b, ra);
        /* After CMP the operand registers are dead, so CSET may target the
         * home even if it aliases ra/rb. */
        a64_cmp_reg(a, ra, rb, vw(c, s->a) == 8);
        a64_cset(a, d, cmp_cond(s), 0);
        commit(c, s->dst, d);
        return;
    }

    int ra = load_op(c, s->a, -1);
    int rb = load_op(c, s->b, ra);
    switch (s->op) {
    case IR_ADD:
        a64_add_reg(a, d, ra, rb, A64_LSL, 0, is64, 0);
        break;
    case IR_MUL:
        a64_mul(a, d, ra, rb, is64);
        break;
    case IR_BAND:
        a64_and_reg(a, d, ra, rb, is64);
        break;
    case IR_BOR:
        a64_or_reg(a, d, ra, rb, is64);
        break;
    case IR_BXOR:
        a64_eor_reg(a, d, ra, rb, is64);
        break;
    case IR_SUB:
        a64_sub_reg(a, d, ra, rb, A64_LSL, 0, is64, 0);
        break;
    case IR_DIV:
        if (s->is_unsigned) a64_udiv(a, d, ra, rb, is64);
        else a64_sdiv(a, d, ra, rb, is64);
        break;
    case IR_MOD: {
        /* Quotient needs a register that is neither operand: both may
         * already occupy x16/x17 when spilled.  x0 is not allocatable. */
        int q = SCR1;
        if (q == ra || q == rb) q = A64_X0;
        if (s->is_unsigned) a64_udiv(a, q, ra, rb, is64);
        else a64_sdiv(a, q, ra, rb, is64);
        a64_msub(a, d, q, rb, ra, is64);  /* d = ra - q*rb */
        break;
    }
    case IR_SHL:
        a64_lslv(a, d, ra, rb, is64);
        break;
    case IR_SHR:
        if (s->is_unsigned) a64_lsrv(a, d, ra, rb, is64);
        else a64_asrv(a, d, ra, rb, is64);
        break;
    case IR_ROL: {
        /* rol(ra, rb) = ror(ra, -rb); the hardware masks the count.
         * Don't negate into ra when the spilled pair occupies both scratches. */
        int nreg = (SCR1 != ra) ? SCR1 : A64_X0;
        a64_neg(a, nreg, rb, is64);
        a64_rorv(a, d, ra, nreg, is64);
        break;
    }
    default:
        c64_die(c, s, "binary op");
        return;
    }
    commit(c, s->dst, d);
}

/* ── Per-function lowering ────────────────────────────────────────── */

static int vlabels_get(C64 *c, int id) {
    if (c->vlabels[id] < 0)
        c->vlabels[id] = a64_new_label(c->as);
    return c->vlabels[id];
}

/* A dead parameter still receives a register color (it has a def, so the
 * allocator does not treat it as unused).  That color can alias a live
 * parameter.  Copying the dead value into the shared register at entry
 * clobbers the live one.  Skip parameters nothing reads. */
static int param_is_used(const IRFunction *fn, IRValue v) {
    if (v < 0) return 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        const IRInst *s = &fn->insts.data[i];
        if (s->op == IR_PARAM) continue;
        if (s->a == v || s->b == v || s->call_callee == v) return 1;
        if (s->op == IR_CALL && s->call_args) {
            for (int k = 0; k < s->call_nargs; k++)
                if (s->call_args[k] == v) return 1;
        }
    }
    return 0;
}

/* imm12 stops at 4095.  Step by 4080 so SP stays 16-byte aligned;
 * the epilogue reloads SP from FP, so the split does not have to be one
 * instruction. */
static void sub_sp_bytes(A64Asm *a, unsigned bytes) {
    while (bytes > 4095) {
        a64_sub_imm12(a, A64_SP, A64_SP, 4080, 0, 1, 0);
        bytes -= 4080;
    }
    if (bytes)
        a64_sub_imm12(a, A64_SP, A64_SP, bytes, 0, 1, 0);
}

static void emit_function(C64 *c, int fi) {
    const IRFunction *fn = c->fn;
    A64Asm *a = c->as;
    a64_bind(a, c->fn_label[fi]);

    const RAResult *ra = (const RAResult *)fn->ra;
    c->ra = ra;
    int nv = fn->next_value_id;

    /* ---- Frame plan ---- */
    c->def = xmalloc((size_t)nv * sizeof(int));
    for (int i = 0; i < nv; i++) c->def[i] = -1;
    c->alloca_off = xmalloc((size_t)nv * sizeof(int));
    memset(c->alloca_off, 0, (size_t)nv * sizeof(int));
    c->spill_off = NULL;

    /* Pinned allocas (fp-relative, mirroring the x86 backend ordering). */
    int pinned = 0;
    for (size_t j = 0; j < fn->insts.len; j++) {
        const IRInst *s = &fn->insts.data[j];
        if (s->dst >= 0) c->def[s->dst] = (int)j;

        if (s->op == IR_ALLOCA && s->alloca_bytes > 0 && s->dst >= 0) {
            int bytes = s->alloca_bytes;
            if (bytes % 8) bytes += 8 - (bytes % 8);
            int aln = (int)s->imm >= 16 ? (int)s->imm : 8;
            if (aln < 8) aln = 8;
            pinned += bytes;
            if (pinned % aln) pinned += aln - (pinned % aln);
            c->alloca_off[s->dst] = -pinned;
        }
    }

    /* GP spill slots, placed below the pinned area. */
    int spill_area = ra ? ra->stack_size : 0;
    if (ra && ra->num_spill_slots > 0) {
        c->spill_off = xmalloc((size_t)ra->num_spill_slots * sizeof(int));
        for (int s2 = 0; s2 < ra->num_spill_slots; s2++)
            c->spill_off[s2] = -(pinned + (s2 + 1) * 8);
    }

    /* Outgoing stack-argument area (max over calls in this function). */
    int call_area = 0;
    for (size_t j = 0; j < fn->insts.len; j++) {
        const IRInst *s = &fn->insts.data[j];
        if (s->op == IR_CALL && s->call_nargs > 8) {
            int bytes = (s->call_nargs - 8) * 8;
            if (bytes > call_area) call_area = bytes;
        }
    }
    if (call_area % 16) call_area += 16 - (call_area % 16);
    c->call_area = call_area;

    int locals = pinned + spill_area + call_area;
    if (locals % 16) locals += 16 - (locals % 16);
    c->frame_locals = locals;

    /* Callee-saved registers actually used (native codes x19..x28;
     * ra->reg[] holds hardware numbers, not palette colors). */
    int cs[10], ncs = 0;
    if (ra)
        for (int v = 0; v < nv; v++)
            if (ra->reg[v] >= A64_X19 && ra->reg[v] <= A64_X28) {
                int hw = ra->reg[v];
                int present = 0;
                for (int k = 0; k < ncs; k++) if (cs[k] == hw) present = 1;
                if (!present) cs[ncs++] = hw;
            }
    /* ascending order for deterministic pairs */
    for (int i = 0; i < ncs; i++)
        for (int j = i + 1; j < ncs; j++)
            if (cs[j] < cs[i]) { int t = cs[i]; cs[i] = cs[j]; cs[j] = t; }
    int cs_pairs = (ncs + 1) / 2;
    int cs_area = cs_pairs * 16;
    int save_total = 16 + cs_area;
    c->save_total = save_total;

    /* IR label table. */
    c->vlabels = xmalloc((size_t)fn->next_label_id * sizeof(int));
    for (int i = 0; i < fn->next_label_id; i++) c->vlabels[i] = -1;

    /* ---- Prologue ---- */
    a64_stp64(a, A64_FP, A64_LR, A64_SP, -save_total, A64_PAIR_PRE);
    mov_sp_like(a, A64_FP, A64_SP);
    for (int p = 0; p < ncs / 2; p++)
        a64_stp64(a, cs[2*p], cs[2*p+1], A64_FP, 16 + 16*p, A64_PAIR_OFFSET);
    if (ncs & 1)
        a64_str64(a, cs[ncs-1], A64_FP, 16 + 16*cs_pairs - 8);
    if (locals)
        sub_sp_bytes(a, (unsigned)locals);

    /* ---- Incoming integer parameters (x0..x7, then caller stack) ---- */
    int nparams = 0;
    while (nparams < (int)fn->insts.len &&
           fn->insts.data[nparams].op == IR_PARAM)
        nparams++;

    /* Cycle-safe placement: a parameter arriving in x0..x7 may be homed in
     * x2..x7, so a naive move can clobber a not-yet-read incoming reg. */
    typedef struct { int src_reg; int src_mem;   /* -1, or fp offset */
                    int dst_reg; int dst_mem;   /* -1, or fp spill offset */
                    int is64; int done; } PMove;
    PMove *mv = xmalloc((size_t)(nparams > 0 ? nparams : 1) * sizeof(PMove));
    int gp_idx = 0, stack_arg_idx = 0;
    for (int p = 0; p < nparams; p++) {
        const IRInst *s = &fn->insts.data[p];
        if (s->is_float || s->force_stack)
            c64_die(c, s, "float/aggregate parameter");
        if (gp_idx < 8) {
            mv[p].src_reg = A64_X0 + gp_idx++;
            mv[p].src_mem = -1;
        } else {
            /* Above the saved frame (Darwin ABI: no pushed return addr). */
            mv[p].src_reg = -1;
            mv[p].src_mem = save_total + 8 * stack_arg_idx++;
        }
        int hr = home_reg(c, s->dst);
        if (hr >= 0) { mv[p].dst_reg = hr; mv[p].dst_mem = -1; }
        else { mv[p].dst_reg = -1; mv[p].dst_mem = spill_off(c, s->dst); }
        mv[p].is64 = s->width == 8;
        if (!param_is_used(fn, s->dst))
            mv[p].done = 1;
        else
            mv[p].done = (mv[p].src_reg >= 0 && mv[p].dst_reg == mv[p].src_reg);
    }
    int remaining = 0;
    for (int p = 0; p < nparams; p++) remaining += !mv[p].done;
#define PM_EMIT(p) do {                                                      \
        PMove *_m = &mv[(p)];                                                \
        if (_m->dst_reg >= 0) {                                              \
            if (_m->src_reg >= 0) {                                          \
                if (_m->src_reg != _m->dst_reg)                              \
                    a64_mov_reg(a, _m->dst_reg, _m->src_reg, _m->is64);      \
            } else {                                                         \
                frame_load(c, SCR1, _m->src_mem, 8, 1);                      \
                a64_mov_reg(a, _m->dst_reg, SCR1, _m->is64);                 \
            }                                                                \
        } else {                                                             \
            if (_m->src_reg >= 0)                                            \
                a64_mov_reg(a, SCR1, _m->src_reg, _m->is64);                 \
            else                                                             \
                frame_load(c, SCR1, _m->src_mem, 8, 1);                      \
            frame_store(c, SCR1, _m->dst_mem, 8);                            \
        }                                                                    \
        _m->done = 1; remaining--;                                           \
    } while (0)

    while (remaining > 0) {
        int picked = -1;
        for (int p = 0; p < nparams; p++) {
            if (mv[p].done) continue;
            int blocked = 0;
            for (int q = 0; q < nparams; q++) {
                if (mv[q].done || q == p) continue;
                /* Emitting p overwrites Wp; blocked while some pending q
                 * still needs the incoming value held in that register. */
                if (mv[p].dst_reg >= 0 && mv[q].src_reg >= 0 &&
                    mv[q].src_reg != mv[q].dst_reg &&
                    mv[p].dst_reg == mv[q].src_reg) { blocked = 1; break; }
            }
            if (!blocked) { picked = p; break; }
        }
        if (picked >= 0) { PM_EMIT(picked); continue; }

        /* No safe move: the pending reg->reg items form one or more pure
         * permutation cycles over x0..x7.  Resolve one cycle: save its
         * first source in x16, then walk destinations backwards, closing
         * with the saved value. */
        int i0 = -1;
        for (int p = 0; p < nparams; p++)
            if (!mv[p].done && mv[p].src_reg >= 0 && mv[p].dst_reg >= 0) {
                i0 = p; break;
            }
        if (i0 < 0) {
            c64_die(c, &fn->insts.data[0], "parameter shuffle");
            break;
        }
        /* Cycles may mix 32/64-bit params; move whole registers so a
         * 64-bit value cannot lose its upper half to a w-form move. */
        mv[i0].is64 = 1;
        a64_mov_reg(a, SCR0, mv[i0].src_reg, 1);
        int cur = mv[i0].src_reg;
        for (;;) {
            int q = -1;
            for (int p = 0; p < nparams; p++)
                if (!mv[p].done && mv[p].dst_reg >= 0 &&
                    mv[p].dst_reg != mv[p].src_reg &&
                    mv[p].dst_reg == cur) { q = p; break; }
            if (q < 0) {
                c64_die(c, &fn->insts.data[0], "parameter cycle");
                remaining = 0;
                break;
            }
            if (q == i0) {
                a64_mov_reg(a, mv[i0].dst_reg, SCR0, 1);
                mv[i0].done = 1; remaining--;
                break;
            }
            cur = mv[q].src_reg;
            mv[q].is64 = 1;
            PM_EMIT(q);
        }
    }
    free(mv);
#undef PM_EMIT

    /* ---- Body ---- */
    int epilog = a64_new_label(a);
    for (size_t j = nparams; j < fn->insts.len; j++) {
        const IRInst *s = &fn->insts.data[j];
        switch (s->op) {
        case IR_CONST: {
            int d = dst_reg(c, s->dst);
            emit_mov_imm_w(a, d, s->imm, s->width == 8);
            commit(c, s->dst, d);
            break;
        }
        case IR_COPY:
        case IR_TRUNC: {
            int ra = load_op(c, s->a, -1);
            int d = dst_reg(c, s->dst);
            if (d != ra) a64_mov_reg(a, d, ra, s->width == 8);
            commit(c, s->dst, d);
            break;
        }
        case IR_SEXT: {
            int ra = load_op(c, s->a, -1);
            int d = dst_reg(c, s->dst);
            int fromw = vw(c, s->a);
            if (fromw >= (int)s->width)
                a64_mov_reg(a, d, ra, s->width == 8);
            else
                a64_sxt(a, d, ra, (unsigned)fromw, s->width == 8);
            commit(c, s->dst, d);
            break;
        }
        case IR_ZEXT: {
            int ra = load_op(c, s->a, -1);
            int d = dst_reg(c, s->dst);
            int fromw = vw(c, s->a);
            if (fromw >= (int)s->width)
                a64_mov_reg(a, d, ra, s->width == 8);
            else
                a64_uxt(a, d, ra, (unsigned)fromw, s->width == 8);
            commit(c, s->dst, d);
            break;
        }
        case IR_NEG: {
            int ra = load_op(c, s->a, -1);
            int d = dst_reg(c, s->dst);
            a64_neg(a, d, ra, s->width == 8);
            commit(c, s->dst, d);
            break;
        }
        case IR_BNOT: {
            int ra = load_op(c, s->a, -1);
            int d = dst_reg(c, s->dst);
            a64_mvn(a, d, ra, s->width == 8);
            commit(c, s->dst, d);
            break;
        }
        case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
        case IR_BAND: case IR_BOR: case IR_BXOR: case IR_SHL: case IR_SHR:
        case IR_ROL:
        case IR_EQ: case IR_NE: case IR_LT: case IR_LE: case IR_GT:
        case IR_GE:
            emit_binop(c, s);
            break;

        case IR_ALLOCA:
            break;  /* slot assigned in the frame plan */
        case IR_ADDR: {
            /* &alloca(alloca id in a) */
            int d = dst_reg(c, s->dst);
            int off = (s->a >= 0 && s->a < nv) ? c->alloca_off[s->a] : 0;
            emit_fp_addr(c, d, off);
            commit(c, s->dst, d);
            break;
        }
        case IR_STORE_PTR: {
            int p = load_ptrv(c, s->a, -1);
            int v = load_op(c, s->b, p);
            emit_store(c, v, p, s->width);
            break;
        }
        case IR_LOAD_PTR: {
            int p = load_ptrv(c, s->a, -1);
            int d = dst_reg(c, s->dst);
            emit_load(c, d, p, s->width, s->is_unsigned);
            commit(c, s->dst, d);
            break;
        }

        case IR_LABEL:
            a64_bind(a, vlabels_get(c, (int)s->imm));
            break;
        case IR_BR:
            a64_b(a, vlabels_get(c, (int)s->imm));
            break;
        case IR_CBR: {
            int r = load_op(c, s->a, -1);
            a64_cbz(a, r, vlabels_get(c, (int)s->b), 1);
            a64_b(a, vlabels_get(c, (int)s->imm));
            break;
        }
        case IR_JMP_PTR: {
            int r = load_ptrv(c, s->a, -1);
            a64_br_reg(a, r);
            break;
        }
        case IR_LADDR: {
            int d = dst_reg(c, s->dst);
            a64_adrp_add_label(a, d, vlabels_get(c, (int)s->imm));
            commit(c, s->dst, d);
            break;
        }

        case IR_CALL:
            emit_call(c, s);
            break;

        case IR_RETURN: {
            if (s->a >= 0) {
                int r = load_op(c, s->a, -1);
                if (r != A64_X0)
                    a64_mov_reg(a, A64_X0, r, fn->ret_width == 8);
            }
            a64_b(a, epilog);
            break;
        }

        case IR_FRAME_ADDR: {
            int d = dst_reg(c, s->dst);
            a64_mov_reg(a, d, A64_FP, 1);
            commit(c, s->dst, d);
            break;
        }
        case IR_RETURN_ADDR: {
            int d = dst_reg(c, s->dst);
            a64_ldr64(a, d, A64_FP, 8);
            commit(c, s->dst, d);
            break;
        }
        case IR_STACK_SAVE: {
            int d = dst_reg(c, s->dst);
            mov_sp_like(a, d, A64_SP);
            commit(c, s->dst, d);
            break;
        }
        case IR_STACK_RESTORE: {
            int r = load_op(c, s->a, -1);
            mov_sp_like(a, A64_SP, r);
            break;
        }
        case IR_DYN_ALLOCA: {
            int r = load_op(c, s->a, -1);
            /* round size up to 16, then bump sp via a GP staging reg
             * (SUB shifted-register cannot address SP). */
            a64_add_imm12(a, SCR1, r, 15, 0, 1, 0);
            if (a64_and_imm(a, SCR1, SCR1, ~(uint64_t)15, 1) != 0)
                a64_mov_reg(a, SCR1, r, 1);
            mov_sp_like(a, SCR0, A64_SP);
            a64_sub_reg(a, SCR0, SCR0, SCR1, A64_LSL, 0, 1, 0);
            mov_sp_like(a, A64_SP, SCR0);
            int d = dst_reg(c, s->dst);
            a64_mov_reg(a, d, SCR0, 1);
            commit(c, s->dst, d);
            break;
        }

        case IR_DBG_VALUE:
            break;

        case IR_LOAD: {
            /* At -O0 a is a pinned-alloca slot (ternary/short-circuit
             * temps).  The unpinned SSA form (-O1) is a plain copy. */
            int off = (s->a >= 0 && s->a < nv) ? c->alloca_off[s->a] : 0;
            int d = dst_reg(c, s->dst);
            if (off != 0) {
                frame_load(c, d, off, s->width, s->is_unsigned);
            } else {
                int ra = load_op(c, s->a, -1);
                if (d != ra) a64_mov_reg(a, d, ra, s->width == 8);
            }
            commit(c, s->dst, d);
            break;
        }
        case IR_STORE: {
            int off = (s->a >= 0 && s->a < nv) ? c->alloca_off[s->a] : 0;
            int v = load_op(c, s->b, -1);
            if (off != 0) {
                frame_store(c, v, off, s->width);
            } else {
                /* -O1 SSA form: make value a's home hold b. */
                int d = dst_reg(c, s->a);
                if (d != v) a64_mov_reg(a, d, v, s->width == 8);
                commit(c, s->a, d);
            }
            break;
        }
        case IR_GADDR: {
            int gi = find_global_idx(c->ir, s->call_name);
            if (gi < 0) c64_die(c, s, "external global variable");
            int d = dst_reg(c, s->dst);
            /* adrp d, page ; add d, d, #pageoff — both words patched in
             * codegen64 once __const/__data/__bss placement is final. */
            if (c->ngfix == c->capgfix) {
                c->capgfix = c->capgfix ? c->capgfix * 2 : 64;
                c->gfix = realloc(c->gfix, c->capgfix * sizeof *c->gfix);
                if (!c->gfix) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
            }
            c->gfix[c->ngfix].at = (uint32_t)a->code.len;
            c->gfix[c->ngfix].gidx = gi;
            c->ngfix++;
            a64_word(a, 0x90000000u | (uint32_t)(d & 31));
            a64_word(a, 0x91000000u | ((uint32_t)(d & 31) << 5)
                                    | (uint32_t)(d & 31));
            commit(c, s->dst, d);
            break;
        }
        case IR_GADDR_TLS:
            c64_die(c, s, "thread-local variable");
            break;
        case IR_FADDR: {
            int fi = 0;
            if (find_function(c->ir, s->call_name, &fi) != 0)
                c64_die(c, s, "external function address");
            int d = dst_reg(c, s->dst);
            a64_adrp_add_label(a, d, c->fn_label[fi]);
            commit(c, s->dst, d);
            break;
        }
        default:
            if (s->is_float) c64_die(c, s, "floating point");
            c64_die(c, s, "IR op");
            break;
        }
    }

    /* ---- Epilogue ---- */
    a64_bind(a, epilog);
    mov_sp_like(a, A64_SP, A64_FP);
    for (int p = 0; p < ncs / 2; p++)
        a64_ldp64(a, cs[2*p], cs[2*p+1], A64_FP, 16 + 16*p, A64_PAIR_OFFSET);
    if (ncs & 1)
        a64_ldr64(a, cs[ncs-1], A64_FP, 16 + 16*cs_pairs - 8);
    a64_ldp64(a, A64_FP, A64_LR, A64_SP, save_total, A64_PAIR_POST);
    a64_ret(a, A64_LR);

    free(c->def);
    free(c->alloca_off);
    free(c->spill_off);
    free(c->vlabels);
    c->ra = NULL;
}

/* ── Module entry point ──────────────────────────────────────────── */

void codegen64(const IRModule *ir, EmitModule *out, int want_debug) {
    (void)want_debug;  /* DWARF emission lands in T20. */

    int main_id = -1;
    if (find_function(ir, "main", &main_id) != 0)
        die_at("<arm64>", 0, 0, "no 'main' function found");

    A64Asm a;
    a64_init(&a);

    C64 c;
    memset(&c, 0, sizeof c);
    c.as = &a;
    c.ir = ir;

    c.fn_label = xmalloc(ir->functions.len * sizeof(int));
    for (size_t i = 0; i < ir->functions.len; i++)
        c.fn_label[i] = a64_new_label(&a);

    /* Lay globals out into __const / __data / __bss (same placement rules
     * as the x86 path).  Their image addresses depend on the final text
     * length, so ADRP+ADD references are patched after resolve. */
    if (ir->globals.len) {
        c.gsect = xmalloc(ir->globals.len * sizeof(int));
        c.goff = xmalloc(ir->globals.len * sizeof(size_t));
    }
    for (size_t gi = 0; gi < ir->globals.len; gi++) {
        const IRGlobal *g = &ir->globals.data[gi];
        size_t al = g->align > 0 ? (size_t)g->align : 8;
        if (g->is_tls)
            die_at(g->loc.file ? g->loc.file : "<arm64>", g->loc.line, 0,
                   "arm64 backend: thread-local variable not supported yet");
        /* A readonly global that contains a pointer must live in __DATA:
         * dyld chained fixups are applied to writable pages, and
         * __TEXT,__const is mapped read-only/execute. */
        if (g->is_readonly && !g->num_fixups) {
            while (out->rodata.len % al) { char z = 0; buffer_append(&out->rodata, &z, 1); }
            c.gsect[gi] = G_RO;
            c.goff[gi] = out->rodata.len;
            if (g->init_bytes) {
                buffer_append(&out->rodata, g->init_bytes, g->size);
            } else {
                for (int k = 0; k < g->size; k++) { char z = 0; buffer_append(&out->rodata, &z, 1); }
            }
            if (al > out->rodata_align) out->rodata_align = al;
        } else if (g->init_bytes || g->num_fixups) {
            while (out->data.len % al) { char z = 0; buffer_append(&out->data, &z, 1); }
            c.gsect[gi] = G_DATA;
            c.goff[gi] = out->data.len;
            if (g->init_bytes) {
                buffer_append(&out->data, g->init_bytes, g->size);
            } else {
                for (int k = 0; k < g->size; k++) { char z = 0; buffer_append(&out->data, &z, 1); }
            }
            if (al > out->data_align) out->data_align = al;
            for (int fi = 0; fi < g->num_fixups; fi++) {
                if (c.npfix == c.cappfix) {
                    c.cappfix = c.cappfix ? c.cappfix * 2 : 16;
                    c.pfix = xrealloc(c.pfix, c.cappfix * sizeof *c.pfix);
                }
                c.pfix[c.npfix].gidx = (int)gi;
                c.pfix[c.npfix].slot_off = g->fixups[fi].offset;
                c.pfix[c.npfix].sym = g->fixups[fi].sym;
                c.pfix[c.npfix].addend = g->fixups[fi].addend;
                c.npfix++;
            }
        } else {
            while (out->bss_size % al) out->bss_size++;
            c.gsect[gi] = G_BSS;
            c.goff[gi] = out->bss_size;
            out->bss_size += g->size;
            if (al > out->bss_align) out->bss_align = al;
        }
    }

    /* Constructor/destructor order (matches the ELF _start walk in
     * link.c): constructors ascending by priority, source order as the
     * stable tiebreak; destructors are the ascending list walked
     * backwards.  Single-module stage: calls are direct BLs to local
     * function labels.  T14/T15 replaces this with a merged pointer
     * table once multi-module linking lands. */
    int *ctors = xmalloc(ir->functions.len * sizeof(int));
    int *dtors = xmalloc(ir->functions.len * sizeof(int));
    int nctors = 0, ndtors = 0;
    for (size_t i = 0; i < ir->functions.len; i++) {
        const IRFunction *fn = &ir->functions.data[i];
        if (fn->is_constructor) {
            int prio = fn->ctor_prio ? fn->ctor_prio : INIT_PRIO_DEFAULT;
            int pos = nctors++;
            while (pos > 0) {
                const IRFunction *prev = &ir->functions.data[ctors[pos - 1]];
                int pp = prev->ctor_prio ? prev->ctor_prio : INIT_PRIO_DEFAULT;
                if (pp <= prio) break;
                ctors[pos] = ctors[pos - 1];
                pos--;
            }
            ctors[pos] = (int)i;
        }
        if (fn->is_destructor) {
            int prio = fn->dtor_prio ? fn->dtor_prio : INIT_PRIO_DEFAULT;
            int pos = ndtors++;
            while (pos > 0) {
                const IRFunction *prev = &ir->functions.data[dtors[pos - 1]];
                int pp = prev->dtor_prio ? prev->dtor_prio : INIT_PRIO_DEFAULT;
                if (pp <= prio) break;
                dtors[pos] = dtors[pos - 1];
                pos--;
            }
            dtors[pos] = (int)i;
        }
    }

    /* LC_MAIN entry stub.  dyld hands us x0=argc, x1=argv, x2=envp; keep
     * them in callee-saved registers across the constructor calls, call
     * main, run destructors in reverse, then Darwin exit(main's value).
     * The stub owns x19..x22 outright (process entry has no caller whose
     * values matter) but saves them anyway to keep the frame ABI-clean. */
    a64_stp64(&a, A64_FP, A64_LR, A64_SP, -48, A64_PAIR_PRE);
    mov_sp_like(&a, A64_FP, A64_SP);   /* ADD, not ORR (x31==SP vs XZR) */
    a64_stp64(&a, A64_X19, A64_X20, A64_FP, 16, A64_PAIR_OFFSET);
    a64_stp64(&a, A64_X21, A64_X22, A64_FP, 32, A64_PAIR_OFFSET);
    a64_mov_reg(&a, A64_X19, A64_X0, 1);
    a64_mov_reg(&a, A64_X20, A64_X1, 1);
    a64_mov_reg(&a, A64_X21, A64_X2, 1);
    for (int i = 0; i < nctors; i++)
        a64_bl(&a, c.fn_label[ctors[i]]);
    a64_mov_reg(&a, A64_X0, A64_X19, 1);
    a64_mov_reg(&a, A64_X1, A64_X20, 1);
    a64_mov_reg(&a, A64_X2, A64_X21, 1);
    a64_bl(&a, c.fn_label[main_id]);
    a64_mov_reg(&a, A64_X22, A64_X0, 1);          /* save main's result */
    for (int i = ndtors - 1; i >= 0; i--)
        a64_bl(&a, c.fn_label[dtors[i]]);
    a64_mov_reg(&a, A64_X0, A64_X22, 1);
    a64_movz(&a, A64_X16, 1, 0, 1);              /* exit */
    a64_svc(&a, 0x80);
    /* Unreachable, but keep a valid epilogue for disassembly/tools. */
    a64_ldp64(&a, A64_X21, A64_X22, A64_FP, 32, A64_PAIR_OFFSET);
    a64_ldp64(&a, A64_X19, A64_X20, A64_FP, 16, A64_PAIR_OFFSET);
    a64_ldp64(&a, A64_FP, A64_LR, A64_SP, 48, A64_PAIR_POST);
    a64_ret(&a, A64_LR);
    free(ctors);
    free(dtors);

    for (size_t i = 0; i < ir->functions.len; i++) {
        c.fn = &ir->functions.data[i];
        emit_function(&c, (int)i);
    }

    /* The code buffer lands after the Mach-O header pad; ADRP page fixups
     * must compute against the runtime file/VA offset. */
    a64_set_base(&a, macho_text_offset());
    if (a64_resolve(&a) != 0) {
        a64_free(&a);
        free(c.fn_label);
        die_at("<arm64>", 0, 0, "arm64 label resolution failed");
    }

    out->text.data = a.code.data;
    out->text.len = a.code.len;
    out->text.cap = a.code.cap;
    out->text_align = 4;
    memset(&a.code, 0, sizeof(a.code));

    /* Patch ADRP+ADD pairs and record dyld rebases for pointer
     * initializers now that text length (and so section placement) is
     * known. */
    if (c.ngfix || c.npfix) {
        uint64_t ro_off, data_off, bss_off;
        macho_section_offsets(out, out->text.len, &ro_off, &data_off, &bss_off);
        for (size_t i = 0; i < c.ngfix; i++) {
            int gi = c.gfix[i].gidx;
            uint64_t base = c.gsect[gi] == G_RO ? ro_off
                          : c.gsect[gi] == G_DATA ? data_off : bss_off;
            uint64_t tgt = base + c.goff[gi];
            uint64_t pc = (uint64_t)macho_text_offset() + c.gfix[i].at;
            int64_t pages = (int64_t)((tgt & ~(uint64_t)0xFFF)
                                      - (pc & ~(uint64_t)0xFFF)) >> 12;
            uint32_t w0, w1;
            memcpy(&w0, out->text.data + c.gfix[i].at, 4);
            memcpy(&w1, out->text.data + c.gfix[i].at + 4, 4);
            w0 |= (uint32_t)((pages & 3) << 29)
                | (uint32_t)(((pages >> 2) & 0x7FFFF) << 5);
            w1 |= (uint32_t)(tgt & 0xFFF) << 10;
            memcpy(out->text.data + c.gfix[i].at, &w0, 4);
            memcpy(out->text.data + c.gfix[i].at + 4, &w1, 4);
        }
        for (size_t i = 0; i < c.npfix; i++) {
            int gi = c.pfix[i].gidx;
            const IRGlobal *g = &ir->globals.data[gi];
            uint64_t gbase = c.gsect[gi] == G_RO ? ro_off
                           : c.gsect[gi] == G_DATA ? data_off : bss_off;
            uint64_t slot = gbase + c.goff[gi] + (uint64_t)c.pfix[i].slot_off;
            int tgi = find_global_idx(ir, c.pfix[i].sym);
            int64_t tgt;
            if (tgi >= 0) {
                uint64_t tb = c.gsect[tgi] == G_RO ? ro_off
                            : c.gsect[tgi] == G_DATA ? data_off : bss_off;
                tgt = (int64_t)(tb + c.goff[tgi]) + c.pfix[i].addend;
            } else {
                int fi = 0;
                if (find_function(ir, c.pfix[i].sym, &fi) != 0 ||
                    !a.labels[c.fn_label[fi]].bound) {
                    die_at(g->loc.file ? g->loc.file : "<arm64>",
                           g->loc.line, 0,
                           "arm64 backend: external symbol '%s' in "
                           "global initializer", c.pfix[i].sym);
                }
                tgt = (int64_t)macho_text_offset()
                    + (int64_t)a.labels[c.fn_label[fi]].pos
                    + c.pfix[i].addend;
            }
            if (tgt < 0 || gbase == 0)
                die_at(g->loc.file ? g->loc.file : "<arm64>", g->loc.line, 0,
                       "arm64 backend: bad pointer initializer");
            emit_module_add_rebase(out, slot, (uint64_t)tgt);
        }
    }
    free(c.gfix);
    free(c.pfix);
    free(c.gsect);
    free(c.goff);
    a64_free(&a);
    free(c.fn_label);
}
