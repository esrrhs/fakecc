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
    const RAResult   *ra_fp;     /* SIMD homes for scalar float/double */
    int              *def;         /* SSA value -> defining inst index  */
    int              *alloca_off;  /* ALLOCA SSA id -> fp-relative off  */
    int              *spill_off;   /* spill slot -> fp-relative offset  */
    int              *fp_spill_off; /* SIMD spill slot -> fp-relative    */
    int              *vlabels;     /* IR label id -> a64 label id       */
    int              *fn_label;    /* function index -> a64 label id    */
    int               frame_locals;
    int               call_area;
    int               save_total;  /* fp/lr + callee-saved saves bytes  */
    int               vararg_off;  /* fp offset of the first anonymous arg */
    /* Module globals: section (G_RO/G_DATA/G_BSS) and in-section offset. */
    int              *gsect;
    size_t           *goff;
    /* ADRP+ADD pairs targeting globals; patched once the final section
     * placement (which depends on the total text length) is known. */
    struct GFix { uint32_t at; int gidx; const char *name; } *gfix;
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
    /* Local copy of runtime/int128.c's restoring division.  The arm64
     * image does not link the Linux runtime, and external BL is T14. */
    int               udiv_label;
    /* Object-file calls to functions that are not in this TU. */
    struct XCall { uint32_t at; const char *name; } *xcall;
    size_t            nxcall, capxcall;
    /* &&label sites.  Object files cannot bake the address: the linker
     * prepends an entry stub, which changes the page offset. */
    struct LRel { int a64lab; char *name; } *lrel;
    size_t            nlrel, caplrel;
} C64;

enum { G_RO = 1, G_DATA = 2, G_BSS = 3, G_COMMON = 4 };

static const char *note_label_sym(C64 *c, int ir_id, int a64lab) {
    const char *fn = (c->fn && c->fn->name) ? c->fn->name : "fn";
    size_t n = strlen(fn) + 24;
    char *buf = xmalloc(n);
    snprintf(buf, n, ".L%s_%d", fn, ir_id);
    for (size_t i = 0; i < c->nlrel; i++) {
        if (strcmp(c->lrel[i].name, buf) == 0) {
            free(buf);
            return c->lrel[i].name;
        }
    }
    if (c->nlrel == c->caplrel) {
        c->caplrel = c->caplrel ? c->caplrel * 2 : 8;
        c->lrel = realloc(c->lrel, c->caplrel * sizeof *c->lrel);
        if (!c->lrel) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    }
    c->lrel[c->nlrel].a64lab = a64lab;
    c->lrel[c->nlrel].name = buf;
    c->nlrel++;
    return buf;
}

static void note_page_reloc(C64 *c, uint32_t at, int gidx, const char *name) {
    if (c->ngfix == c->capgfix) {
        c->capgfix = c->capgfix ? c->capgfix * 2 : 64;
        c->gfix = realloc(c->gfix, c->capgfix * sizeof *c->gfix);
        if (!c->gfix) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    }
    c->gfix[c->ngfix].at = at;
    c->gfix[c->ngfix].gidx = gidx;
    c->gfix[c->ngfix].name = name;
    c->ngfix++;
}

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
static void emit_fp_addr(C64 *c, int rd, int off);
static void emit_mov_imm_w(A64Asm *a, int rd, int64_t imm, int is64);
static void fmov_q(A64Asm *a, int rd, int rn);
static void str_q(A64Asm *a, int rt, int rn, int byte_off);
static void ldr_q(A64Asm *a, int rt, int rn, int byte_off);

/* Destination register for v: its home, or SCR0 when spilled. */
static int dst_reg(C64 *c, IRValue v) {
    int r = home_reg(c, v);
    return r >= 0 ? r : SCR0;
}

static void commit(C64 *c, IRValue v, int srcreg) {
    if (home_reg(c, v) >= 0) return;
    frame_store(c, srcreg, spill_off(c, v), 8);
}

/* IEEE scalar float/double (value_is_float == 1, width 4 or 8).
 * long double, decimal and NEON vectors stay out of this path. */
static int scalar_fp_val(const C64 *c, IRValue v) {
    const IRFunction *fn = c->fn;
    if (!fn || v < 0 || !fn->value_is_float || v >= fn->value_meta_cap)
        return 0;
    if (fn->value_is_float[v] != 1) return 0;
    int w = (fn->value_width && v < fn->next_value_id && fn->value_width[v])
            ? fn->value_width[v] : 8;
    return w == 4 || w == 8;
}

/* Darwin vector_size(16): one Q register (value_is_float == 4, width 16). */
static int vec16_val(const C64 *c, IRValue v) {
    const IRFunction *fn = c->fn;
    if (!fn || v < 0 || !fn->value_is_float || v >= fn->value_meta_cap)
        return 0;
    if (fn->value_is_float[v] != 4) return 0;
    int w = (fn->value_width && v < fn->next_value_id) ? fn->value_width[v] : 0;
    return w == 16;
}

static int fp_home(const C64 *c, IRValue v) {
    if (!c->ra_fp || v < 0 || v >= c->ra_fp->num_values) return -1;
    int hw = c->ra_fp->reg[v];
    return hw >= 0 ? hw : -1;
}

static int fp_spill(const C64 *c, IRValue v) {
    if (!c->ra_fp || v < 0 || v >= c->ra_fp->num_values || c->ra_fp->reg[v] >= 0)
        return 0;
    return c->fp_spill_off[c->ra_fp->spill_slot[v]];
}

/* v30/v31 are outside the allocatable set. */
#define FSCR A64_V31

static void fp_frame(C64 *c, int vt, int off, int is_double, int store) {
    emit_fp_addr(c, SCR1, off);
    if (store) {
        if (is_double) a64_str_d(c->as, vt, SCR1, 0);
        else a64_str_s(c->as, vt, SCR1, 0);
    } else {
        if (is_double) a64_ldr_d(c->as, vt, SCR1, 0);
        else a64_ldr_s(c->as, vt, SCR1, 0);
    }
}

static void commit_fp(C64 *c, IRValue v, int src) {
    int h = fp_home(c, v);
    if (h >= 0) {
        if (h != src) a64_fmov_reg(c->as, h, src, vw(c, v) == 8);
        return;
    }
    fp_frame(c, src, fp_spill(c, v), vw(c, v) == 8, 1);
}

/* Materialize v into a V register.  `avoid` is a V reg that must stay intact. */
static int load_fp(C64 *c, IRValue v, int avoid) {
    int h = fp_home(c, v);
    if (h >= 0) return h;
    int tmp = (avoid == FSCR) ? A64_V30 : FSCR;
    const IRInst *d = (v >= 0 && c->def && c->def[v] >= 0)
                      ? &c->fn->insts.data[c->def[v]] : NULL;
    if (d && d->op == IR_CONST) {
        int isd = vw(c, v) == 8;
        emit_mov_imm_w(c->as, SCR0, d->float_imm, isd);
        a64_fmov_gp(c->as, tmp, SCR0, 1, isd);
        return tmp;
    }
    fp_frame(c, tmp, fp_spill(c, v), vw(c, v) == 8, 0);
    return tmp;
}

static void commit_q(C64 *c, IRValue v, int src) {
    int h = fp_home(c, v);
    if (h >= 0) {
        fmov_q(c->as, h, src);
        return;
    }
    emit_fp_addr(c, SCR1, fp_spill(c, v));
    str_q(c->as, src, SCR1, 0);
}

