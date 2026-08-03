#include "fakecc/ir.h"
#include "fakecc/ast.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* File-scope handles used by lower_expr when it needs to allocate a new
 * anonymous rodata global for a string literal.  Set at the start of
 * ir_generate; cleared at the end. */
IRModule *g_ir_module = NULL;
int g_str_counter = 0;
const StructRegistry *g_ir_structs = NULL;   /* set by ir_generate */

/* Loop-stack: (continue_label, break_label).  Pushed on entry to every
 * loop-lowering block; popped on exit.  ST_BREAK / ST_CONTINUE consult the
 * top entry.  Depth guarded by sema — this stack never underflows. */
typedef struct {
    int cont_label;
    int break_label;
} LoopFrame;
static LoopFrame g_loops[32];
static int g_loop_depth = 0;
static void push_loop(int cont, int brk) {
    g_loops[g_loop_depth].cont_label = cont;
    g_loops[g_loop_depth].break_label = brk;
    g_loop_depth++;
}
static void pop_loop(void) { g_loop_depth--; }

/* ------------------------------------------------------------------ */
/* IRModule lifetime                                                   */
/* ------------------------------------------------------------------ */

void ir_module_init(IRModule *m) {
    m->functions.data = NULL;
    m->functions.len = 0;
    m->functions.cap = 0;
    m->globals.data = NULL;
    m->globals.len = 0;
    m->globals.cap = 0;
}

void ir_module_free(IRModule *m) {
    for (size_t i = 0; i < m->functions.len; i++) {
        free(m->functions.data[i].name);
        for (size_t j = 0; j < m->functions.data[i].insts.len; j++)
            free(m->functions.data[i].insts.data[j].call_name);
        free(m->functions.data[i].insts.data);
        free(m->functions.data[i].value_width);
        free(m->functions.data[i].value_is_unsigned);
        /* Free register allocation result if present. */
        extern void ra_result_free(void *ra);
        if (m->functions.data[i].ra)
            ra_result_free(m->functions.data[i].ra);
    }
    free(m->functions.data);
    for (size_t i = 0; i < m->globals.len; i++) {
        free(m->globals.data[i].name);
        free(m->globals.data[i].init_bytes);
    }
    free(m->globals.data);
    m->functions.data = NULL;
    m->functions.len = 0;
    m->functions.cap = 0;
    m->globals.data = NULL;
    m->globals.len = 0;
    m->globals.cap = 0;
}

static IRGlobal *ir_module_push_global(IRModule *m, const char *name,
                                       int size, char *init_bytes,
                                       int is_readonly, SourceLoc loc) {
    if (m->globals.len >= m->globals.cap) {
        size_t nc = m->globals.cap ? m->globals.cap * 2 : 4;
        m->globals.data = realloc(m->globals.data, nc * sizeof(IRGlobal));
        if (!m->globals.data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->globals.cap = nc;
    }
    IRGlobal *g = &m->globals.data[m->globals.len++];
    g->name = xstrdup(name);
    g->size = size;
    g->init_bytes = init_bytes;   /* takes ownership */
    g->is_readonly = is_readonly;
    g->loc = loc;
    return g;
}

/* ------------------------------------------------------------------ */
/* IRInstArray helpers                                                 */
/* ------------------------------------------------------------------ */

static void ir_inst_array_push(IRInstArray *a, IRInst inst) {
    if (a->len >= a->cap) {
        size_t new_cap = a->cap ? a->cap * 2 : 8;
        a->data = realloc(a->data, new_cap * sizeof(IRInst));
        if (!a->data) {
            fprintf(stderr, "fakecc: out of memory\n");
            exit(1);
        }
        a->cap = new_cap;
    }
    a->data[a->len++] = inst;
}

/* ------------------------------------------------------------------ */
/* IRFunctionArray helpers                                             */
/* ------------------------------------------------------------------ */

static void ir_func_array_push(IRFunctionArray *a, IRFunction fn) {
    fn.ra = NULL;  /* No register allocation yet. */
    if (a->len >= a->cap) {
        size_t new_cap = a->cap ? a->cap * 2 : 4;
        a->data = realloc(a->data, new_cap * sizeof(IRFunction));
        if (!a->data) {
            fprintf(stderr, "fakecc: out of memory\n");
            exit(1);
        }
        a->cap = new_cap;
    }
    a->data[a->len++] = fn;
}

/* ------------------------------------------------------------------ */
/* AST → IR lowering                                                   */
/* ------------------------------------------------------------------ */

/* Allocate a fresh SSA value id and return it. */
static IRValue new_value(IRFunction *fn) {
    return fn->next_value_id++;
}

/* Record width/signedness for value `v`. */
static void set_value_type(IRFunction *fn, IRValue v, int width, int is_unsigned) {
    if (v < 0) return;
    if (v >= fn->value_meta_cap) {
        int old_cap = fn->value_meta_cap;
        int new_cap = old_cap ? old_cap * 2 : 16;
        while (new_cap <= v) new_cap *= 2;
        fn->value_width = realloc(fn->value_width, new_cap * sizeof(int));
        fn->value_is_unsigned = realloc(fn->value_is_unsigned, new_cap * sizeof(int));
        if (!fn->value_width || !fn->value_is_unsigned) {
            fprintf(stderr, "fakecc: OOM\n"); exit(1);
        }
        for (int i = old_cap; i < new_cap; i++) {
            fn->value_width[i] = 4;
            fn->value_is_unsigned[i] = 0;
        }
        fn->value_meta_cap = new_cap;
    }
    fn->value_width[v] = width;
    fn->value_is_unsigned[v] = is_unsigned;
}

static int get_value_width(const IRFunction *fn, IRValue v) {
    if (v < 0 || v >= fn->value_meta_cap) return 4;
    return fn->value_width[v];
}
static int get_value_is_unsigned(const IRFunction *fn, IRValue v) {
    if (v < 0 || v >= fn->value_meta_cap) return 0;
    return fn->value_is_unsigned[v];
}

/* ---- Address-taken analysis: does `name` have `&name` anywhere in expr? ---- */
static int expr_takes_addr_of(const Expr *e, const char *name);
static int stmt_takes_addr_of(const Stmt *s, const char *name);

static int expr_takes_addr_of(const Expr *e, const char *name) {
    if (!e) return 0;
    switch (e->kind) {
    case EX_INT_LIT: return 0;
    case EX_STR: return 0;
    case EX_BINOP:
        return expr_takes_addr_of(e->u.bin.l, name)
            || expr_takes_addr_of(e->u.bin.r, name);
    case EX_UNARY: return expr_takes_addr_of(e->u.un.operand, name);
    case EX_VAR: return 0;
    case EX_ASSIGN:
        return expr_takes_addr_of(e->u.assign.lvalue, name)
            || expr_takes_addr_of(e->u.assign.rvalue, name);
    case EX_CALL: {
        for (size_t i = 0; i < e->u.call.args.len; i++)
            if (expr_takes_addr_of(e->u.call.args.data[i], name)) return 1;
        return 0;
    }
    case EX_ADDR:
        if (e->u.addr.operand->kind == EX_VAR &&
            strcmp(e->u.addr.operand->u.var.name, name) == 0) return 1;
        return expr_takes_addr_of(e->u.addr.operand, name);
    case EX_DEREF: return expr_takes_addr_of(e->u.deref.operand, name);
    case EX_INDEX:
        return expr_takes_addr_of(e->u.idx.array, name)
            || expr_takes_addr_of(e->u.idx.index, name);
    case EX_MEMBER:
        return expr_takes_addr_of(e->u.member.obj, name);
    case EX_CAST: return expr_takes_addr_of(e->u.cast.operand, name);
    case EX_SIZEOF_TYPE: return 0;
    case EX_SIZEOF_EXPR: return expr_takes_addr_of(e->u.sizeof_e.operand, name);
    }
    return 0;
}

static int stmt_takes_addr_of(const Stmt *s, const char *name) {
    if (!s) return 0;
    switch (s->kind) {
    case ST_DECL:   return s->u.decl.init && expr_takes_addr_of(s->u.decl.init, name);
    case ST_EXPR:   return expr_takes_addr_of(s->u.expr, name);
    case ST_RETURN: return expr_takes_addr_of(s->u.value, name);
    case ST_IF:
        if (expr_takes_addr_of(s->u.if_s.cond, name)) return 1;
        if (stmt_takes_addr_of(s->u.if_s.then_s, name)) return 1;
        return s->u.if_s.else_s && stmt_takes_addr_of(s->u.if_s.else_s, name);
    case ST_WHILE:
        return expr_takes_addr_of(s->u.while_s.cond, name)
            || stmt_takes_addr_of(s->u.while_s.body, name);
    case ST_FOR:
        if (s->u.for_s.init && stmt_takes_addr_of(s->u.for_s.init, name)) return 1;
        if (s->u.for_s.cond && expr_takes_addr_of(s->u.for_s.cond, name)) return 1;
        if (s->u.for_s.step && expr_takes_addr_of(s->u.for_s.step, name)) return 1;
        return stmt_takes_addr_of(s->u.for_s.body, name);
    case ST_BREAK:
    case ST_CONTINUE:
        return 0;
    case ST_BLOCK:
        for (size_t i = 0; i < s->u.block.len; i++)
            if (stmt_takes_addr_of(&s->u.block.data[i], name)) return 1;
        return 0;
    }
    return 0;
}

/* Is `name` pinned in the given function body? True iff array-typed
 * (its decl is TY_ARRAY, or param would be TY_PTR — the latter is fine) or
 * `&name` appears anywhere in the function. */
static int is_pinned_in_body(const FunctionDecl *fd, const char *name, Type ty) {
    if (ty.kind == TY_ARRAY) return 1;
    if (ty.kind == TY_STRUCT) return 1;
    for (size_t i = 0; i < fd->body.len; i++)
        if (stmt_takes_addr_of(&fd->body.data[i], name)) return 1;
    return 0;
}

/* Push an instruction with the given fields (width + signedness). */
static void emit_inst_w(IRFunction *fn, IROpcode op, IRValue dst, IRValue a, IRValue b,
                        int imm, int width, int is_unsigned, SourceLoc loc) {
    IRInst inst;
    inst.op = op;
    inst.dst = dst;
    inst.a = a;
    inst.b = b;
    inst.imm = imm;
    inst.loc = loc;
    inst.call_name = NULL;
    inst.call_nargs = 0;
    inst.width = width;
    inst.is_unsigned = is_unsigned;
    inst.alloca_bytes = 0;
    ir_inst_array_push(&fn->insts, inst);
    if (dst >= 0) set_value_type(fn, dst, width ? width : 4, is_unsigned);
}

/* Push an instruction (width/signedness default 4/signed). */
static void emit_inst(IRFunction *fn, IROpcode op, IRValue dst, IRValue a, IRValue b,
                      int imm, SourceLoc loc) {
    emit_inst_w(fn, op, dst, a, b, imm, 4, 0, loc);
}

/* Push a binary/2-source instruction with the given opcode. */
static IRValue emit_bin_w(IRFunction *fn, IROpcode op, IRValue a, IRValue b,
                          int width, int is_unsigned, SourceLoc loc) {
    IRValue v = new_value(fn);
    emit_inst_w(fn, op, v, a, b, 0, width, is_unsigned, loc);
    return v;
}

/* Emit an IR_ALLOCA with a total-byte size (for pinned allocas: arrays or
 * address-taken variables). */
static IRValue emit_alloca(IRFunction *fn, int total_bytes, int width,
                           int is_unsigned, SourceLoc loc) {
    IRValue v = new_value(fn);
    emit_inst_w(fn, IR_ALLOCA, v, -1, -1, 0, width, is_unsigned, loc);
    fn->insts.data[fn->insts.len - 1].alloca_bytes = total_bytes;
    return v;
}

/* Emit `dst = &global-with-name`. */
static IRValue emit_gaddr(IRFunction *fn, const char *name, SourceLoc loc) {
    IRValue v = new_value(fn);
    emit_inst_w(fn, IR_GADDR, v, -1, -1, 0, 8, 1, loc);
    fn->insts.data[fn->insts.len - 1].call_name = xstrdup(name);
    return v;
}

/* IR symbol table: variable name → slot (an IRValue, i.e. a stack slot). */
typedef struct {
    const char *name;
    IRValue slot;         /* alloca-value id (locals) or -1 (globals) */
    int pinned;
    int width;
    int is_unsigned;
    int is_global;        /* 1 = refers to a module-level global */
    Type ty;
} IRSlot;

typedef struct {
    IRSlot *data;
    size_t len;
    size_t cap;
} IRSymTable;

static void irsymtable_init(IRSymTable *st) {
    st->data = NULL;
    st->len = 0;
    st->cap = 0;
}

static void irsymtable_free(IRSymTable *st) {
    free(st->data);
    st->data = NULL;
    st->len = 0;
    st->cap = 0;
}

static void irsymtable_push(IRSymTable *st, const char *name, IRValue slot,
                            int pinned, Type ty) {
    if (st->len >= st->cap) {
        size_t new_cap = st->cap ? st->cap * 2 : 8;
        st->data = realloc(st->data, new_cap * sizeof(IRSlot));
        if (!st->data) {
            fprintf(stderr, "fakecc: out of memory\n");
            exit(1);
        }
        st->cap = new_cap;
    }
    st->data[st->len].name = name;
    st->data[st->len].slot = slot;
    st->data[st->len].pinned = pinned;
    st->data[st->len].width = ty.kind == TY_ARRAY
        ? type_size(*ty.elem_type)
        : (ty.kind == TY_PTR ? 8 : (ty.width ? ty.width : 4));
    st->data[st->len].is_unsigned = ty.is_unsigned;
    st->data[st->len].is_global = 0;
    st->data[st->len].ty = ty;
    st->len++;
}

static void irsymtable_push_global(IRSymTable *st, const char *name, Type ty) {
    irsymtable_push(st, name, -1, 1, ty);
    st->data[st->len - 1].is_global = 1;
}

/* Look up a variable's slot. Sema guarantees the name exists. */
#if 0
static IRValue irsymtable_get(const IRSymTable *st, const char *name) {
    for (size_t i = 0; i < st->len; i++) {
        if (strcmp(st->data[i].name, name) == 0) {
            return st->data[i].slot;
        }
    }
    return -1;
}
#endif

static const IRSlot *irsymtable_find(const IRSymTable *st, const char *name) {
    for (size_t i = st->len; i > 0; i--)
        if (strcmp(st->data[i-1].name, name) == 0) return &st->data[i-1];
    return NULL;
}

/* Forward decl. */
static IRValue lower_expr(IRFunction *fn, IRSymTable *st, const Expr *e);
static IRValue lower_lvalue_addr(IRFunction *fn, IRSymTable *st, const Expr *e);

/* Coerce a value to a target (width, is_unsigned) — emits SEXT/ZEXT/TRUNC
 * as needed. `imm` on the conversion op holds the SOURCE width so codegen
 * knows how to extend/mask. Returns the coerced SSA value id. */
static IRValue coerce(IRFunction *fn, IRValue v, int src_w, int src_u,
                      int dst_w, int dst_u, SourceLoc loc) {
    if (src_w == dst_w && src_u == dst_u) return v;
    IROpcode op;
    if (dst_w > src_w) op = src_u ? IR_ZEXT : IR_SEXT;
    else if (dst_w < src_w) op = IR_TRUNC;
    else op = IR_TRUNC;   /* same width, retag signedness */
    IRValue res = new_value(fn);
    emit_inst_w(fn, op, res, v, -1, src_w /* imm carries src width */,
                dst_w, dst_u, loc);
    return res;
}

/* Compute the address of an lvalue expression (EX_VAR/EX_DEREF/EX_INDEX/EX_MEMBER).
 * Returns a pointer-typed SSA value. */
static IRValue lower_lvalue_addr(IRFunction *fn, IRSymTable *st, const Expr *e) {
    switch (e->kind) {
    case EX_VAR: {
        const IRSlot *entry = irsymtable_find(st, e->u.var.name);
        if (entry->is_global) return emit_gaddr(fn, entry->name, e->loc);
        return emit_bin_w(fn, IR_ADDR, entry->slot, -1, 8, 1, e->loc);
    }
    case EX_DEREF:
        return lower_expr(fn, st, e->u.deref.operand);
    case EX_INDEX: {
        IRValue base = lower_expr(fn, st, e->u.idx.array);
        IRValue idx  = lower_expr(fn, st, e->u.idx.index);
        int esize = type_size(e->type);
        IRValue esize_v = new_value(fn);
        emit_inst_w(fn, IR_CONST, esize_v, -1, -1, esize, 8, 1, e->loc);
        int iw = get_value_width(fn, idx), iu = get_value_is_unsigned(fn, idx);
        IRValue idx8 = coerce(fn, idx, iw, iu, 8, 0, e->loc);
        IRValue off = emit_bin_w(fn, IR_MUL, idx8, esize_v, 8, 1, e->loc);
        return emit_bin_w(fn, IR_ADD, base, off, 8, 1, e->loc);
    }
    case EX_MEMBER: {
        IRValue base = lower_lvalue_addr(fn, st, e->u.member.obj);
        const StructDef *sd = struct_registry_find_c(g_ir_structs,
                                                     e->u.member.obj->type.tag);
        int off = 0;
        for (int i = 0; i < sd->num_members; i++)
            if (strcmp(sd->members[i].name, e->u.member.name) == 0)
                { off = sd->members[i].offset; break; }
        IRValue off_v = new_value(fn);
        emit_inst_w(fn, IR_CONST, off_v, -1, -1, off, 8, 1, e->loc);
        return emit_bin_w(fn, IR_ADD, base, off_v, 8, 1, e->loc);
    }
    default: break;
    }
    return -1;
}

/* Lower an expression to a value id, emitting instructions as needed.
 * Sema has annotated e->type; we lower operands and coerce them per UAC. */
static IRValue lower_expr(IRFunction *fn, IRSymTable *st, const Expr *e) {
    switch (e->kind) {
    case EX_INT_LIT: {
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_CONST, v, -1, -1, e->u.int_val,
                    e->type.width ? e->type.width : 4,
                    e->type.is_unsigned, e->loc);
        return v;
    }
    case EX_STR: {
        /* Register a rodata global holding the bytes, then produce its
         * address via IR_GADDR. */
        extern IRModule *g_ir_module;   /* set by ir_generate */
        extern int g_str_counter;
        char name[32];
        snprintf(name, sizeof name, "__str.%d", g_str_counter++);
        int bytes = e->u.str.len + 1;
        char *init = malloc(bytes);
        memcpy(init, e->u.str.bytes, bytes);
        ir_module_push_global(g_ir_module, name, bytes, init, 1, e->loc);
        return emit_gaddr(fn, name, e->loc);
    }
    case EX_UNARY:
        switch (e->u.un.op) {
        case UOP_NEG: {
            IRValue x = lower_expr(fn, st, e->u.un.operand);
            int sw = get_value_width(fn, x);
            int su = get_value_is_unsigned(fn, x);
            int rw = e->type.width ? e->type.width : 4;
            int ru = e->type.is_unsigned;
            IRValue px = coerce(fn, x, sw, su, rw, ru, e->loc);
            return emit_bin_w(fn, IR_NEG, px, -1, rw, ru, e->loc);
        }
        case UOP_POS:
            return lower_expr(fn, st, e->u.un.operand);
        }
        break; /* unreachable */
    case EX_BINOP: {
        /* Pointer arithmetic: p+i, i+p, p-i, p-q.
         * Sema has already decayed array operands to pointer types on both
         * l and r; check the l/r Expr's type to know. */
        Type lt = e->u.bin.l->type, rt = e->u.bin.r->type;
        int l_is_ptr = (lt.kind == TY_PTR);
        int r_is_ptr = (rt.kind == TY_PTR);
        BinOp bop = e->u.bin.op;
        if ((bop == BOP_ADD || bop == BOP_SUB) && (l_is_ptr || r_is_ptr)) {
            IRValue lv = lower_expr(fn, st, e->u.bin.l);
            IRValue rv = lower_expr(fn, st, e->u.bin.r);
            if (bop == BOP_SUB && l_is_ptr && r_is_ptr) {
                /* ptrdiff: (p - q) / sizeof(*p). */
                int esize = type_size(*lt.pointee);
                IRValue diff = emit_bin_w(fn, IR_SUB, lv, rv, 8, 0, e->loc);
                IRValue esv = new_value(fn);
                emit_inst_w(fn, IR_CONST, esv, -1, -1, esize, 8, 0, e->loc);
                return emit_bin_w(fn, IR_DIV, diff, esv, 8, 0, e->loc);
            }
            /* pointer +/- int : scale int by sizeof(pointee). */
            IRValue pv = l_is_ptr ? lv : rv;
            IRValue iv = l_is_ptr ? rv : lv;
            Type pty = l_is_ptr ? lt : rt;
            int esize = type_size(*pty.pointee);
            int iw = get_value_width(fn, iv), iu = get_value_is_unsigned(fn, iv);
            IRValue iv8 = coerce(fn, iv, iw, iu, 8, 0, e->loc);
            IRValue esv = new_value(fn);
            emit_inst_w(fn, IR_CONST, esv, -1, -1, esize, 8, 1, e->loc);
            IRValue off = emit_bin_w(fn, IR_MUL, iv8, esv, 8, 1, e->loc);
            IROpcode op = (bop == BOP_ADD) ? IR_ADD : IR_SUB;
            return emit_bin_w(fn, op, pv, off, 8, 1, e->loc);
        }
        IRValue l = lower_expr(fn, st, e->u.bin.l);
        IRValue r = lower_expr(fn, st, e->u.bin.r);
        int lw = get_value_width(fn, l), lu = get_value_is_unsigned(fn, l);
        int rw = get_value_width(fn, r), ru = get_value_is_unsigned(fn, r);

        int is_cmp = (e->u.bin.op >= BOP_EQ && e->u.bin.op <= BOP_GE);
        /* Operand width for the op = e->type for arith; UAC result for cmp. */
        int op_w, op_u;
        if (is_cmp) {
            /* Reapply UAC to operand types to know cmp width/signedness. */
            int aw = lw < 4 ? 4 : lw, au = lw < 4 ? 0 : lu;
            int bw = rw < 4 ? 4 : rw, bu = rw < 4 ? 0 : ru;
            if (aw == bw && au == bu) { op_w = aw; op_u = au; }
            else if (au == bu) { op_w = aw > bw ? aw : bw; op_u = au; }
            else {
                int uw = au ? aw : bw; int uu = 1;
                int sw = au ? bw : aw;
                if (uw >= sw) { op_w = uw; op_u = uu; }
                else { op_w = sw; op_u = 0; }
            }
        } else {
            op_w = e->type.width ? e->type.width : 4;
            op_u = e->type.is_unsigned;
        }
        IRValue pl = coerce(fn, l, lw, lu, op_w, op_u, e->loc);
        IRValue pr = coerce(fn, r, rw, ru, op_w, op_u, e->loc);
        IROpcode op;
        switch (e->u.bin.op) {
        case BOP_ADD: op = IR_ADD; break;
        case BOP_SUB: op = IR_SUB; break;
        case BOP_MUL: op = IR_MUL; break;
        case BOP_DIV: op = IR_DIV; break;
        case BOP_MOD: op = IR_MOD; break;
        case BOP_EQ:  op = IR_EQ;  break;
        case BOP_NE:  op = IR_NE;  break;
        case BOP_LT:  op = IR_LT;  break;
        case BOP_LE:  op = IR_LE;  break;
        case BOP_GT:  op = IR_GT;  break;
        case BOP_GE:  op = IR_GE;  break;
        default: op = IR_ADD; break;
        }
        int rw_res = is_cmp ? 4 : op_w;
        int ru_res = is_cmp ? 0 : op_u;
        /* Comparison ops carry operand width/signedness for codegen — we
         * encode it as the instruction's own width for cmps too (used only
         * by cmp encoding). Result value's width/signedness recorded via
         * emit_bin_w's set_value_type. */
        IRValue result = emit_bin_w(fn, op, pl, pr, is_cmp ? op_w : op_w,
                                    is_cmp ? op_u : op_u, e->loc);
        if (is_cmp) {
            /* Retag the result's own SSA width to int(4,signed). */
            set_value_type(fn, result, rw_res, ru_res);
        }
        return result;
    }
    case EX_VAR: {
        const IRSlot *entry = irsymtable_find(st, e->u.var.name);
        if (!entry) return -1;
        if (entry->is_global) {
            IRValue addr = emit_gaddr(fn, entry->name, e->loc);
            if (entry->ty.kind == TY_ARRAY || entry->ty.kind == TY_STRUCT) return addr;
            IRValue v = new_value(fn);
            emit_inst_w(fn, IR_LOAD_PTR, v, addr, -1, 0,
                        entry->width, entry->is_unsigned, e->loc);
            return v;
        }
        if (entry->ty.kind == TY_ARRAY || entry->ty.kind == TY_STRUCT) {
            return emit_bin_w(fn, IR_ADDR, entry->slot, -1, 8, 1, e->loc);
        }
        if (entry->pinned) {
            IRValue addr = emit_bin_w(fn, IR_ADDR, entry->slot, -1, 8, 1, e->loc);
            IRValue v = new_value(fn);
            emit_inst_w(fn, IR_LOAD_PTR, v, addr, -1, 0,
                        entry->width, entry->is_unsigned, e->loc);
            return v;
        }
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_LOAD, v, entry->slot, -1, 0,
                    e->type.width, e->type.is_unsigned, e->loc);
        return v;
    }
    case EX_ASSIGN: {
        Expr *lv = e->u.assign.lvalue;
        IRValue rv = lower_expr(fn, st, e->u.assign.rvalue);
        int rw = get_value_width(fn, rv), ru = get_value_is_unsigned(fn, rv);
        int lw = lv->type.kind == TY_PTR ? 8 : (lv->type.width ? lv->type.width : 4);
        int lu = lv->type.is_unsigned;
        IRValue coerced = coerce(fn, rv, rw, ru, lw, lu, e->loc);
        if (lv->kind == EX_VAR) {
            const IRSlot *entry = irsymtable_find(st, lv->u.var.name);
            if (!entry) return -1;
            if (entry->is_global) {
                IRValue addr = emit_gaddr(fn, entry->name, e->loc);
                emit_inst_w(fn, IR_STORE_PTR, -1, addr, coerced, 0, lw, lu, e->loc);
            } else if (entry->pinned) {
                IRValue addr = emit_bin_w(fn, IR_ADDR, entry->slot, -1, 8, 1, e->loc);
                emit_inst_w(fn, IR_STORE_PTR, -1, addr, coerced, 0, lw, lu, e->loc);
            } else {
                emit_inst_w(fn, IR_STORE, -1, entry->slot, coerced, 0, lw, lu, e->loc);
            }
            return coerced;
        } else if (lv->kind == EX_DEREF) {
            IRValue ptr = lower_expr(fn, st, lv->u.deref.operand);
            emit_inst_w(fn, IR_STORE_PTR, -1, ptr, coerced, 0, lw, lu, e->loc);
            return coerced;
        } else if (lv->kind == EX_INDEX || lv->kind == EX_MEMBER) {
            IRValue addr = lower_lvalue_addr(fn, st, lv);
            emit_inst_w(fn, IR_STORE_PTR, -1, addr, coerced, 0, lw, lu, e->loc);
            return coerced;
        }
        return coerced;
    }
    case EX_CALL: {
        const FunctionDecl *unused = NULL; (void)unused;
        IRValue arg_vals[IR_CALL_MAX_ARGS];
        int nargs = (int)e->u.call.args.len;
        for (int i = 0; i < nargs; i++)
            arg_vals[i] = lower_expr(fn, st, e->u.call.args.data[i]);
        IRValue v = new_value(fn);
        IRInst inst;
        inst.op = IR_CALL;
        inst.dst = v;
        inst.a = -1;
        inst.b = -1;
        inst.imm = 0;
        inst.loc = e->loc;
        inst.call_name = xstrdup(e->u.call.callee);
        inst.call_nargs = nargs;
        for (int i = 0; i < nargs; i++) inst.call_args[i] = arg_vals[i];
        inst.width = e->type.width ? e->type.width : 4;
        inst.is_unsigned = e->type.is_unsigned;
        ir_inst_array_push(&fn->insts, inst);
        set_value_type(fn, v, inst.width, inst.is_unsigned);
        return v;
    }
    case EX_ADDR: {
        Expr *op = e->u.addr.operand;
        if (op->kind == EX_VAR) {
            const IRSlot *entry = irsymtable_find(st, op->u.var.name);
            if (entry->is_global) return emit_gaddr(fn, entry->name, e->loc);
            return emit_bin_w(fn, IR_ADDR, entry->slot, -1, 8, 1, e->loc);
        }
        if (op->kind == EX_DEREF) {
            /* &*p == p */
            return lower_expr(fn, st, op->u.deref.operand);
        }
        if (op->kind == EX_INDEX || op->kind == EX_MEMBER) {
            return lower_lvalue_addr(fn, st, op);
        }
        return -1;
    }
    case EX_DEREF: {
        IRValue ptr = lower_expr(fn, st, e->u.deref.operand);
        if (e->type.kind == TY_STRUCT || e->type.kind == TY_ARRAY) return ptr;
        int w = e->type.kind == TY_PTR ? 8 : (e->type.width ? e->type.width : 4);
        int u = e->type.is_unsigned;
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, v, ptr, -1, 0, w, u, e->loc);
        return v;
    }
    case EX_INDEX: {
        /* Same as *(a + i*sizeof(elem)) for rvalue use. */
        IRValue base = lower_expr(fn, st, e->u.idx.array);
        IRValue idx  = lower_expr(fn, st, e->u.idx.index);
        int esize = type_size(e->type);
        IRValue esize_v = new_value(fn);
        emit_inst_w(fn, IR_CONST, esize_v, -1, -1, esize, 8, 1, e->loc);
        int iw = get_value_width(fn, idx), iu = get_value_is_unsigned(fn, idx);
        IRValue idx8 = coerce(fn, idx, iw, iu, 8, 0, e->loc);
        IRValue off = emit_bin_w(fn, IR_MUL, idx8, esize_v, 8, 1, e->loc);
        IRValue addr = emit_bin_w(fn, IR_ADD, base, off, 8, 1, e->loc);
        int w = e->type.kind == TY_PTR ? 8 : (e->type.width ? e->type.width : 4);
        int u = e->type.is_unsigned;
        if (e->type.kind == TY_ARRAY) return addr;
        if (e->type.kind == TY_STRUCT) return addr;
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, v, addr, -1, 0, w, u, e->loc);
        return v;
    }
    case EX_MEMBER: {
        IRValue addr = lower_lvalue_addr(fn, st, e);
        /* Struct/array members: return address (decay). Scalar/pointer: load. */
        if (e->type.kind == TY_STRUCT) return addr;
        if (e->type.kind == TY_ARRAY)  return addr;
        int w = e->type.kind == TY_PTR ? 8 : (e->type.width ? e->type.width : 4);
        int u = e->type.is_unsigned;
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, v, addr, -1, 0, w, u, e->loc);
        return v;
    }
    case EX_CAST: {
        IRValue x = lower_expr(fn, st, e->u.cast.operand);
        int sw = get_value_width(fn, x), su = get_value_is_unsigned(fn, x);
        int dw = e->type.kind == TY_PTR ? 8 : (e->type.width ? e->type.width : 4);
        int du = e->type.kind == TY_PTR ? 1 : e->type.is_unsigned;
        return coerce(fn, x, sw, su, dw, du, e->loc);
    }
    case EX_SIZEOF_TYPE: {
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_CONST, v, -1, -1, type_size(e->u.sizeof_t.target),
                    8, 1, e->loc);
        return v;
    }
    case EX_SIZEOF_EXPR: {
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_CONST, v, -1, -1, type_size(e->u.sizeof_e.operand->type),
                    8, 1, e->loc);
        return v;
    }
    }
    /* unreachable */
    return -1;
}