static int load_q(C64 *c, IRValue v, int avoid) {
    int h = fp_home(c, v);
    if (h >= 0) return h;
    int tmp = (avoid == FSCR) ? A64_V30 : FSCR;
    emit_fp_addr(c, SCR1, fp_spill(c, v));
    ldr_q(c->as, tmp, SCR1, 0);
    return tmp;
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

/* Defining instruction, or NULL when v was never written (a -1 slot
 * in the def map).  Indexing that slot used to walk off the front of
 * the instruction array. */
static const IRInst *def_inst(C64 *c, IRValue v) {
    if (!c->def || v < 0 || v >= c->fn->next_value_id) return NULL;
    int di = c->def[v];
    if (di < 0 || (size_t)di >= c->fn->insts.len) return NULL;
    return &c->fn->insts.data[di];
}

/* Load operand v into a register; `other` is the register already
 * holding the other operand (scratch choice must not collide). */
static int load_op(C64 *c, IRValue v, int other) {
    int scratch = (other == SCR0) ? SCR1 : SCR0;
    const IRInst *d = def_inst(c, v);
    if (d && d->op == IR_CONST) {
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
    int is_bzero = strcmp(name, "bzero") == 0;
    int is_mempcpy = strcmp(name, "mempcpy") == 0;
    int is_bcopy = strcmp(name, "bcopy") == 0;
    if (!is_memcpy && !is_memmove && !is_memset && !is_bzero && !is_mempcpy
        && !is_bcopy)
        return 0;
    int defined = 0;
    if (find_function(c->ir, name, &defined) == 0) return 0;

    A64Asm *a = c->as;
    if (is_bcopy) {
        /* bcopy(src, dst, n): opposite of memmove, and overlap-safe. */
        a64_mov_reg(a, SCR0, A64_X0, 1);
        a64_mov_reg(a, A64_X0, A64_X1, 1);
        a64_mov_reg(a, A64_X1, SCR0, 1);
        is_memmove = 1;
    }
    if (is_bzero) {
        /* bzero(dst, n): the count arrives in x1, the fill byte is 0. */
        a64_mov_reg(a, A64_X2, A64_X1, 1);
        a64_movz(a, A64_X1, 0, 0, 1);
        is_memset = 1;
    }
    if (is_mempcpy) is_memcpy = 1;
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
    /* mempcpy returns dest+n, which is where the loop stopped. */
    if (!is_mempcpy)
        a64_mov_reg(a, A64_X0, SCR1, 1);
    return 1;
}

/* memcmp/strcmp/strlen/strncmp.  Same reason as memcpy: the Darwin
 * runtime is not linked yet, and a user definition still wins.
 * Arguments are already in x0/x1(/x2).  The signed byte difference
 * (or the length) is left in x0.  x3/x4 are caller-saved. */
static int emit_scan_builtin(C64 *c, const char *name) {
    int is_memcmp = strcmp(name, "memcmp") == 0 || strcmp(name, "bcmp") == 0;
    int is_strcmp = strcmp(name, "strcmp") == 0;
    int is_strncmp = strcmp(name, "strncmp") == 0;
    int is_strlen = strcmp(name, "strlen") == 0;
    if (!is_memcmp && !is_strcmp && !is_strncmp && !is_strlen) return 0;
    int defined = 0;
    if (find_function(c->ir, name, &defined) == 0) return 0;

    A64Asm *a = c->as;
    int Lloop = a64_new_label(a);
    int Ldiff = a64_new_label(a);
    int Lzero = a64_new_label(a);
    int Ldone = a64_new_label(a);

    if (is_strlen) {
        a64_mov_reg(a, A64_X1, A64_X0, 1);
        a64_bind(a, Lloop);
        a64_ldr8(a, A64_X2, A64_X0, 0);
        a64_cbz(a, A64_X2, Ldone, 0);
        a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
        a64_b(a, Lloop);
        a64_bind(a, Ldone);
        a64_sub_reg(a, A64_X0, A64_X0, A64_X1, A64_LSL, 0, 1, 0);
        return 1;
    }

    a64_bind(a, Lloop);
    if (is_memcmp || is_strncmp)
        a64_cbz(a, A64_X2, Lzero, 1);
    a64_ldr8(a, A64_X3, A64_X0, 0);
    a64_ldr8(a, A64_X4, A64_X1, 0);
    a64_cmp_reg(a, A64_X3, A64_X4, 0);
    a64_bcond(a, A64_NE, Ldiff);
    if (is_strcmp || is_strncmp)
        a64_cbz(a, A64_X3, Lzero, 0);
    a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
    a64_add_imm12(a, A64_X1, A64_X1, 1, 0, 1, 0);
    if (is_memcmp || is_strncmp)
        a64_sub_imm12(a, A64_X2, A64_X2, 1, 0, 1, 0);
    a64_b(a, Lloop);
    a64_bind(a, Lzero);
    a64_movz(a, A64_X0, 0, 0, 1);
    a64_b(a, Ldone);
    a64_bind(a, Ldiff);
    /* Unsigned bytes.  Sign-extend so a 64-bit compare still sees < 0. */
    a64_sub_reg(a, A64_X0, A64_X3, A64_X4, A64_LSL, 0, 0, 0);
    a64_sxt(a, A64_X0, A64_X0, 4, 1);
    a64_bind(a, Ldone);
    return 1;
}

/* memchr/strchr/strrchr/strnlen.  Pointer or length left in x0.
 * The searched byte is masked to 8 bits.  x3/x4 are caller-saved. */
static int emit_find_builtin(C64 *c, const char *name) {
    int is_memchr = strcmp(name, "memchr") == 0;
    int is_strchr = strcmp(name, "strchr") == 0 || strcmp(name, "index") == 0;
    int is_strrchr = strcmp(name, "strrchr") == 0 || strcmp(name, "rindex") == 0;
    int is_strnlen = strcmp(name, "strnlen") == 0;
    if (!is_memchr && !is_strchr && !is_strrchr && !is_strnlen) return 0;
    int defined = 0;
    if (find_function(c->ir, name, &defined) == 0) return 0;

    A64Asm *a = c->as;
    int Lloop = a64_new_label(a);
    int Ldone = a64_new_label(a);
    int Lmiss = a64_new_label(a);

    if (is_strnlen) {
        a64_mov_reg(a, A64_X3, A64_X0, 1);
        a64_bind(a, Lloop);
        a64_cbz(a, A64_X1, Ldone, 1);
        a64_ldr8(a, A64_X4, A64_X0, 0);
        a64_cbz(a, A64_X4, Ldone, 0);
        a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
        a64_sub_imm12(a, A64_X1, A64_X1, 1, 0, 1, 0);
        a64_b(a, Lloop);
        a64_bind(a, Ldone);
        a64_sub_reg(a, A64_X0, A64_X0, A64_X3, A64_LSL, 0, 1, 0);
        return 1;
    }

    a64_and_imm(a, A64_X1, A64_X1, 0xff, 0);
    if (is_strrchr)
        a64_movz(a, A64_X4, 0, 0, 1);
    a64_bind(a, Lloop);
    if (is_memchr)
        a64_cbz(a, A64_X2, Lmiss, 1);
    a64_ldr8(a, A64_X3, A64_X0, 0);
    a64_cmp_reg(a, A64_X3, A64_X1, 0);
    if (is_strrchr) {
        int Lnext = a64_new_label(a);
        a64_bcond(a, A64_NE, Lnext);
        a64_mov_reg(a, A64_X4, A64_X0, 1);
        a64_bind(a, Lnext);
        a64_cbz(a, A64_X3, Ldone, 0);
        a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
        a64_b(a, Lloop);
        a64_bind(a, Ldone);
        a64_mov_reg(a, A64_X0, A64_X4, 1);
        return 1;
    }
    a64_bcond(a, A64_EQ, Ldone);
    if (!is_memchr)
        a64_cbz(a, A64_X3, Lmiss, 0);
    a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
    if (is_memchr)
        a64_sub_imm12(a, A64_X2, A64_X2, 1, 0, 1, 0);
    a64_b(a, Lloop);
    a64_bind(a, Lmiss);
    a64_movz(a, A64_X0, 0, 0, 1);
    a64_bind(a, Ldone);
    return 1;
}

/* strspn/strcspn.  The span length is left in x0.  x2–x5 are caller-saved.
 * A user definition of the same name still wins. */
static int emit_span_builtin(C64 *c, const char *name) {
    int is_spn = strcmp(name, "strspn") == 0;
    int is_cspn = strcmp(name, "strcspn") == 0;
    if (!is_spn && !is_cspn) return 0;
    int defined = 0;
    if (find_function(c->ir, name, &defined) == 0) return 0;

    A64Asm *a = c->as;
    int Louter = a64_new_label(a);
    int Linner = a64_new_label(a);
    int Ladvance = a64_new_label(a);
    int Ldone = a64_new_label(a);

    a64_mov_reg(a, A64_X4, A64_X0, 1);
    a64_bind(a, Louter);
    a64_ldr8(a, A64_X2, A64_X0, 0);
    a64_cbz(a, A64_X2, Ldone, 0);
    a64_mov_reg(a, A64_X5, A64_X1, 1);
    a64_bind(a, Linner);
    a64_ldr8(a, A64_X3, A64_X5, 0);
    /* strspn stops when accept runs out.  strcspn advances instead. */
    a64_cbz(a, A64_X3, is_spn ? Ldone : Ladvance, 0);
    a64_cmp_reg(a, A64_X2, A64_X3, 0);
    a64_bcond(a, A64_EQ, is_spn ? Ladvance : Ldone);
    a64_add_imm12(a, A64_X5, A64_X5, 1, 0, 1, 0);
    a64_b(a, Linner);
    a64_bind(a, Ladvance);
    a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
    a64_b(a, Louter);
    a64_bind(a, Ldone);
    a64_sub_reg(a, A64_X0, A64_X0, A64_X4, A64_LSL, 0, 1, 0);
    return 1;
}

/* strcpy/stpcpy/strncpy/strcat/strncat/strstr.  Result pointer in x0.
 * x3–x5 are caller-saved.  A user definition of the same name still wins. */
static int emit_copy_builtin(C64 *c, const char *name) {
    int is_strcpy = strcmp(name, "strcpy") == 0;
    int is_stpcpy = strcmp(name, "stpcpy") == 0;
    int is_strncpy = strcmp(name, "strncpy") == 0;
    int is_stpncpy = strcmp(name, "stpncpy") == 0;
    int is_strcat = strcmp(name, "strcat") == 0;
    int is_strncat = strcmp(name, "strncat") == 0;
    int is_strstr = strcmp(name, "strstr") == 0;
    if (!is_strcpy && !is_stpcpy && !is_strncpy && !is_stpncpy && !is_strcat
        && !is_strncat && !is_strstr)
        return 0;
    int defined = 0;
    if (find_function(c->ir, name, &defined) == 0) return 0;

    A64Asm *a = c->as;
    if (is_strstr) {
        int Louter = a64_new_label(a);
        int Linner = a64_new_label(a);
        int Lnext = a64_new_label(a);
        int Lmiss = a64_new_label(a);
        int Ldone = a64_new_label(a);
        a64_ldr8(a, A64_X2, A64_X1, 0);
        a64_cbz(a, A64_X2, Ldone, 0);
        a64_bind(a, Louter);
        a64_ldr8(a, A64_X3, A64_X0, 0);
        a64_cbz(a, A64_X3, Lmiss, 0);
        a64_mov_reg(a, A64_X4, A64_X0, 1);
        a64_mov_reg(a, A64_X5, A64_X1, 1);
        a64_bind(a, Linner);
        a64_ldr8(a, A64_X2, A64_X5, 0);
        a64_cbz(a, A64_X2, Ldone, 0);
        a64_ldr8(a, A64_X3, A64_X4, 0);
        a64_cmp_reg(a, A64_X2, A64_X3, 0);
        a64_bcond(a, A64_NE, Lnext);
        a64_add_imm12(a, A64_X4, A64_X4, 1, 0, 1, 0);
        a64_add_imm12(a, A64_X5, A64_X5, 1, 0, 1, 0);
        a64_b(a, Linner);
        a64_bind(a, Lnext);
        a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
        a64_b(a, Louter);
        a64_bind(a, Lmiss);
        a64_movz(a, A64_X0, 0, 0, 1);
        a64_bind(a, Ldone);
        return 1;
    }

    if (is_strncpy || is_stpncpy) {
        int Lloop = a64_new_label(a);
        int Lpad = a64_new_label(a);
        int Lnul = a64_new_label(a);
        int Ldone = a64_new_label(a);
        int Lend = a64_new_label(a);
        a64_mov_reg(a, A64_X4, A64_X0, 1);
        a64_movz(a, A64_X5, 0, 0, 1); /* address of the first NUL, or 0 */
        a64_bind(a, Lloop);
        a64_cbz(a, A64_X2, Ldone, 1);
        a64_ldr8(a, A64_X3, A64_X1, 0);
        a64_str8(a, A64_X3, A64_X0, 0);
        a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
        a64_sub_imm12(a, A64_X2, A64_X2, 1, 0, 1, 0);
        a64_cbz(a, A64_X3, Lnul, 0);
        a64_add_imm12(a, A64_X1, A64_X1, 1, 0, 1, 0);
        a64_b(a, Lloop);
        a64_bind(a, Lnul);
        a64_sub_imm12(a, A64_X5, A64_X0, 1, 0, 1, 0);
        a64_bind(a, Lpad);
        a64_cbz(a, A64_X2, Ldone, 1);
        a64_str8(a, 31, A64_X0, 0);
        a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
        a64_sub_imm12(a, A64_X2, A64_X2, 1, 0, 1, 0);
        a64_b(a, Lpad);
        a64_bind(a, Ldone);
        if (is_stpncpy) {
            /* No NUL inside n bytes: return dest+n (x0).  Otherwise the
             * first NUL, saved in x5. */
            a64_cbz(a, A64_X5, Lend, 1);
            a64_mov_reg(a, A64_X0, A64_X5, 1);
        } else {
            a64_mov_reg(a, A64_X0, A64_X4, 1);
        }
        a64_bind(a, Lend);
        return 1;
    }

    int Lfind = a64_new_label(a);
    int Lcopy = a64_new_label(a);
    int Lterm = a64_new_label(a);
    int Ldone = a64_new_label(a);
    a64_mov_reg(a, A64_X4, A64_X0, 1);
    if (is_strcat || is_strncat) {
        a64_bind(a, Lfind);
        a64_ldr8(a, A64_X3, A64_X0, 0);
        a64_cbz(a, A64_X3, Lcopy, 0);
        a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
        a64_b(a, Lfind);
    }
    a64_bind(a, Lcopy);
    if (is_strncat) {
        a64_cbz(a, A64_X2, Lterm, 1);
        a64_ldr8(a, A64_X3, A64_X1, 0);
        a64_cbz(a, A64_X3, Lterm, 0);
        a64_str8(a, A64_X3, A64_X0, 0);
        a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
        a64_add_imm12(a, A64_X1, A64_X1, 1, 0, 1, 0);
        a64_sub_imm12(a, A64_X2, A64_X2, 1, 0, 1, 0);
        a64_b(a, Lcopy);
        a64_bind(a, Lterm);
        a64_str8(a, 31, A64_X0, 0);
        a64_mov_reg(a, A64_X0, A64_X4, 1);
        return 1;
    }
    a64_ldr8(a, A64_X3, A64_X1, 0);
    a64_str8(a, A64_X3, A64_X0, 0);
    a64_add_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
    a64_add_imm12(a, A64_X1, A64_X1, 1, 0, 1, 0);
    a64_cbnz(a, A64_X3, Lcopy, 0);
    a64_bind(a, Ldone);
    if (is_stpcpy)
        a64_sub_imm12(a, A64_X0, A64_X0, 1, 0, 1, 0);
    else
        a64_mov_reg(a, A64_X0, A64_X4, 1);
    return 1;
}

/* Bytes of outgoing stack traffic for one call: 8 per stack slot, plus
 * 16 when call_args[0] must be stashed before it is written to x8. */
static int is_va_builtin(const char *name) {
    return name && (strcmp(name, "va_start") == 0 || strcmp(name, "va_arg") == 0
                    || strcmp(name, "va_end") == 0);
}

/* GCC integer bit builtins stay named calls so x86 can emit lzcnt/tzcnt.
 * On arm64 they are instructions, not libc helpers.  `long` is 64-bit. */
static int is_bit_builtin(const char *bn) {
    if (!bn || strncmp(bn, "__builtin_", 10) != 0) return 0;
    return strstr(bn, "clz") || strstr(bn, "ctz") || strstr(bn, "ffs")
        || strstr(bn, "popcount") || strstr(bn, "parity")
        || strstr(bn, "clrsb") || strstr(bn, "bswap");
}

static int bit_is64(const char *bn) {
    if (strstr(bn, "bswap16") || strstr(bn, "bswap32")) return 0;
    if (strstr(bn, "bswap64")) return 1;
    size_t n = strlen(bn);
    if (n >= 2 && bn[n - 2] == 'l' && bn[n - 1] == 'l') return 1;
    if (n >= 1 && bn[n - 1] == 'l') return 1;
    return 0;
}

static int safe_tmp(int a, int b, int c, int d);

static void bit1(A64Asm *a, uint32_t base, int rd, int rn) {
    a64_word(a, base | ((uint32_t)(rn & 31) << 5) | (uint32_t)(rd & 31));
}

/* CNT + ADDV into V31, which the allocator never assigns. */
static void emit_popc(A64Asm *a, int dst, int src, int is64) {
    a64_fmov_gp(a, FSCR, src, 1, is64);
    a64_word(a, 0x0E205800u | ((uint32_t)FSCR << 5) | (uint32_t)FSCR);
    a64_word(a, 0x0E31B800u | ((uint32_t)FSCR << 5) | (uint32_t)FSCR);
    a64_fmov_gp(a, dst, FSCR, 0, 0);
}

static void emit_bit_builtin(C64 *c, const IRInst *s) {
    A64Asm *a = c->as;
    const char *bn = s->call_name;
    if (s->dst < 0 || s->call_nargs < 1) return;
    int is64 = bit_is64(bn);
    int dst = dst_reg(c, s->dst);
    int src = load_op(c, s->call_args[0], dst);
    uint32_t clz = is64 ? 0xDAC01000u : 0x5AC01000u;
    uint32_t rbit = is64 ? 0xDAC00000u : 0x5AC00000u;
    if (strstr(bn, "bswap")) {
        if (strstr(bn, "16")) {
            /* rev + lsr 16 keeps only the swapped low half. */
            bit1(a, 0x5AC00800u, dst, src);
            a64_lsr_imm(a, dst, dst, 16, 0);
        } else if (is64) {
            bit1(a, 0xDAC00C00u, dst, src);
        } else {
            bit1(a, 0x5AC00800u, dst, src);
        }
    } else if (strstr(bn, "clrsb")) {
        bit1(a, is64 ? 0xDAC01400u : 0x5AC01400u, dst, src);
    } else if (strstr(bn, "ffs")) {
        /* 0 if src is zero, otherwise ctz(src)+1. */
        int tmp = safe_tmp(src, -1, -1, -1);
        bit1(a, rbit, tmp, src);
        bit1(a, clz, tmp, tmp);
        a64_cmp_imm12(a, src, 0, 0, is64);
        a64_word(a, 0x1A8007E0u | ((uint32_t)(tmp & 31) << 16)
                                | (uint32_t)(dst & 31));
    } else if (strstr(bn, "ctz")) {
        bit1(a, rbit, dst, src);
        bit1(a, clz, dst, dst);
    } else if (strstr(bn, "clz")) {
        bit1(a, clz, dst, src);
    } else if (strstr(bn, "popcount") || strstr(bn, "parity")) {
        emit_popc(a, dst, src, is64);
        if (strstr(bn, "parity"))
            a64_and_imm(a, dst, dst, 1, 0);
    }
    commit(c, s->dst, dst);
}

static int outgoing_stack_bytes(C64 *c, const IRInst *s) {
    if (!s->call_nargs) return s->align16 == A64_MARK_SRET ? 16 : 0;
    if (s->call_name && strcmp(s->call_name, "__syscall") == 0) return 0;
    if (is_va_builtin(s->call_name)) return 0;
    int start = s->align16 == A64_MARK_SRET ? 1 : 0;
    int gp = 0, fp = 0, st = 0;
    for (int i = start; i < s->call_nargs; i++) {
        unsigned f = s->call_arg_on_stack ? s->call_arg_on_stack[i] : 0;
        int scalar = scalar_fp_val(c, s->call_args[i]);
        int qv = vec16_val(c, s->call_args[i]);
        if ((scalar || qv) && !(f & CALL_ARG_STACK) && fp < 8) {
            fp++;
            continue;
        }
        if ((f & CALL_ARG_HFA) && !(f & CALL_ARG_STACK) && fp < 8) {
            fp++;
            continue;
        }
        if ((f & CALL_ARG_STACK) || (f & CALL_ARG_BLOB) || gp >= 8 || scalar || qv) {
            if (qv) {
                if (st & 15) st = (st + 15) & ~15;
                st += 16;
            } else {
                st += 8;
            }
        } else {
            gp++;
        }
    }
    int bytes = st;
    if (s->align16 == A64_MARK_SRET) bytes += 16;
    return bytes;
}

/* A scratch that is not one of the named registers (x0/x1/x16/x17 are
 * never allocated, so one of them is always free of homes). */
static int safe_tmp(int a, int b, int c, int d) {
    int cand[4] = { SCR0, SCR1, A64_X0, A64_X1 };
    for (int i = 0; i < 4; i++) {
        int t = cand[i];
        if (t != a && t != b && t != c && t != d) return t;
    }
    return SCR0;
}

static void place_from(C64 *c, IRValue v, int src) {
    if (v < 0) return;
    int hr = home_reg(c, v);
    if (hr < 0)
        frame_store(c, src, spill_off(c, v), 8);
    else if (hr != src)
        a64_mov_reg(c->as, hr, src, 1);
}

/* Deliver two hardware registers into SSA homes without either write
 * destroying the other source. */
static void place_pair(C64 *c, IRValue lo, IRValue hi, int s0, int s1) {
    if (hi < 0) { place_from(c, lo, s0); return; }
    int h0 = home_reg(c, lo);
    int h1 = home_reg(c, hi);
    if (h0 == s1) {
        int tmp = safe_tmp(s0, s1, h0, h1);
        a64_mov_reg(c->as, tmp, s1, 1);
        place_from(c, lo, s0);
        place_from(c, hi, tmp);
    } else if (h1 == s0) {
        int tmp = safe_tmp(s0, s1, h0, h1);
        a64_mov_reg(c->as, tmp, s0, 1);
        place_from(c, hi, s1);
        place_from(c, lo, tmp);
    } else {
        place_from(c, hi, s1);
        place_from(c, lo, s0);
    }
}

/* Copy `sz` bytes from src to dest.  Both addresses are advanced.
 * x0/x1 are not allocatable, so one of them can hold the chunk. */
static void copy_bytes(C64 *c, int dest, int src, int sz) {
    A64Asm *a = c->as;
    int data = A64_X0;
    if (data == dest || data == src) data = A64_X1;
    if (data == dest || data == src) data = SCR0;
    int off = 0;
    while (off < sz) {
        int left = sz - off;
        int w = 1;
        if (left >= 8 && (off & 7) == 0) w = 8;
        else if (left >= 4 && (off & 3) == 0) w = 4;
        else if (left >= 2 && (off & 1) == 0) w = 2;
        emit_load(c, data, src, w, 1);
        emit_store(c, data, dest, w);
        off += w;
        if (off < sz) {
            a64_add_imm12(a, src, src, (unsigned)w, 0, 1, 0);
            a64_add_imm12(a, dest, dest, (unsigned)w, 0, 1, 0);
        }
    }
}

/* Darwin va_list is one pointer.  va_start stores the address of the
 * first anonymous stack slot; va_arg reads 8-byte slots and advances. */
static void emit_va(C64 *c, const IRInst *s) {
    A64Asm *a = c->as;
    if (strcmp(s->call_name, "va_end") == 0) return;

    int ap_home = home_reg(c, s->call_args[0]);
    int ap = ap_home >= 0 ? ap_home : SCR0;
    if (ap_home < 0)
        frame_load(c, ap, spill_off(c, s->call_args[0]), 8, 1);

    if (strcmp(s->call_name, "va_start") == 0) {
        int cur = (ap == SCR1) ? A64_X0 : SCR1;
        emit_fp_addr(c, cur, c->vararg_off);
        a64_str64(a, cur, ap, 0);
        return;
    }

    int cur = (ap == SCR1) ? A64_X0 : SCR1;
    a64_ldr64(a, cur, ap, 0);
    int sz = s->imm > 0 ? (int)s->imm : 0;
    int indirect = sz > 16 || s->force_stack;
    int adv = (sz == 0 || indirect) ? 8 : ((sz + 7) & ~7);
    int nxt = A64_X0;
    if (nxt == cur || nxt == ap) nxt = A64_X1;
    if (nxt == cur || nxt == ap) nxt = SCR0;
    a64_add_imm12(a, nxt, cur, (unsigned)adv, 0, 1, 0);
    a64_str64(a, nxt, ap, 0);

    if (sz == 0) {
        int w = s->width ? (int)s->width : 8;
        if (w != 1 && w != 2 && w != 4 && w != 8) w = 8;
        if (scalar_fp_val(c, s->dst)) {
            int h = fp_home(c, s->dst);
            int d = h >= 0 ? h : FSCR;
            if (w == 8) a64_ldr_d(a, d, cur, 0);
            else a64_ldr_s(a, d, cur, 0);
            if (h < 0) commit_fp(c, s->dst, d);
            return;
        }
        int d = dst_reg(c, s->dst);
        int tmp = d;
        if (tmp == cur || tmp == ap)
            tmp = safe_tmp(cur, ap, -1, -1);
        emit_load(c, tmp, cur, w, s->is_unsigned);
        if (tmp != d) a64_mov_reg(a, d, tmp, w == 8);
        commit(c, s->dst, d);
        return;
    }

    if (indirect)
        a64_ldr64(a, cur, cur, 0);
    int dest_home = s->call_nargs >= 2 ? home_reg(c, s->call_args[1]) : -1;
    int base;
    if (dest_home >= 0 && dest_home != cur && dest_home != nxt)
        base = dest_home;
    else {
        base = safe_tmp(cur, ap, nxt, dest_home);
        if (dest_home >= 0)
            a64_mov_reg(a, base, dest_home, 1);
        else if (s->call_nargs >= 2)
            frame_load(c, base, spill_off(c, s->call_args[1]), 8, 1);
    }
    /* nxt is free once *ap has been stored, and it is not `base`. */
    a64_mov_reg(a, nxt, base, 1);
    copy_bytes(c, nxt, cur, sz);
    place_from(c, s->dst, base);
}

/* Names after the IR strips the __builtin_ prefix.  A trailing f/l is the
 * width suffix; long double is IEEE double on Darwin, so 'l' uses the
 * 64-bit instruction.  A user function of the same name still wins. */
static int fp_named(const char *n, const char *root) {
    size_t L = strlen(root);
    if (strncmp(n, root, L) != 0) return 0;
    char s = n[L];
    return s == '\0' || ((s == 'f' || s == 'l') && n[L + 1] == '\0');
}

static int fp_builtin_kind(const char *n, int *nargs) {
    if (!n) return 0;
    struct { const char *root; int kind; int narg; } tab[] = {
        {"copysign", 1, 2}, {"nearbyint", 2, 1}, {"floor", 3, 1},
        {"trunc", 4, 1}, {"round", 5, 1}, {"ceil", 6, 1},
        {"fabs", 7, 1}, {"sqrt", 8, 1}, {"fmin", 9, 2},
        {"fmax", 10, 2}, {"fma", 11, 3}, {"rint", 2, 1},
    };
    for (size_t i = 0; i < sizeof tab / sizeof tab[0]; i++) {
        if (fp_named(n, tab[i].root)) {
            *nargs = tab[i].narg;
            return tab[i].kind;
        }
    }
    return 0;
}

static void fp_unop(A64Asm *a, uint32_t base_d, int rd, int rn, int isd) {
    uint32_t base = isd ? base_d : (base_d & ~0x00400000u);
    a64_word(a, base | ((uint32_t)(rn & 31) << 5) | (uint32_t)(rd & 31));
}

static void fp_binop(A64Asm *a, uint32_t base_d, int rd, int rn, int rm, int isd) {
    uint32_t base = isd ? base_d : (base_d & ~0x00400000u);
    a64_word(a, base | ((uint32_t)(rm & 31) << 16)
                    | ((uint32_t)(rn & 31) << 5)
                    | (uint32_t)(rd & 31));
}

static int fp_result(C64 *c, IRValue v, int fallback) {
    int h = fp_home(c, v);
    return h >= 0 ? h : fallback;
}

static void emit_fp_builtin(C64 *c, const IRInst *s, int kind) {
    A64Asm *a = c->as;
    if (s->dst < 0) return;
    int isd = vw(c, s->call_args[0]) == 8;
    if (kind == 1) {
        /* copysign: clear the sign bit, then OR in the sign of arg2. */
        int mag = load_fp(c, s->call_args[0], -1);
        int sgn = load_fp(c, s->call_args[1], mag);
        a64_fmov_gp(a, A64_X0, mag, 0, isd);
        a64_fmov_gp(a, A64_X1, sgn, 0, isd);
        unsigned sign = isd ? 63u : 31u;
        a64_lsl_imm(a, A64_X0, A64_X0, 1, isd);
        a64_lsr_imm(a, A64_X0, A64_X0, 1, isd);
        a64_lsr_imm(a, A64_X1, A64_X1, sign, isd);
        a64_lsl_imm(a, A64_X1, A64_X1, sign, isd);
        a64_or_reg(a, A64_X0, A64_X0, A64_X1, isd);
        int dst = fp_result(c, s->dst, (mag == FSCR || sgn == FSCR) ? A64_V30 : FSCR);
        a64_fmov_gp(a, dst, A64_X0, 1, isd);
        commit_fp(c, s->dst, dst);
        return;
    }
    if (kind == 11) {
        /* Park all three in GP first: a spilled third operand would
         * otherwise reuse the only two vector scratches. */
        int r = load_fp(c, s->call_args[0], -1);
        a64_fmov_gp(a, A64_X0, r, 0, isd);
        r = load_fp(c, s->call_args[1], -1);
        a64_fmov_gp(a, A64_X1, r, 0, isd);
        r = load_fp(c, s->call_args[2], -1);
        a64_fmov_gp(a, SCR0, r, 0, isd);
        a64_fmov_gp(a, A64_V0, A64_X0, 1, isd);
        a64_fmov_gp(a, A64_V1, A64_X1, 1, isd);
        a64_fmov_gp(a, A64_V2, SCR0, 1, isd);
        int dst = fp_result(c, s->dst, A64_V3);
        /* FMADD Dd, Dn, Dm, Da = arg0 * arg1 + arg2. */
        uint32_t base = isd ? 0x1F400000u : 0x1F000000u;
        a64_word(a, base | ((uint32_t)A64_V1 << 16) | ((uint32_t)A64_V2 << 10)
                       | ((uint32_t)A64_V0 << 5) | (uint32_t)(dst & 31));
        commit_fp(c, s->dst, dst);
        return;
    }
    if (kind == 9 || kind == 10) {
        int a0 = load_fp(c, s->call_args[0], -1);
        int a1 = load_fp(c, s->call_args[1], a0);
        int dst = fp_result(c, s->dst, a0);
        /* FMINNM/FMAXNM: a quiet NaN loses to the numeric operand.
         * Plain FMIN/FMAX return the NaN on this CPU. */
        fp_binop(a, kind == 9 ? 0x1E607800u : 0x1E606800u, dst, a0, a1, isd);
        commit_fp(c, s->dst, dst);
        return;
    }
    int src = load_fp(c, s->call_args[0], -1);
    int dst = fp_result(c, s->dst, src == FSCR ? A64_V30 : FSCR);
    uint32_t base = 0x1E60C000u; /* fabs */
    if (kind == 8) base = 0x1E61C000u;       /* sqrt */
    else if (kind == 6) base = 0x1E64C000u;  /* ceil  = frintp */
    else if (kind == 3) base = 0x1E654000u;  /* floor = frintm */
    else if (kind == 4) base = 0x1E65C000u;  /* trunc = frintz */
    else if (kind == 5) base = 0x1E664000u;  /* round = frinta */
    else if (kind == 2) base = 0x1E67C000u;  /* rint / nearbyint = frinti */
    fp_unop(a, base, dst, src, isd);
    commit_fp(c, s->dst, dst);
}

/* __builtin_trap and __builtin_abort both arrive as a call named abort.
 * __builtin_unreachable keeps the stripped name.  brk #1 is what clang
 * emits; the kernel reports SIGTRAP.  A user-defined function wins. */
static int emit_trap_builtin(C64 *c, const char *name) {
    if (!name || (strcmp(name, "abort") != 0 && strcmp(name, "unreachable") != 0))
        return 0;
    int defined = 0;
    if (find_function(c->ir, name, &defined) == 0) return 0;
    a64_word(c->as, 0xD4200020u);
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
    if (is_va_builtin(s->call_name)) {
        emit_va(c, s);
        return;
    }
    if (is_bit_builtin(s->call_name)) {
        emit_bit_builtin(c, s);
        return;
    }
    {
        int fp_narg = 0;
        int fp_kind = fp_builtin_kind(s->call_name, &fp_narg);
        if (fp_kind && s->call_nargs >= fp_narg) {
            int defined = 0;
            if (find_function(c->ir, s->call_name, &defined) != 0) {
                emit_fp_builtin(c, s, fp_kind);
                return;
            }
        }
    }
    if (emit_trap_builtin(c, s->call_name)) return;

    int sret = s->align16 == A64_MARK_SRET;
    int start = sret ? 1 : 0;
    int gp_at[8], gp_n = 0, gp_used = 0, fp_used = 0, st_used = 0;
    int hfa_at[8], hfa_v[8], hfa_n = 0;
    int fp_at[8], fp_vn[8], fp_n = 0;
    int stk_at[IR_CALL_MAX_ARGS];
    int stk_off[IR_CALL_MAX_ARGS];
    int stk_n = 0;
    int stk_bytes = 0;
    for (int i = start; i < n; i++) {
        unsigned f = s->call_arg_on_stack ? s->call_arg_on_stack[i] : 0;
        int scalar = scalar_fp_val(c, s->call_args[i]);
        int qv = vec16_val(c, s->call_args[i]);
        if (f & CALL_ARG_BLOB)
            c64_die(c, s, "aggregate stack blob");
        if ((scalar || qv) && !(f & CALL_ARG_STACK) && fp_used < 8) {
            fp_at[fp_n] = i;
            fp_vn[fp_n] = fp_used++;
            fp_n++;
        } else if ((f & CALL_ARG_HFA) && !(f & CALL_ARG_STACK) && fp_used < 8) {
            hfa_at[hfa_n] = i;
            hfa_v[hfa_n] = fp_used++;
            hfa_n++;
        } else if ((f & CALL_ARG_STACK) || gp_used >= 8 || scalar || qv) {
            if (qv && (stk_bytes & 15))
                stk_bytes = (stk_bytes + 15) & ~15;
            stk_at[stk_n] = i;
            stk_off[stk_n] = stk_bytes;
            stk_bytes += qv ? 16 : 8;
            stk_n++;
            st_used++;
        } else {
            gp_at[gp_n++] = i;
            gp_used++;
        }
    }
    (void)st_used;

    /* Stack slots first: they read homes that the GP shuffle will
     * overwrite.  The area sits at [sp+0 ..).  A 16-byte vector occupies
     * one aligned Q slot once v0–v7 are full. */
    for (int i = 0; i < stk_n; i++) {
        IRValue av = s->call_args[stk_at[i]];
        int off = stk_off[i];
        if (vec16_val(c, av)) {
            int r = load_q(c, av, -1);
            str_q(a, r, A64_SP, off);
        } else if (scalar_fp_val(c, av)) {
            int r = load_fp(c, av, -1);
            if (vw(c, av) == 8) a64_str_d(a, r, A64_SP, off);
            else a64_str_s(a, r, A64_SP, off);
        } else {
            int r = load_op(c, av, -1);
            a64_str64(a, r, A64_SP, off);
        }
    }
    /* Scalar float/double arguments.  A home in v1 moving to v0 must
     * not run before the value that still lives in v0. */
    {
        int fsrc[8], fdst[8], fisd[8], fdone[8], fhome[8];
        IRValue fav[8];
        for (int i = 0; i < fp_n; i++) {
            fav[i] = s->call_args[fp_at[i]];
            fsrc[i] = fp_home(c, fav[i]);
            fhome[i] = fsrc[i] >= 0;
            fdst[i] = fp_vn[i];
            fisd[i] = vec16_val(c, fav[i]) ? 2 : (vw(c, fav[i]) == 8);
            fdone[i] = fhome[i] && fsrc[i] == fdst[i];
        }
        int nleft = fp_n;
        for (int i = 0; i < fp_n; i++) nleft -= fdone[i];
        while (nleft > 0) {
            int picked = -1;
            for (int i = 0; i < fp_n; i++)
                if (!fdone[i] && fsrc[i] == FSCR) { picked = i; break; }
            for (int i = 0; picked < 0 && i < fp_n; i++) {
                if (fdone[i]) continue;
                int blocked = 0;
                if (fhome[i])
                    for (int j = 0; j < fp_n; j++)
                        if (!fdone[j] && j != i && fhome[j] && fsrc[j] == fdst[i])
                            blocked = 1;
                if (!blocked) { picked = i; break; }
            }
            if (picked < 0) {
                for (int i = 0; i < fp_n; i++)
                    if (!fdone[i] && fhome[i]) {
                        if (fisd[i] == 2) fmov_q(a, FSCR, fsrc[i]);
                        else a64_fmov_reg(a, FSCR, fsrc[i], fisd[i]);
                        fsrc[i] = FSCR;
                        fhome[i] = 0;
                        break;
                    }
                continue;
            }
            int r = fhome[picked] ? fsrc[picked]
                    : (fisd[picked] == 2 ? load_q(c, fav[picked], fdst[picked])
                                         : load_fp(c, fav[picked], fdst[picked]));
            if (r != fdst[picked]) {
                if (fisd[picked] == 2) fmov_q(a, fdst[picked], r);
                else a64_fmov_reg(a, fdst[picked], r, fisd[picked]);
            }
            fdone[picked] = 1;
            nleft--;
        }
    }
    /* HFA eightbytes: integer bits → dN/sN, before GP moves. */
    for (int i = 0; i < hfa_n; i++) {
        IRValue av = s->call_args[hfa_at[i]];
        int r = load_op(c, av, -1);
        a64_fmov_gp(a, hfa_v[i], r, 1, vw(c, av) == 8);
    }
    /* Indirect result pointer, stashed above the outgoing args. */
    if (sret) {
        int r = load_op(c, s->call_args[0], -1);
        a64_str64(a, r, A64_SP, c->call_area - 16);
    }

    /* Lift an indirect target into x17 (IP1), which no allocation owns. */
    int tgt = -1;
    if (s->call_callee >= 0) {
        int r = load_ptrv(c, s->call_callee, -1);
        a64_mov_reg(a, SCR1, r, 1);
        tgt = SCR1;
    }

    /* Register-argument moves with cycle breaking.
     *   kind 0: source lives in register src[i]
     *   kind 1: source is a spill slot / CONST (materialized into xi)
     * Slot i's destination is x{dreg}, which is not necessarily x{i}
     * once HFA and stack-forced args have opened holes. */
    int src[8], kind[8], done[8], dreg[8];
    for (int i = 0; i < gp_n; i++) {
        done[i] = 0;
        dreg[i] = A64_X0 + i;
        IRValue av = s->call_args[gp_at[i]];
        const IRInst *d = def_inst(c, av);
        int r = (d && d->op == IR_CONST) ? -1 : home_reg(c, av);
        if (r >= 0) { kind[i] = 0; src[i] = r; }
        else { kind[i] = 1; src[i] = -1; }
    }

    int remaining = gp_n;
    while (remaining > 0) {
        int picked = -1;
        for (int i = 0; i < gp_n; i++) {
            if (done[i]) continue;
            int blocked = 0;
            for (int j = 0; j < gp_n; j++)
                if (!done[j] && j != i && kind[j] == 0 &&
                    src[j] == dreg[i]) { blocked = 1; break; }
            if (!blocked) { picked = i; break; }
        }
        if (picked < 0) {
            for (int i = 0; i < gp_n; i++)
                if (!done[i] && kind[i] == 0) {
                    a64_mov_reg(a, SCR0, src[i], 1);
                    src[i] = SCR0;
                    break;
                }
            continue;
        }
        int i = picked;
        IRValue av = s->call_args[gp_at[i]];
        int is64 = vw(c, av) == 8;
        if (kind[i] == 0) {
            if (src[i] != dreg[i])
                a64_mov_reg(a, dreg[i], src[i], is64);
        } else {
            const IRInst *d = def_inst(c, av);
            if (d && d->op == IR_CONST) {
                emit_mov_imm_w(a, dreg[i], d->imm, is64);
            } else {
                int so = spill_off(c, av);
                frame_load(c, dreg[i], so, is64 ? 8 : 4, is64 ? 1 : 0);
            }
        }
        done[i] = 1;
        remaining--;
    }

    if (sret)
        a64_ldr64(a, A64_X8, A64_SP, c->call_area - 16);

    if (tgt >= 0) {
        a64_blr(a, tgt);
    } else if (s->call_name && strcmp(s->call_name, "__fakecc_udivmodti4") == 0) {
        if (c->udiv_label < 0)
            c->udiv_label = a64_new_label(a);
        a64_bl(a, c->udiv_label);
    } else if (!(s->call_name && (emit_mem_builtin(c, s->call_name)
                                 || emit_scan_builtin(c, s->call_name)
                                 || emit_find_builtin(c, s->call_name)
                                 || emit_span_builtin(c, s->call_name)
                                 || emit_copy_builtin(c, s->call_name)))) {
        int fi = 0;
        if (!s->call_name) {
            die_at(s->loc.file ? s->loc.file : c->fn->loc.file,
                   s->loc.line, s->loc.col,
                   "arm64 backend: call with no target");
        } else if (find_function(c->ir, s->call_name, &fi) != 0) {
            if (!emit_object_mode())
                die_at(s->loc.file ? s->loc.file : c->fn->loc.file,
                       s->loc.line, s->loc.col,
                       "arm64 backend: call to undefined function '%s'",
                       s->call_name);
            else {
                if (c->nxcall == c->capxcall) {
                    c->capxcall = c->capxcall ? c->capxcall * 2 : 8;
                    c->xcall = xrealloc(c->xcall, c->capxcall * sizeof *c->xcall);
                }
                c->xcall[c->nxcall].at = (uint32_t)a->code.len;
                c->xcall[c->nxcall].name = s->call_name;
                c->nxcall++;
                a64_word(a, 0x94000000u); /* bl, ARM64_RELOC_BRANCH26 */
            }
        } else
            a64_bl(a, c->fn_label[fi]);
    }

    if (s->align16 == A64_MARK_HFA) {
        int r0 = SCR0, r1 = SCR1;
        a64_fmov_gp(a, r0, A64_V0, 0, 1);
        if (s->b >= 0)
            a64_fmov_gp(a, r1, A64_V1, 0, 1);
        place_pair(c, s->dst, s->b, r0, r1);
    } else if (s->dst >= 0 && vec16_val(c, s->dst)) {
        int h = fp_home(c, s->dst);
        if (h < 0) commit_q(c, s->dst, A64_V0);
        else fmov_q(c->as, h, A64_V0);
    } else if (s->dst >= 0 && scalar_fp_val(c, s->dst)) {
        int h = fp_home(c, s->dst);
        int isd = vw(c, s->dst) == 8 || s->width == 8;
        if (h < 0) commit_fp(c, s->dst, A64_V0);
        else if (h != A64_V0) a64_fmov_reg(a, h, A64_V0, isd);
    } else if (s->dst >= 0 && s->b >= 0) {
        place_pair(c, s->dst, s->b, A64_X0, A64_X1);
    } else if (s->dst >= 0) {
        int d = dst_reg(c, s->dst);
        if (d != A64_X0)
            a64_mov_reg(a, d, A64_X0, s->width == 8);
        commit(c, s->dst, d);
    }
}

/* Raw Darwin syscall: number in x16, arguments in x0..x5, svc #0x80.
 * The kernel sets the carry flag and leaves a positive errno in x0 on
 * failure.  x9 (caller-saved, already forbidden across the call, and
 * the scratch Apple's own stubs use) records that carry; a set carry
 * negates x0 so the result is the Linux-shaped -errno. */
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
        const IRInst *d = def_inst(c, av);
        int r = (d && d->op == IR_CONST) ? -1 : home_reg(c, av);
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
            const IRInst *d = def_inst(c, av);
            if (d && d->op == IR_CONST) {
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
    const IRInst *nd = def_inst(c, nv);
    if (nd && nd->op == IR_CONST) {
        emit_mov_imm_w(a, A64_X16, nd->imm, 1);
    } else {
        int sr = load_op(c, nv, SCR0);
        a64_mov_reg(a, A64_X16, sr, 1);
    }
    a64_svc(a, 0x80);
    {
        int ok = a64_new_label(a);
        a64_cset(a, A64_X9, A64_CS, 1);
        a64_cbz(a, A64_X9, ok, 1);
        a64_neg(a, A64_X0, A64_X0, 1);
        a64_bind(a, ok);
    }

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

static void stp_q(A64Asm *a, int rt, int rt2, int rn, int byte_off) {
    int imm = byte_off / 16;
    a64_word(a, 0xAD000000u | ((imm & 0x7f) << 15)
             | ((rt2 & 31) << 10) | ((rn & 31) << 5) | (rt & 31));
}
static void ldp_q(A64Asm *a, int rt, int rt2, int rn, int byte_off) {
    int imm = byte_off / 16;
    a64_word(a, 0xAD400000u | ((imm & 0x7f) << 15)
             | ((rt2 & 31) << 10) | ((rn & 31) << 5) | (rt & 31));
}
static void str_q(A64Asm *a, int rt, int rn, int byte_off) {
    int imm = byte_off / 16;
    a64_word(a, 0x3D800000u | ((imm & 0xfff) << 10)
             | ((rn & 31) << 5) | (rt & 31));
}
/* MOV Vd.16B, Vn.16B — full Q, not a scalar FMOV. */
static void fmov_q(A64Asm *a, int rd, int rn) {
    if (rd == rn) return;
    a64_word(a, 0x4EA01C00u | ((unsigned)(rn & 31) << 16)
             | ((unsigned)(rn & 31) << 5) | (unsigned)(rd & 31));
}
static void ldr_q(A64Asm *a, int rt, int rn, int byte_off) {
    int imm = byte_off / 16;
    a64_word(a, 0x3DC00000u | ((imm & 0xfff) << 10)
             | ((rn & 31) << 5) | (rt & 31));
}

/* imm5 for UMOV/INS: index shifted above the size marker bit. */
static int elem_imm5(int esz, int index) {
    int shift = esz == 1 ? 1 : esz == 2 ? 2 : esz == 4 ? 3 : 4;
    int marker = esz == 1 ? 1 : esz == 2 ? 2 : esz == 4 ? 4 : 8;
    return (index << shift) | marker;
}

static void umov_elem(A64Asm *a, int rd, int vn, int esz, int index) {
    int imm5 = elem_imm5(esz, index);
    uint32_t base = (esz == 8) ? 0x4E003C00u : 0x0E003C00u;
    a64_word(a, base | ((uint32_t)imm5 << 16) | ((uint32_t)(vn & 31) << 5)
                  | (uint32_t)(rd & 31));
}

static void ins_elem(A64Asm *a, int vd, int rn, int esz, int index) {
    int imm5 = elem_imm5(esz, index);
    a64_word(a, 0x4E001C00u | ((uint32_t)imm5 << 16) | ((uint32_t)(rn & 31) << 5)
                            | (uint32_t)(vd & 31));
}

/* dst = a op b, all three are pointers to 16-byte vectors.
 * imm is the element size, width is the vector size, is_float selects
 * NEON scalar-FP rather than integer.  v30/v31 are the only scratches. */
static void emit_vec(C64 *c, const IRInst *s) {
    A64Asm *a = c->as;
    int esz = (int)s->imm;
    if (s->width != 16 || (esz != 1 && esz != 2 && esz != 4 && esz != 8)) {
        c64_die(c, s, "vector wider than 16");
        return;
    }
    int pa = load_ptrv(c, s->a, -1);
    ldr_q(a, A64_V30, pa, 0);
    int pb = load_ptrv(c, s->b, -1);
    ldr_q(a, A64_V31, pb, 0);

    int sz = esz == 1 ? 0 : esz == 2 ? 1 : esz == 4 ? 2 : 3;
    uint32_t base = 0;
    int lane = 0;
    if (s->is_float) {
        if (esz != 4 && esz != 8) {
            c64_die(c, s, "vector float element");
            return;
        }
        uint32_t bit = (esz == 8) ? (1u << 22) : 0;
        switch (s->op) {
        case IR_VADD: base = 0x4E20D400u | bit; break;
        case IR_VSUB: base = 0x4EA0D400u | bit; break;
        case IR_VMUL: base = 0x6E20DC00u | bit; break;
        case IR_VDIV: base = 0x6E20FC00u | bit; break;
        case IR_VBAND: base = 0x4E201C00u; break;
        case IR_VBOR:  base = 0x4EA01C00u; break;
        case IR_VBXOR: base = 0x6E201C00u; break;
        default: c64_die(c, s, "vector op"); return;
        }
    } else {
        switch (s->op) {
        case IR_VADD: base = 0x4E208400u | ((uint32_t)sz << 22); break;
        case IR_VSUB: base = 0x6E208400u | ((uint32_t)sz << 22); break;
        case IR_VBAND: base = 0x4E201C00u; break;
        case IR_VBOR:  base = 0x4EA01C00u; break;
        case IR_VBXOR: base = 0x6E201C00u; break;
        case IR_VMUL:
            /* NEON has no 8-bit or 64-bit integer MUL.  Those lanes go
             * through scalar MUL; the low esz bits match either signedness. */
            if (esz != 2 && esz != 4) {
                lane = 1;
                break;
            }
            base = 0x4E209C00u | ((uint32_t)sz << 22);
            break;
        default:
            c64_die(c, s, "vector integer divide");
            return;
        }
    }
    if (lane) {
        int n = 16 / esz;
        for (int i = 0; i < n; i++) {
            umov_elem(a, SCR0, A64_V30, esz, i);
            umov_elem(a, SCR1, A64_V31, esz, i);
            a64_mul(a, SCR0, SCR0, SCR1, esz == 8);
            ins_elem(a, A64_V30, SCR0, esz, i);
        }
    } else {
        a64_word(a, base | ((uint32_t)A64_V31 << 16) | ((uint32_t)A64_V30 << 5)
                      | (uint32_t)A64_V30);
    }
    int pd = load_ptrv(c, s->dst, -1);
    str_q(a, A64_V30, pd, 0);
}

static void emit_function(C64 *c, int fi) {
    const IRFunction *fn = c->fn;
    A64Asm *a = c->as;
    a64_bind(a, c->fn_label[fi]);

    const RAResult *ra = (const RAResult *)fn->ra;
    c->ra = ra;
    c->ra_fp = (const RAResult *)fn->ra_xmm;
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

    /* GP spill slots, placed below the pinned area.  SIMD spills
     * follow, 16 bytes each and 16-aligned. */
    int spill_area = ra ? ra->stack_size : 0;
    if (ra && ra->num_spill_slots > 0) {
        c->spill_off = xmalloc((size_t)ra->num_spill_slots * sizeof(int));
        for (int s2 = 0; s2 < ra->num_spill_slots; s2++)
            c->spill_off[s2] = -(pinned + (s2 + 1) * 8);
    }
    int fp_bytes = 0;
    c->fp_spill_off = NULL;
    if (c->ra_fp && c->ra_fp->num_spill_slots > 0) {
        int base = pinned + spill_area;
        if (base % 16) base += 16 - (base % 16);
        int nfp = c->ra_fp->num_spill_slots;
        c->fp_spill_off = xmalloc((size_t)nfp * sizeof(int));
        for (int s2 = 0; s2 < nfp; s2++)
            c->fp_spill_off[s2] = -(base + (s2 + 1) * 16);
        fp_bytes = (base - pinned - spill_area) + nfp * 16;
    }

    /* Outgoing stack-argument area (max over calls in this function). */
    int call_area = 0;
    for (size_t j = 0; j < fn->insts.len; j++) {
        const IRInst *s = &fn->insts.data[j];
        if (s->op == IR_CALL) {
            int bytes = outgoing_stack_bytes(c, s);
            if (bytes > call_area) call_area = bytes;
        }
    }
    if (call_area % 16) call_area += 16 - (call_area % 16);
    c->call_area = call_area;

    int locals = pinned + spill_area + fp_bytes + call_area;
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
    /* v8..v15 are callee-saved (the full 128 bits). */
    int vcs[8], nvcs = 0;
    if (c->ra_fp)
        for (int v = 0; v < nv; v++) {
            int hw = c->ra_fp->reg[v];
            if (hw < A64_V8 || hw > A64_V15) continue;
            int present = 0;
            for (int k = 0; k < nvcs; k++) if (vcs[k] == hw) present = 1;
            if (!present) vcs[nvcs++] = hw;
        }
    for (int i = 0; i < nvcs; i++)
        for (int j = i + 1; j < nvcs; j++)
            if (vcs[j] < vcs[i]) { int t = vcs[i]; vcs[i] = vcs[j]; vcs[j] = t; }
    int vcs_pairs = (nvcs + 1) / 2;
    int vcs_area = vcs_pairs * 32;
    if (nvcs & 1) vcs_area -= 16; /* a lone Q is 16 bytes, not a 32-byte pair */
    int save_total = 16 + cs_area + vcs_area;
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
    {
        int qbase = 16 + cs_area;
        for (int p = 0; p < nvcs / 2; p++)
            stp_q(a, vcs[2 * p], vcs[2 * p + 1], A64_FP, qbase + 32 * p);
        if (nvcs & 1)
            str_q(a, vcs[nvcs - 1], A64_FP, qbase + 32 * (nvcs / 2));
    }
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
                    int src_vec;                /* >=0: incoming V register */
                    int dst_reg; int dst_mem;   /* -1, or fp spill offset */
                    int is64; int done; } PMove;
    PMove *mv = xmalloc((size_t)(nparams > 0 ? nparams : 1) * sizeof(PMove));
    typedef struct { int src; int dst; int spill; int isd; int stack; int done; } FIn;
    FIn fin[16];
    int nfin = 0;
    int gp_idx = 0, fp_idx = 0, stack_arg_idx = 0;
    for (int p = 0; p < nparams; p++) {
        const IRInst *s = &fn->insts.data[p];
        mv[p].src_vec = -1;
        mv[p].src_reg = -1;
        mv[p].src_mem = -1;
        if (s->force_stack && s->alloca_bytes > 8)
            c64_die(c, s, "aggregate stack blob");
        int is_sret = fn->sret_value >= 0 && s->dst == fn->sret_value;
        if (vec16_val(c, s->dst)) {
            int from_stack = s->force_stack || fp_idx >= 8;
            int srcv;
            if (from_stack) {
                int byte = 8 * stack_arg_idx;
                if (byte & 15) {
                    stack_arg_idx++;
                    byte += 8;
                }
                srcv = save_total + byte;
                stack_arg_idx += 2;
            } else {
                srcv = fp_idx++;
            }
            mv[p].done = 1;
            mv[p].src_reg = -1;
            mv[p].src_vec = -1;
            mv[p].dst_reg = -1;
            mv[p].src_mem = -1;
            if (param_is_used(fn, s->dst)) {
                int dh = fp_home(c, s->dst);
                fin[nfin].src = srcv;
                fin[nfin].dst = dh;
                fin[nfin].spill = dh < 0 ? fp_spill(c, s->dst) : 0;
                fin[nfin].isd = 2;
                fin[nfin].stack = from_stack;
                fin[nfin].done = 0;
                nfin++;
            }
            continue;
        }
        if (scalar_fp_val(c, s->dst)) {
            int isd = s->width == 8;
            int from_stack = s->force_stack || fp_idx >= 8;
            int srcv = from_stack ? save_total + 8 * stack_arg_idx++ : fp_idx++;
            mv[p].done = 1;
            mv[p].src_reg = -1;
            mv[p].src_vec = -1;
            mv[p].dst_reg = -1;
            mv[p].src_mem = -1;
            if (param_is_used(fn, s->dst)) {
                int dh = fp_home(c, s->dst);
                fin[nfin].src = srcv;
                fin[nfin].dst = dh;
                fin[nfin].spill = dh < 0 ? fp_spill(c, s->dst) : 0;
                fin[nfin].isd = isd;
                fin[nfin].stack = from_stack;
                fin[nfin].done = 0;
                nfin++;
            }
            continue;
        }
        if (s->is_float && !s->force_stack) {
            /* HFA eightbyte: bits arrive in the next V register. */
            mv[p].src_vec = fp_idx++;
        } else if (is_sret) {
            /* Darwin indirect result pointer.  Does not consume x0. */
            mv[p].src_reg = A64_X8;
        } else if (s->force_stack || gp_idx >= 8) {
            /* Above the saved frame (Darwin ABI: no pushed return addr). */
            mv[p].src_mem = save_total + 8 * stack_arg_idx++;
        } else {
            mv[p].src_reg = A64_X0 + gp_idx++;
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
    /* Incoming V-reg parameters can be homed in v0..v7, so place them
     * before the GP shuffle and break cycles through v31. */
    {
        int nleft = nfin;
        while (nleft > 0) {
            int picked = -1;
            for (int i = 0; i < nfin; i++) {
                if (fin[i].done) continue;
                int blocked = 0;
                if (fin[i].dst >= 0)
                    for (int j = 0; j < nfin; j++) {
                        if (fin[j].done || j == i || fin[j].stack) continue;
                        if (fin[j].src == fin[i].dst) { blocked = 1; break; }
                    }
                if (!blocked) { picked = i; break; }
            }
            if (picked < 0) {
                for (int i = 0; i < nfin; i++)
                    if (!fin[i].done && !fin[i].stack) {
                        if (fin[i].isd == 2) fmov_q(a, FSCR, fin[i].src);
                        else a64_fmov_reg(a, FSCR, fin[i].src, fin[i].isd);
                        fin[i].src = FSCR;
                        break;
                    }
                continue;
            }
            FIn *m = &fin[picked];
            int tmp = m->dst >= 0 ? m->dst : A64_V30;
            if (m->stack) {
                emit_fp_addr(c, SCR0, m->src);
                if (m->isd == 2) ldr_q(a, tmp, SCR0, 0);
                else if (m->isd) a64_ldr_d(a, tmp, SCR0, 0);
                else a64_ldr_s(a, tmp, SCR0, 0);
            } else if (m->isd == 2) {
                fmov_q(a, tmp, m->src);
            } else if (tmp != m->src) {
                a64_fmov_reg(a, tmp, m->src, m->isd);
            }
            if (m->dst < 0) {
                if (m->isd == 2) {
                    emit_fp_addr(c, SCR1, m->spill);
                    str_q(a, tmp, SCR1, 0);
                } else {
                    fp_frame(c, tmp, m->spill, m->isd, 1);
                }
            }
            m->done = 1;
            nleft--;
        }
    }
    int remaining = 0;
    for (int p = 0; p < nparams; p++) remaining += !mv[p].done;
#define PM_EMIT(p) do {                                                      \
        PMove *_m = &mv[(p)];                                                \
        if (_m->src_vec >= 0) {                                              \
            int _tmp = _m->dst_reg >= 0 ? _m->dst_reg : SCR1;                \
            a64_fmov_gp(a, _tmp, _m->src_vec, 0, _m->is64);                  \
            if (_m->dst_reg < 0)                                             \
                frame_store(c, _tmp, _m->dst_mem, 8);                        \
        } else if (_m->dst_reg >= 0) {                                       \
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
    c->vararg_off = save_total + 8 * stack_arg_idx;

    /* ---- Body ---- */
    int epilog = a64_new_label(a);
    for (size_t j = nparams; j < fn->insts.len; j++) {
        const IRInst *s = &fn->insts.data[j];
        switch (s->op) {
        case IR_CONST: {
            if (scalar_fp_val(c, s->dst) || s->is_float) {
                if (s->width != 4 && s->width != 8)
                    c64_die(c, s, "long double");
                int isd = s->width == 8;
                int h = fp_home(c, s->dst);
                int tmp = h >= 0 ? h : FSCR;
                emit_mov_imm_w(a, SCR0, s->float_imm, isd);
                a64_fmov_gp(a, tmp, SCR0, 1, isd);
                if (h < 0) commit_fp(c, s->dst, tmp);
                break;
            }
            int d = dst_reg(c, s->dst);
            emit_mov_imm_w(a, d, s->imm, s->width == 8);
            commit(c, s->dst, d);
            break;
        }
        case IR_COPY:
        case IR_TRUNC: {
            if (vec16_val(c, s->dst) || vec16_val(c, s->a)) {
                int r = load_q(c, s->a, -1);
                commit_q(c, s->dst, r);
                break;
            }
            if (scalar_fp_val(c, s->dst) || scalar_fp_val(c, s->a)) {
                int r = load_fp(c, s->a, -1);
                commit_fp(c, s->dst, r);
                break;
            }
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
            if (scalar_fp_val(c, s->a) || scalar_fp_val(c, s->dst)) {
                int r = load_fp(c, s->a, -1);
                int h = fp_home(c, s->dst);
                int d = h >= 0 ? h : FSCR;
                int isd = vw(c, s->a) == 8;
                a64_word(a, (isd ? 0x1E614000u : 0x1E214000u)
                          | ((r & 31) << 5) | (d & 31));
                if (h < 0) commit_fp(c, s->dst, d);
                break;
            }
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

        case IR_VADD: case IR_VSUB: case IR_VMUL: case IR_VDIV:
        case IR_VBAND: case IR_VBOR: case IR_VBXOR:
            emit_vec(c, s);
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
            if (vec16_val(c, s->b)) {
                int v = load_q(c, s->b, -1);
                int p = load_ptrv(c, s->a, SCR0);
                str_q(a, v, p, 0);
                break;
            }
            if (scalar_fp_val(c, s->b)) {
                /* Float materialization uses x16; load the address after it. */
                int v = load_fp(c, s->b, -1);
                int p = load_ptrv(c, s->a, SCR0);
                if (vw(c, s->b) == 8) a64_str_d(a, v, p, 0);
                else a64_str_s(a, v, p, 0);
                break;
            }
            int p = load_ptrv(c, s->a, -1);
            int v = load_op(c, s->b, p);
            emit_store(c, v, p, s->width);
            break;
        }
        case IR_LOAD_PTR: {
            if (vec16_val(c, s->dst)) {
                int p = load_ptrv(c, s->a, -1);
                int h = fp_home(c, s->dst);
                int d = h >= 0 ? h : FSCR;
                ldr_q(a, d, p, 0);
                if (h < 0) commit_q(c, s->dst, d);
                break;
            }
            if (scalar_fp_val(c, s->dst)) {
                int p = load_ptrv(c, s->a, -1);
                int h = fp_home(c, s->dst);
                int d = h >= 0 ? h : FSCR;
                if (vw(c, s->dst) == 8) a64_ldr_d(a, d, p, 0);
                else a64_ldr_s(a, d, p, 0);
                if (h < 0) commit_fp(c, s->dst, d);
                break;
            }
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
            int lab = vlabels_get(c, (int)s->imm);
            if (emit_object_mode()) {
                const char *nm = note_label_sym(c, (int)s->imm, lab);
                note_page_reloc(c, (uint32_t)a->code.len, -1, nm);
                a64_word(a, 0x90000000u | (uint32_t)(d & 31));
                a64_word(a, 0x91000000u | ((uint32_t)(d & 31) << 5)
                                        | (uint32_t)(d & 31));
            } else {
                a64_adrp_add_label(a, d, lab);
            }
            commit(c, s->dst, d);
            break;
        }

        case IR_CALL:
            emit_call(c, s);
            break;

        case IR_RETURN: {
            if (s->a >= 0 && vec16_val(c, s->a)) {
                int r = load_q(c, s->a, -1);
                fmov_q(a, A64_V0, r);
                a64_b(a, epilog);
                break;
            }
            if (s->a >= 0 && scalar_fp_val(c, s->a)) {
                int r = load_fp(c, s->a, -1);
                int isd = vw(c, s->a) == 8;
                if (r != A64_V0) a64_fmov_reg(a, A64_V0, r, isd);
                a64_b(a, epilog);
                break;
            }
            if (s->align16 == A64_MARK_HFA && s->a >= 0) {
                int r0 = load_op(c, s->a, -1);
                int r1 = s->b >= 0 ? load_op(c, s->b, r0) : -1;
                a64_fmov_gp(a, A64_V0, r0, 1, vw(c, s->a) == 8);
                if (r1 >= 0)
                    a64_fmov_gp(a, A64_V1, r1, 1, vw(c, s->b) == 8);
            } else if (s->a >= 0 && s->b >= 0) {
                int r0 = load_op(c, s->a, -1);
                int r1 = load_op(c, s->b, r0);
                int wide = vw(c, s->a) == 8;
                if (r0 == A64_X1) {
                    a64_mov_reg(a, SCR0, r0, wide);
                    if (r1 != A64_X1) a64_mov_reg(a, A64_X1, r1, vw(c, s->b) == 8);
                    a64_mov_reg(a, A64_X0, SCR0, wide);
                } else {
                    if (r1 != A64_X1) a64_mov_reg(a, A64_X1, r1, vw(c, s->b) == 8);
                    if (r0 != A64_X0) a64_mov_reg(a, A64_X0, r0, wide);
                }
            } else if (s->a >= 0) {
                int r = load_op(c, s->a, -1);
                int wide = vw(c, s->a) == 8 || fn->ret_width >= 8;
                if (r != A64_X0)
                    a64_mov_reg(a, A64_X0, r, wide);
            }
            a64_b(a, epilog);
            break;
        }

        case IR_FRAME_ADDR: {
            /* [fp] is the caller's frame pointer, same chain as rbp. */
            int d = dst_reg(c, s->dst);
            a64_mov_reg(a, d, A64_FP, 1);
            int level = (int)s->imm;
            if (level < 0) level = 0;
            for (int i = 0; i < level; i++)
                a64_ldr64(a, d, d, 0);
            commit(c, s->dst, d);
            break;
        }
        case IR_RETURN_ADDR: {
            int d = dst_reg(c, s->dst);
            a64_mov_reg(a, d, A64_FP, 1);
            int level = (int)s->imm;
            if (level < 0) level = 0;
            for (int i = 0; i < level; i++)
                a64_ldr64(a, d, d, 0);
            a64_ldr64(a, d, d, 8);
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
        case IR_LONGJMP: {
            /* buf[0] = fp, buf[1] = resume pc, buf[2] = sp, matching the
             * x86 lowering.  Read all three before writing fp or sp.
             * x0/x1 are not allocatable; a spilled pointer arrives in x16. */
            int buf = load_ptrv(c, s->a, -1);
            int pc = A64_X0;
            int spv = A64_X1;
            if (buf == pc) pc = SCR0;
            if (buf == spv) spv = SCR1;
            a64_ldr64(a, pc, buf, 8);
            a64_ldr64(a, spv, buf, 16);
            a64_ldr64(a, A64_FP, buf, 0);
            mov_sp_like(a, A64_SP, spv);
            a64_br_reg(a, pc);
            break;
        }
        case IR_DYN_ALLOCA: {
            int r = load_op(c, s->a, -1);
            if (s->b >= 0) {
                /* alloca_with_align: alignment arrives in bits.  The ABI
                 * still requires SP to stay 16-byte aligned, so anything
                 * finer than that is raised.  For a power of two,
                 * ~(align-1) is just -align. */
                if (r == SCR1) {
                    a64_mov_reg(a, SCR0, r, 1);
                    r = SCR0;
                }
                int al = load_op(c, s->b, r);
                if (al == r) {
                    a64_mov_reg(a, SCR1, al, 1);
                    al = SCR1;
                }
                a64_lsr_imm(a, SCR1, al, 3, 1);
                int Lbig = a64_new_label(a);
                a64_cmp_imm12(a, SCR1, 16, 0, 1);
                a64_bcond(a, A64_CS, Lbig);
                a64_movz(a, SCR1, 16, 0, 1);
                a64_bind(a, Lbig);
                a64_neg(a, SCR1, SCR1, 1);
                mov_sp_like(a, A64_X0, A64_SP);
                a64_sub_reg(a, A64_X0, A64_X0, r, A64_LSL, 0, 1, 0);
                a64_and_reg(a, A64_X0, A64_X0, SCR1, 1);
                mov_sp_like(a, A64_SP, A64_X0);
                int d = dst_reg(c, s->dst);
                if (d != A64_X0) a64_mov_reg(a, d, A64_X0, 1);
                commit(c, s->dst, d);
                break;
            }
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

        case IR_FADD: case IR_FSUB: case IR_FMUL: case IR_FDIV: {
            int isd = s->width == 8;
            int ra = load_fp(c, s->a, -1);
            int rb = load_fp(c, s->b, ra);
            int h = fp_home(c, s->dst);
            int d = h >= 0 ? h : FSCR;
            if (s->op == IR_FADD) a64_fadd(a, d, ra, rb, isd);
            else if (s->op == IR_FSUB) a64_fsub(a, d, ra, rb, isd);
            else if (s->op == IR_FMUL) a64_fmul(a, d, ra, rb, isd);
            else a64_fdiv(a, d, ra, rb, isd);
            if (h < 0) commit_fp(c, s->dst, d);
            break;
        }
        case IR_FCMP: {
            int isd = vw(c, s->a) == 8;
            int ra = load_fp(c, s->a, -1);
            int rb = load_fp(c, s->b, ra);
            a64_fcmp(a, ra, rb, isd);
            int d = dst_reg(c, s->dst);
            /* FCMP sets NZCV.  Ordered LE is EQ|LT; the single LE
             * condition is true for NaN, which C does not want. */
            if (s->is_unsigned == 1) {
                int tmp = (d == SCR0) ? SCR1 : SCR0;
                a64_cset(a, d, A64_EQ, 0);
                a64_cset(a, tmp, A64_MI, 0);
                a64_or_reg(a, d, d, tmp, 0);
            } else {
                int cond = A64_MI;
                switch (s->is_unsigned) {
                case 0: cond = A64_MI; break;
                case 2: cond = A64_GT; break;
                case 3: cond = A64_GE; break;
                case 4: cond = A64_EQ; break;
                default: cond = A64_NE; break;
                }
                a64_cset(a, d, cond, 0);
            }
            commit(c, s->dst, d);
            break;
        }
        case IR_SITOFP: {
            int src_w = s->imm ? (int)s->imm : 4;
            int src_u = s->is_unsigned;
            int isd = s->width == 8;
            int gp = load_op(c, s->a, -1);
            if (src_w < 4) {
                int tmp = (gp == SCR0) ? SCR1 : SCR0;
                if (src_u) a64_uxt(a, tmp, gp, (unsigned)src_w, 0);
                else a64_sxt(a, tmp, gp, (unsigned)src_w, 0);
                gp = tmp;
                src_w = 4;
            }
            int from64 = src_w >= 8;
            uint32_t base;
            if (isd && from64) base = src_u ? 0x9E630000u : 0x9E620000u;
            else if (isd) base = src_u ? 0x1E630000u : 0x1E620000u;
            else if (from64) base = src_u ? 0x9E230000u : 0x9E220000u;
            else base = src_u ? 0x1E230000u : 0x1E220000u;
            int h = fp_home(c, s->dst);
            int d = h >= 0 ? h : FSCR;
            a64_word(a, base | ((gp & 31) << 5) | (d & 31));
            if (h < 0) commit_fp(c, s->dst, d);
            break;
        }
        case IR_FPTOSI: {
            int isd = vw(c, s->a) == 8;
            int fs = load_fp(c, s->a, -1);
            int to64 = s->width >= 8;
            int un = s->is_unsigned;
            uint32_t base;
            if (isd && to64) base = un ? 0x9E790000u : 0x9E780000u;
            else if (isd) base = un ? 0x1E790000u : 0x1E780000u;
            else if (to64) base = un ? 0x9E390000u : 0x9E380000u;
            else base = un ? 0x1E390000u : 0x1E380000u;
            int d = dst_reg(c, s->dst);
            a64_word(a, base | ((fs & 31) << 5) | (d & 31));
            if (s->width < 4) {
                /* Keep the low bytes.  The convert wrote a W or X. */
                if (s->width == 1) a64_uxt(a, d, d, 1, 0);
                else if (s->width == 2) a64_uxt(a, d, d, 2, 0);
            }
            commit(c, s->dst, d);
            break;
        }
        case IR_FPEXT: case IR_FPTRUNC: {
            int src_d = vw(c, s->a) == 8;
            int dst_d = s->width == 8;
            int r = load_fp(c, s->a, -1);
            int h = fp_home(c, s->dst);
            int d = h >= 0 ? h : FSCR;
            if (src_d == dst_d) {
                if (d != r) a64_fmov_reg(a, d, r, dst_d);
            } else {
                uint32_t base = dst_d ? 0x1E22C000u : 0x1E624000u;
                a64_word(a, base | ((r & 31) << 5) | (d & 31));
            }
            if (h < 0) commit_fp(c, s->dst, d);
            break;
        }
        case IR_LOAD: {
            /* At -O0 a is a pinned-alloca slot (ternary/short-circuit
             * temps).  The unpinned SSA form (-O1) is a plain copy. */
            if (scalar_fp_val(c, s->dst)) {
                int off = (s->a >= 0 && s->a < nv) ? c->alloca_off[s->a] : 0;
                int h = fp_home(c, s->dst);
                int d = h >= 0 ? h : FSCR;
                if (off != 0) {
                    fp_frame(c, d, off, vw(c, s->dst) == 8, 0);
                } else {
                    int r = load_fp(c, s->a, d == FSCR ? -1 : d);
                    if (d != r) a64_fmov_reg(a, d, r, vw(c, s->dst) == 8);
                }
                if (h < 0) commit_fp(c, s->dst, d);
                break;
            }
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
            if (scalar_fp_val(c, s->b)) {
                int off = (s->a >= 0 && s->a < nv) ? c->alloca_off[s->a] : 0;
                int v = load_fp(c, s->b, -1);
                if (off != 0) fp_frame(c, v, off, vw(c, s->b) == 8, 1);
                else commit_fp(c, s->a, v);
                break;
            }
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
            /* A single TU executable has nowhere to find this symbol.
             * An object file records a page reloc and the linker resolves it. */
            if (gi < 0 && !emit_object_mode()) {
                c64_die(c, s, "external global variable");
                break;
            }
            int d = dst_reg(c, s->dst);
            /* adrp d, page ; add d, d, #pageoff — both words patched in
             * codegen64 once __const/__data/__bss placement is final. */
            note_page_reloc(c, (uint32_t)a->code.len, gi,
                            gi < 0 ? s->call_name : NULL);
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
            int missing = find_function(c->ir, s->call_name, &fi) != 0;
            if (missing && !emit_object_mode()) {
                c64_die(c, s, "external function address");
                break;
            }
            int d = dst_reg(c, s->dst);
            /* Object text is concatenated after an entry stub, so a baked
             * ADRP would miss the real page.  Record a reloc instead. */
            if (emit_object_mode()) {
                note_page_reloc(c, (uint32_t)a->code.len, -1, s->call_name);
                a64_word(a, 0x90000000u | (uint32_t)(d & 31));
                a64_word(a, 0x91000000u | ((uint32_t)(d & 31) << 5)
                                        | (uint32_t)(d & 31));
            } else {
                a64_adrp_add_label(a, d, c->fn_label[fi]);
            }
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
    {
        int qbase = 16 + cs_area;
        for (int p = 0; p < nvcs / 2; p++)
            ldp_q(a, vcs[2 * p], vcs[2 * p + 1], A64_FP, qbase + 32 * p);
        if (nvcs & 1)
            ldr_q(a, vcs[nvcs - 1], A64_FP, qbase + 32 * (nvcs / 2));
    }
    a64_ldp64(a, A64_FP, A64_LR, A64_SP, save_total, A64_PAIR_POST);
    a64_ret(a, A64_LR);

    free(c->def);
    free(c->alloca_off);
    free(c->spill_off);
    free(c->fp_spill_off);
    free(c->vlabels);
    c->ra = NULL;
}

/* Unsigned 128/128 restoring division.  Same contract as
 * runtime/int128.c __fakecc_udivmodti4: x0..x3 are the two 128-bit
 * values (lo, hi), x4..x7 are pointers to q_lo, q_hi, r_lo, r_hi.
 * A zero divisor yields quotient 0 and remainder = numerator. */
static void emit_udivmodti4(A64Asm *a, int label) {
    a64_bind(a, label);
    int Lzero = a64_new_label(a);
    int Ltop  = a64_new_label(a);
    int Lge   = a64_new_label(a);
    int Lbit  = a64_new_label(a);
    int Lsub  = a64_new_label(a);
    int Lqhi  = a64_new_label(a);
    int Lnext = a64_new_label(a);
    int Lstore = a64_new_label(a);

    a64_or_reg(a, A64_X14, A64_X2, A64_X3, 1);
    a64_cbz(a, A64_X14, Lzero, 1);

    a64_movz(a, A64_X9,  0, 0, 1);          /* q_lo */
    a64_movz(a, A64_X10, 0, 0, 1);          /* q_hi */
    a64_movz(a, A64_X11, 0, 0, 1);          /* r_lo */
    a64_movz(a, A64_X12, 0, 0, 1);          /* r_hi */
    a64_movz(a, A64_X13, 127, 0, 1);        /* bit index */

    a64_bind(a, Ltop);
    a64_lsr_imm(a, SCR0, A64_X12, 63, 1);   /* carry out of r_hi */
    a64_lsr_imm(a, A64_X14, A64_X11, 63, 1);
    a64_lsl_imm(a, A64_X12, A64_X12, 1, 1);
    a64_or_reg(a, A64_X12, A64_X12, A64_X14, 1);
    a64_lsl_imm(a, A64_X11, A64_X11, 1, 1);

    a64_cmp_imm12(a, A64_X13, 64, 0, 1);
    a64_bcond(a, A64_GE, Lge);
    a64_lsrv(a, A64_X14, A64_X0, A64_X13, 1);
    a64_b(a, Lbit);
    a64_bind(a, Lge);
    a64_sub_imm12(a, A64_X15, A64_X13, 64, 0, 1, 0);
    a64_lsrv(a, A64_X14, A64_X1, A64_X15, 1);
    a64_bind(a, Lbit);
    a64_movz(a, A64_X15, 1, 0, 1);
    a64_and_reg(a, A64_X14, A64_X14, A64_X15, 1);
    a64_or_reg(a, A64_X11, A64_X11, A64_X14, 1);

    a64_cbnz(a, SCR0, Lsub, 1);
    a64_cmp_reg(a, A64_X12, A64_X3, 1);
    a64_bcond(a, A64_HI, Lsub);
    a64_bcond(a, A64_CC, Lnext);
    a64_cmp_reg(a, A64_X11, A64_X2, 1);
    a64_bcond(a, A64_CC, Lnext);

    a64_bind(a, Lsub);
    a64_cmp_reg(a, A64_X11, A64_X2, 1);
    a64_cset(a, A64_X14, A64_CC, 1);        /* borrow */
    a64_sub_reg(a, A64_X11, A64_X11, A64_X2, A64_LSL, 0, 1, 0);
    a64_sub_reg(a, A64_X12, A64_X12, A64_X3, A64_LSL, 0, 1, 0);
    a64_sub_reg(a, A64_X12, A64_X12, A64_X14, A64_LSL, 0, 1, 0);
    a64_cmp_imm12(a, A64_X13, 64, 0, 1);
    a64_bcond(a, A64_GE, Lqhi);
    a64_movz(a, A64_X15, 1, 0, 1);
    a64_lslv(a, A64_X15, A64_X15, A64_X13, 1);
    a64_or_reg(a, A64_X9, A64_X9, A64_X15, 1);
    a64_b(a, Lnext);
    a64_bind(a, Lqhi);
    a64_sub_imm12(a, A64_X15, A64_X13, 64, 0, 1, 0);
    a64_movz(a, A64_X14, 1, 0, 1);
    a64_lslv(a, A64_X14, A64_X14, A64_X15, 1);
    a64_or_reg(a, A64_X10, A64_X10, A64_X14, 1);

    a64_bind(a, Lnext);
    a64_sub_imm12(a, A64_X13, A64_X13, 1, 0, 1, 0);
    a64_cmp_imm12(a, A64_X13, 0, 0, 1);
    a64_bcond(a, A64_GE, Ltop);
    a64_b(a, Lstore);

    a64_bind(a, Lzero);
    a64_movz(a, A64_X9,  0, 0, 1);
    a64_movz(a, A64_X10, 0, 0, 1);
    a64_mov_reg(a, A64_X11, A64_X0, 1);
    a64_mov_reg(a, A64_X12, A64_X1, 1);

    a64_bind(a, Lstore);
    a64_str64(a, A64_X9,  A64_X4, 0);
    a64_str64(a, A64_X10, A64_X5, 0);
    a64_str64(a, A64_X11, A64_X6, 0);
    a64_str64(a, A64_X12, A64_X7, 0);
    a64_ret(a, A64_LR);
}

/* ── Module entry point ──────────────────────────────────────────── */

void codegen64(const IRModule *ir, EmitModule *out, int want_debug) {
    (void)want_debug;  /* DWARF emission lands in T20. */

    int main_id = -1;
    if (!emit_object_mode() && find_function(ir, "main", &main_id) != 0)
        die_at("<arm64>", 0, 0, "no 'main' function found");

    A64Asm a;
    a64_init(&a);

    C64 c;
    memset(&c, 0, sizeof c);
    c.as = &a;
    c.ir = ir;
    c.udiv_label = -1;

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
        } else if (emit_object_mode() && !g->is_static) {
            /* Tentative definition.  A later real definition in another
             * file must be able to win, so the object carries a common
             * symbol instead of its own BSS bytes. */
            c.gsect[gi] = G_COMMON;
            c.goff[gi] = al;
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

    /* LC_MAIN entry stub.  Object files (-c) are just the functions.
     * dyld hands us x0=argc, x1=argv, x2=envp; keep
     * them in callee-saved registers across the constructor calls, call
     * main, run destructors in reverse, then Darwin exit(main's value).
     * The stub owns x19..x22 outright (process entry has no caller whose
     * values matter) but saves them anyway to keep the frame ABI-clean. */
    if (!emit_object_mode()) {
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
    }
    free(ctors);
    free(dtors);

    for (size_t i = 0; i < ir->functions.len; i++) {
        c.fn = &ir->functions.data[i];
        emit_function(&c, (int)i);
    }
    if (c.udiv_label >= 0)
        emit_udivmodti4(&a, c.udiv_label);

    /* The code buffer lands after the Mach-O header pad; ADRP page fixups
     * must compute against the runtime file/VA offset. */
    a64_set_base(&a, emit_object_mode() ? 0 : macho_text_offset());
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
     * known.  An object file cannot bake in the executable layout. */
    if (!emit_object_mode() && (c.ngfix || c.npfix)) {
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
    if (emit_object_mode()) {
        for (size_t i = 0; i < ir->functions.len; i++) {
            if (!a.labels[c.fn_label[i]].bound) continue;
            const IRFunction *fn = &ir->functions.data[i];
            size_t start = a.labels[c.fn_label[i]].pos;
            size_t end = out->text.len;
            if (i + 1 < ir->functions.len &&
                a.labels[c.fn_label[i + 1]].bound)
                end = a.labels[c.fn_label[i + 1]].pos;
            else if (c.udiv_label >= 0 && a.labels[c.udiv_label].bound)
                end = a.labels[c.udiv_label].pos;
            emit_module_add_symbol(out, fn->name,
                                   fn->is_static ? 0 : 1, 2 /* STT_FUNC */,
                                   (uint16_t)SECT_TEXT, start, end - start);
            if (fn->is_constructor || fn->is_destructor) {
                int fsym = emit_module_find_symbol(out, fn->name);
                Buffer *arr = fn->is_constructor ? &out->init_array
                                                 : &out->fini_array;
                int **prio = fn->is_constructor ? &out->init_prio
                                                : &out->fini_prio;
                size_t slot = arr->len;
                uint64_t z = 0;
                buffer_append(arr, (const char *)&z, 8);
                emit_module_add_data_reloc(out, slot, 0 /* UNSIGNED */, fsym, 0);
                out->data_relocs[out->num_data_relocs - 1].shndx =
                    fn->is_constructor ? (uint16_t)SECT_INIT_ARRAY
                                       : (uint16_t)SECT_FINI_ARRAY;
                size_t nslot = arr->len / 8;
                *prio = realloc(*prio, nslot * sizeof(int));
                if (!*prio) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
                (*prio)[nslot - 1] = fn->is_constructor
                    ? (fn->ctor_prio ? fn->ctor_prio : INIT_PRIO_DEFAULT)
                    : (fn->dtor_prio ? fn->dtor_prio : INIT_PRIO_DEFAULT);
            }
        }
        int *gsym = NULL;
        if (ir->globals.len)
            gsym = xmalloc(ir->globals.len * sizeof(int));
        for (size_t gi = 0; gi < ir->globals.len; gi++) {
            const IRGlobal *g = &ir->globals.data[gi];
            if (gsym) gsym[gi] = -1;
            if (!g->name || !c.gsect) continue;
            uint16_t sh = c.gsect[gi] == G_COMMON ? (uint16_t)SHN_COMMON
                        : c.gsect[gi] == G_RO ? (uint16_t)SECT_RODATA
                        : c.gsect[gi] == G_DATA ? (uint16_t)SECT_DATA
                        : (uint16_t)SECT_BSS;
            /* Common: value is the alignment.  A real symbol: value is
             * the offset within its section. */
            gsym[gi] = emit_module_add_symbol(out, g->name,
                                              g->is_static ? 0 : 1,
                                              1 /* STT_OBJECT */, sh,
                                              c.goff[gi], (size_t)g->size);
        }
        for (size_t i = 0; i < c.nlrel; i++) {
            if (emit_module_find_symbol(out, c.lrel[i].name) >= 0) continue;
            int lab = c.lrel[i].a64lab;
            if (lab < 0 || (size_t)lab >= a.nlabels || !a.labels[lab].bound)
                continue;
            emit_module_add_symbol(out, c.lrel[i].name, 0 /* local */,
                                   2 /* STT_FUNC */, (uint16_t)SECT_TEXT,
                                   a.labels[lab].pos, 0);
        }
        /* ADRP + ADD against the global symbol.  The linker fills the
         * page and page-offset immediates (ARM64_RELOC_PAGE21 / PAGEOFF12). */
        for (size_t i = 0; i < c.ngfix; i++) {
            int gi = c.gfix[i].gidx;
            int si = (gsym && gi >= 0) ? gsym[gi] : -1;
            if (si < 0 && c.gfix[i].name) {
                si = emit_module_find_symbol(out, c.gfix[i].name);
                if (si < 0)
                    si = emit_module_add_undefined(out, c.gfix[i].name);
            }
            if (si < 0) {
                die_at("<arm64>", 0, 0,
                       "arm64 object file: global has no symbol");
                continue;
            }
            emit_module_add_reloc(out, c.gfix[i].at, 3 /* PAGE21 */, si, 0);
            emit_module_add_reloc(out, c.gfix[i].at + 4, 4 /* PAGEOFF12 */,
                                  si, 0);
        }
        /* Pointer slots in __data: ARM64_RELOC_UNSIGNED, addend in the
         * eight bytes at the slot. */
        for (size_t i = 0; i < c.npfix; i++) {
            int si = emit_module_find_symbol(out, c.pfix[i].sym);
            if (si < 0)
                si = emit_module_add_undefined(out, c.pfix[i].sym);
            int gi = c.pfix[i].gidx;
            if (!c.gsect || gi < 0 || c.gsect[gi] != G_DATA) {
                die_at("<arm64>", 0, 0,
                       "arm64 object file: pointer initializer is not in __data");
                continue;
            }
            size_t off = c.goff[gi] + (size_t)c.pfix[i].slot_off;
            if (off + 8 <= out->data.len) {
                int64_t add = c.pfix[i].addend;
                memcpy(out->data.data + off, &add, 8);
            }
            emit_module_add_data_reloc(out, off, 0 /* UNSIGNED */, si,
                                       c.pfix[i].addend);
        }
        for (size_t i = 0; i < c.nxcall; i++) {
            int si = emit_module_find_symbol(out, c.xcall[i].name);
            if (si < 0)
                si = emit_module_add_undefined(out, c.xcall[i].name);
            emit_module_add_reloc(out, c.xcall[i].at, 2 /* BRANCH26 */, si, 0);
        }
        free(gsym);
    }
    free(c.gfix);
    free(c.pfix);
    free(c.xcall);
    for (size_t i = 0; i < c.nlrel; i++) free(c.lrel[i].name);
    free(c.lrel);
    free(c.gsect);
    free(c.goff);
    a64_free(&a);
    free(c.fn_label);
}