/* Allocate a fresh label id. */
static int new_label(IRFunction *fn) {
    return fn->next_label_id++;
}

/* Emit a LABEL pseudo-instruction. */
static void emit_label(IRFunction *fn, int label, SourceLoc loc) {
    emit_inst(fn, IR_LABEL, -1, -1, -1, label, loc);
}

/* Emit unconditional branch. */
static void emit_br(IRFunction *fn, int label, SourceLoc loc) {
    emit_inst(fn, IR_BR, -1, -1, -1, label, loc);
}

/* Emit conditional branch: if cond true jump to t_label else f_label. */
static void emit_cbr(IRFunction *fn, IRValue cond, int t_label, int f_label,
                     SourceLoc loc) {
    /* CBR encoding: a=cond, imm=t_label, b=f_label */
    emit_inst(fn, IR_CBR, -1, cond, f_label, t_label, loc);
}

/* Lower a single statement, emitting instructions as needed. */
static void lower_stmt(IRFunction *fn, IRSymTable *st, const Stmt *s,
                       const FunctionDecl *cur_fd);

static void lower_stmt(IRFunction *fn, IRSymTable *st, const Stmt *s,
                       const FunctionDecl *cur_fd) {
    switch (s->kind) {
    case ST_DECL: {
        Type dty = s->u.decl.type;
        int pinned = is_pinned_in_body(cur_fd, s->u.decl.name, dty);
        int dw = dty.kind == TY_ARRAY ? type_size(*dty.elem_type)
                : (dty.kind == TY_PTR ? 8
                   : (dty.kind == TY_STRUCT ? 8 : dty.width));
        int du = dty.is_unsigned;
        IRValue v;
        if (pinned) {
            int total = type_size(dty);
            v = emit_alloca(fn, total, dw, du, s->loc);
        } else {
            v = new_value(fn);
            emit_inst_w(fn, IR_ALLOCA, v, -1, -1, 0, dw, du, s->loc);
        }
        irsymtable_push(st, s->u.decl.name, v, pinned, dty);
        if (s->u.decl.init) {
            if (dty.kind == TY_STRUCT || dty.kind == TY_ARRAY) {
                fprintf(stderr, "fakecc: struct/array initializers not supported in this slice\n");
                exit(1);
            }
            IRValue rv = lower_expr(fn, st, s->u.decl.init);
            int rw = get_value_width(fn, rv), ru = get_value_is_unsigned(fn, rv);
            IRValue coerced = coerce(fn, rv, rw, ru, dw, du, s->loc);
            if (pinned) {
                IRValue addr = emit_bin_w(fn, IR_ADDR, v, -1, 8, 1, s->loc);
                emit_inst_w(fn, IR_STORE_PTR, -1, addr, coerced, 0, dw, du, s->loc);
            } else {
                emit_inst_w(fn, IR_STORE, -1, v, coerced, 0, dw, du, s->loc);
            }
        }
        break;
    }
    case ST_EXPR:
        lower_expr(fn, st, s->u.expr);
        break;
    case ST_RETURN: {
        IRValue v = lower_expr(fn, st, s->u.value);
        int vw = get_value_width(fn, v), vu = get_value_is_unsigned(fn, v);
        IRValue coerced = coerce(fn, v, vw, vu, fn->ret_width, fn->ret_is_unsigned, s->loc);
        emit_inst_w(fn, IR_RETURN, -1, coerced, -1, 0,
                    fn->ret_width, fn->ret_is_unsigned, s->loc);
        break;
    }
    case ST_IF: {
        IRValue cond = lower_expr(fn, st, s->u.if_s.cond);
        int L_then = new_label(fn);
        int L_end  = new_label(fn);
        int L_else = s->u.if_s.else_s ? new_label(fn) : L_end;
        emit_cbr(fn, cond, L_then, L_else, s->loc);
        emit_label(fn, L_then, s->loc);
        lower_stmt(fn, st, s->u.if_s.then_s, cur_fd);
        if (s->u.if_s.else_s) {
            emit_br(fn, L_end, s->loc);
            emit_label(fn, L_else, s->loc);
            lower_stmt(fn, st, s->u.if_s.else_s, cur_fd);
        }
        emit_label(fn, L_end, s->loc);
        break;
    }
    case ST_WHILE: {
        int L_head = new_label(fn);
        int L_body = new_label(fn);
        int L_exit = new_label(fn);
        emit_br(fn, L_head, s->loc);
        emit_label(fn, L_head, s->loc);
        IRValue cond = lower_expr(fn, st, s->u.while_s.cond);
        emit_cbr(fn, cond, L_body, L_exit, s->loc);
        emit_label(fn, L_body, s->loc);
        push_loop(L_head, L_exit);
        lower_stmt(fn, st, s->u.while_s.body, cur_fd);
        pop_loop();
        emit_br(fn, L_head, s->loc);
        emit_label(fn, L_exit, s->loc);
        break;
    }
    case ST_FOR: {
        /* Lowering:
         *   [init]
         *   L_head: [cond ? goto L_body : goto L_exit]  (or unconditional
         *           goto L_body if cond absent)
         *   L_body: body
         *   L_step: [step]
         *           goto L_head
         *   L_exit:
         * continue → L_step, break → L_exit. */
        size_t mark = st->len;
        if (s->u.for_s.init)
            lower_stmt(fn, st, s->u.for_s.init, cur_fd);
        int L_head = new_label(fn);
        int L_body = new_label(fn);
        int L_step = new_label(fn);
        int L_exit = new_label(fn);
        emit_br(fn, L_head, s->loc);
        emit_label(fn, L_head, s->loc);
        if (s->u.for_s.cond) {
            IRValue cond = lower_expr(fn, st, s->u.for_s.cond);
            emit_cbr(fn, cond, L_body, L_exit, s->loc);
        } else {
            emit_br(fn, L_body, s->loc);
        }
        emit_label(fn, L_body, s->loc);
        push_loop(L_step, L_exit);
        lower_stmt(fn, st, s->u.for_s.body, cur_fd);
        pop_loop();
        emit_br(fn, L_step, s->loc);
        emit_label(fn, L_step, s->loc);
        if (s->u.for_s.step) lower_expr(fn, st, s->u.for_s.step);
        emit_br(fn, L_head, s->loc);
        emit_label(fn, L_exit, s->loc);
        st->len = mark;
        break;
    }
    case ST_BREAK:
        emit_br(fn, g_loops[g_loop_depth - 1].break_label, s->loc);
        break;
    case ST_CONTINUE:
        emit_br(fn, g_loops[g_loop_depth - 1].cont_label, s->loc);
        break;
    case ST_BLOCK: {
        size_t mark = st->len;
        for (size_t i = 0; i < s->u.block.len; i++) {
            lower_stmt(fn, st, &s->u.block.data[i], cur_fd);
        }
        st->len = mark;
        break;
    }
    }
}

void ir_generate(const TranslationUnit *tu, IRModule *ir) {
    /* Publish module + reset string counter for lower_expr's use. */
    g_ir_module = ir;
    g_str_counter = 0;
    g_ir_structs = &tu->structs;

    /* Register named globals from tu->globals.  Populate initializer bytes
     * for scalar globals from a simple compile-time integer literal (or 0
     * if no init).  Complex initializers (arrays, non-const exprs) are
     * left to a future slice — for 8 we accept `int x = 42;` style. */
    for (size_t i = 0; i < tu->globals.len; i++) {
        const Stmt *s = &tu->globals.data[i];
        if (s->kind != ST_DECL) continue;
        int sz = type_size(s->u.decl.type);
        if (sz <= 0) sz = 8;
        char *bytes = calloc(sz, 1);
        if (!bytes) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        if (s->u.decl.init) {
            Expr *ie = s->u.decl.init;
            if (ie->kind == EX_INT_LIT) {
                long long v = ie->u.int_val;
                for (int b = 0; b < sz && b < 8; b++)
                    bytes[b] = (char)((v >> (8 * b)) & 0xff);
            } else if (ie->kind == EX_UNARY && ie->u.un.op == UOP_NEG
                       && ie->u.un.operand->kind == EX_INT_LIT) {
                long long v = -(long long)ie->u.un.operand->u.int_val;
                for (int b = 0; b < sz && b < 8; b++)
                    bytes[b] = (char)((v >> (8 * b)) & 0xff);
            } else {
                die_at(s->loc.file, s->loc.line, s->loc.col,
                       "global '%s' initializer must be an integer literal (Slice 8 limit)",
                       s->u.decl.name);
            }
        }
        ir_module_push_global(ir, s->u.decl.name, sz, bytes, 0, s->loc);
    }

    for (size_t i = 0; i < tu->functions.len; i++) {
        const FunctionDecl *fd = &tu->functions.data[i];

        IRFunction irfn;
        irfn.name = xstrdup(fd->name);
        irfn.loc = fd->loc;
        irfn.insts.data = NULL;
        irfn.insts.len = 0;
        irfn.insts.cap = 0;
        irfn.next_value_id = 0;
        irfn.next_label_id = 0;
        irfn.value_width = NULL;
        irfn.value_is_unsigned = NULL;
        irfn.value_meta_cap = 0;
        irfn.ret_width = fd->ret_type.width;
        irfn.ret_is_unsigned = fd->ret_type.is_unsigned;

        IRSymTable st;
        irsymtable_init(&st);

        /* Bind globals so function-body EX_VAR lookups find them. */
        for (size_t g = 0; g < tu->globals.len; g++) {
            const Stmt *gs = &tu->globals.data[g];
            if (gs->kind == ST_DECL)
                irsymtable_push_global(&st, gs->u.decl.name, gs->u.decl.type);
        }

        for (size_t p = 0; p < fd->params.len; p++) {
            Type pty = fd->params.data[p].type;
            int pw = pty.kind == TY_PTR ? 8 : (pty.width ? pty.width : 4);
            int pu = pty.kind == TY_PTR ? 1 : pty.is_unsigned;
            const char *pname = fd->params.data[p].name;
            int pinned = is_pinned_in_body(fd, pname, pty);

            IRValue param_v = new_value(&irfn);
            emit_inst_w(&irfn, IR_PARAM, param_v, -1, -1, (int)p, pw, pu,
                        fd->params.data[p].loc);

            IRValue slot;
            if (pinned) {
                int total = type_size(pty);
                slot = emit_alloca(&irfn, total, pw, pu, fd->params.data[p].loc);
                IRValue addr = emit_bin_w(&irfn, IR_ADDR, slot, -1, 8, 1,
                                          fd->params.data[p].loc);
                emit_inst_w(&irfn, IR_STORE_PTR, -1, addr, param_v, 0, pw, pu,
                            fd->params.data[p].loc);
            } else {
                slot = new_value(&irfn);
                emit_inst_w(&irfn, IR_ALLOCA, slot, -1, -1, 0, pw, pu,
                            fd->params.data[p].loc);
                emit_inst_w(&irfn, IR_STORE, -1, slot, param_v, 0, pw, pu,
                            fd->params.data[p].loc);
            }
            irsymtable_push(&st, pname, slot, pinned, pty);
        }

        for (size_t j = 0; j < fd->body.len; j++) {
            lower_stmt(&irfn, &st, &fd->body.data[j], fd);
        }

        irsymtable_free(&st);
        ir_func_array_push(&ir->functions, irfn);
    }
}
