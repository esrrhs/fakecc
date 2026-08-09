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
int g_flt_counter = 0;
const StructRegistry *g_ir_structs = NULL;   /* set by ir_generate */
const TranslationUnit *g_ir_tu = NULL;      /* set by ir_generate */

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
        free(m->functions.data[i].value_is_float);
        /* Free register allocation results if present. */
        extern void ra_result_free(void *ra);
        if (m->functions.data[i].ra)
            ra_result_free(m->functions.data[i].ra);
        if (m->functions.data[i].ra_xmm)
            ra_result_free(m->functions.data[i].ra_xmm);
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
                                       int is_readonly, int is_static,
                                       SourceLoc loc) {
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
    g->is_static = is_static;
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
        fn->value_is_float = realloc(fn->value_is_float, new_cap * sizeof(int));
        if (!fn->value_width || !fn->value_is_unsigned || !fn->value_is_float) {
            fprintf(stderr, "fakecc: OOM\n"); exit(1);
        }
        for (int i = old_cap; i < new_cap; i++) {
            fn->value_width[i] = 4;
            fn->value_is_unsigned[i] = 0;
            fn->value_is_float[i] = 0;
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
    case EX_FLOAT_LIT: return 0;
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
    case EX_TERNARY:
        return expr_takes_addr_of(e->u.tern.cond, name)
            || expr_takes_addr_of(e->u.tern.then, name)
            || expr_takes_addr_of(e->u.tern.else_, name);
    case EX_INC_DEC:
        return expr_takes_addr_of(e->u.incdec.operand, name);
    case EX_COMPOUND_ASSIGN:
        return expr_takes_addr_of(e->u.comp.lvalue, name);
    case EX_COMMA:
        return expr_takes_addr_of(e->u.comma.lhs, name)
            || expr_takes_addr_of(e->u.comma.rhs, name);
    case EX_INIT_LIST:
        for (int i = 0; i < e->u.init_list.num_elements; i++)
            if (expr_takes_addr_of(e->u.init_list.elements[i], name)) return 1;
        return 0;
    case EX_COMPOUND_LITERAL:
        return expr_takes_addr_of(e->u.compound.init, name);
    case EX_ALIGNOF_TYPE:
        return 0; /* _Alignof(T) references no variable */
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
    case ST_DO_WHILE:
        return expr_takes_addr_of(s->u.do_s.cond, name)
            || stmt_takes_addr_of(s->u.do_s.body, name);
    case ST_GOTO:
        return 0;
    case ST_LABEL:
        return stmt_takes_addr_of(s->u.label_s.stmt, name);
    case ST_SWITCH:
        if (expr_takes_addr_of(s->u.switch_s.cond, name)) return 1;
        for (int i = 0; i < s->u.switch_s.num_cases; i++)
            for (size_t j = 0; j < s->u.switch_s.cases[i].stmts.len; j++)
                if (stmt_takes_addr_of(&s->u.switch_s.cases[i].stmts.data[j], name))
                    return 1;
        return 0;
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

/* Look up bitfield info for `e` (an EX_MEMBER).  If the accessed member is a
 * bitfield, set *bit_width/bit_offset to its width (bits) and position within
 * the storage unit, and return 1.  Otherwise return 0.  The storage unit is the
 * naturally-aligned integer (1/2/4 bytes) that holds the bitfield run. */
static int member_bitfield(const Expr *e, int *bit_width, int *bit_offset,
                           int *unit_width) {
    if (e->kind != EX_MEMBER) return 0;
    if (e->u.member.obj->type.kind != TY_STRUCT) return 0;
    const char *tag = e->u.member.obj->type.tag;
    if (!tag) return 0;
    const StructDef *sd = struct_registry_find_c(g_ir_structs, tag);
    if (!sd) return 0;
    for (int i = 0; i < sd->num_members; i++) {
        if (strcmp(sd->members[i].name, e->u.member.name) == 0) {
            if (sd->members[i].bit_width > 0) {
                *bit_width = sd->members[i].bit_width;
                *bit_offset = sd->members[i].bit_offset;
                *unit_width = type_size(sd->members[i].type);
                return 1;
            }
            return 0;
        }
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

/* Record float-ness for value `v`. */
static void set_value_float(IRFunction *fn, IRValue v, int is_float) {
    if (v < 0) return;
    if (v >= fn->value_meta_cap) {
        /* Ensure meta arrays are allocated (set_value_type would do this, but
         * a pure-float value may never pass through set_value_type). */
        set_value_type(fn, v, 4, 0);
    }
    if (fn->value_is_float)
        fn->value_is_float[v] = is_float;
}
static int get_value_is_float(const IRFunction *fn, IRValue v) {
    if (v < 0 || v >= fn->value_meta_cap || !fn->value_is_float) return 0;
    return fn->value_is_float[v];
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
    inst.float_imm = 0;
    inst.is_float = 0;
    ir_inst_array_push(&fn->insts, inst);
    if (dst >= 0) set_value_type(fn, dst, width ? width : 4, is_unsigned);
}

/* Push a float-typed instruction. */
static void emit_inst_f(IRFunction *fn, IROpcode op, IRValue dst, IRValue a, IRValue b,
                        int width, SourceLoc loc) {
    emit_inst_w(fn, op, dst, a, b, 0, width, 0, loc);
    if (dst >= 0) {
        set_value_float(fn, dst, 1);
        fn->insts.data[fn->insts.len - 1].is_float = 1;
    }
}

/* Emit a float constant (bit pattern in `bits`). */
static IRValue emit_float_const(IRFunction *fn, int width, int64_t bits, SourceLoc loc) {
    IRValue v = new_value(fn);
    emit_inst_w(fn, IR_CONST, v, -1, -1, 0, width, 0, loc);
    IRInst *inst = &fn->insts.data[fn->insts.len - 1];
    inst->is_float = 1;
    inst->float_imm = bits;
    set_value_float(fn, v, 1);
    return v;
}

/* Forward decls — bool_normalize (below) calls coerce / emit_bin_w, which are
 * defined later in this file. */
static IRValue coerce(IRFunction *fn, IRValue v, int src_w, int src_u,
                      int dst_w, int dst_u, SourceLoc loc);
static IRValue emit_bin_w(IRFunction *fn, IROpcode op, IRValue a, IRValue b,
                          int width, int is_unsigned, SourceLoc loc);

/* Normalize any scalar value to a 0/1 int (width 4) for a _Bool destination.
 * C _Bool semantics: 0 if the value compares equal to 0, else 1.  The
 * comparison is done at the source width/domain so that e.g. (_Bool)0x100
 * is 1, not 0 (an early truncate to width 1 would lose the high bits). */
static IRValue bool_normalize(IRFunction *fn, IRValue v, int w, int u,
                              int is_float, SourceLoc loc) {
    if (is_float) {
        IRValue zero = emit_float_const(fn, w, 0, loc);
        IRValue r = new_value(fn);
        emit_inst_f(fn, IR_FCMP, r, v, zero, w, loc);
        fn->insts.data[fn->insts.len - 1].is_unsigned = 5;  /* NE encoding */
        set_value_float(fn, r, 0);
        set_value_type(fn, r, 4, 0);
        return r;
    }
    IRValue i = coerce(fn, v, w, u, 4, 0, loc);
    IRValue zero = new_value(fn);
    emit_inst_w(fn, IR_CONST, zero, -1, -1, 0, 4, 0, loc);
    return emit_bin_w(fn, IR_NE, i, zero, 4, 0, loc);
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

/* Emit `dst = ptr + delta` where delta is a compile-time byte constant.
 * Used to form addresses of struct members / copy chunks. */
static IRValue emit_add_const(IRFunction *fn, IRValue ptr, int delta,
                              SourceLoc loc) {
    if (delta == 0) return ptr;
    IRValue c = new_value(fn);
    emit_inst_w(fn, IR_CONST, c, -1, -1, delta, 8, 1, loc);
    return emit_bin_w(fn, IR_ADD, ptr, c, 8, 1, loc);
}

/* Copy `size` bytes from *src into *dst, one natural-width chunk at a time
 * (8/4/2/1 bytes).  Implements struct value copy: a struct is represented as
 * a pointer to its bytes, so every by-value boundary (param bind, return,
 * assignment) must move the bytes, not the pointer.  Chosen width is always
 * a power of two no larger than the remaining bytes and no larger than 8, so
 * it matches a single LOAD_PTR/STORE_PTR on the codegen side. */
static void emit_struct_copy(IRFunction *fn, IRValue dst, IRValue src,
                              int size, SourceLoc loc) {
    int off = 0;
    while (size > 0) {
        int w = (size >= 8) ? 8 : (size >= 4) ? 4 : (size >= 2) ? 2 : 1;
        IRValue saddr = emit_add_const(fn, src, off, loc);
        IRValue daddr = emit_add_const(fn, dst, off, loc);
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, v, saddr, -1, 0, w, 1, loc);
        emit_inst_w(fn, IR_STORE_PTR, -1, daddr, v, 0, w, 1, loc);
        off += w;
        size -= w;
    }
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
    const char *global_name; /* non-NULL only for static locals: the mangled
                              * global (fn.name) that backs this variable.
                              * emit_gaddr uses this instead of `name`. */
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
    st->data[st->len].global_name = NULL;
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

/* Push a static local: it is backed by a mangled global `global_name`
 * (fn.name " . " varname) but bound in the symbol table under its C name so
 * ordinary EX_VAR lookups find it.  The global path in lower_expr handles the
 * load/store via the mangled name. */
static void irsymtable_push_static_local(IRSymTable *st, const char *name,
                                         const char *global_name, Type ty) {
    irsymtable_push(st, name, -1, 1, ty);
    st->data[st->len - 1].is_global = 1;
    st->data[st->len - 1].global_name = xstrdup(global_name);
}

/* The global name a global/static-local slot refers to.  For ordinary globals
 * this is the C name; for static locals it is the mangled fn.varname. */
static const char *slot_global_name(const IRSlot *entry) {
    return entry->global_name ? entry->global_name : entry->name;
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

/* Initializer-list lowering (defined after ir_generate). */
static void pack_init(const IRModule *ir, const Type *ty, const Expr *e,
                      char *bytes, int sz, const char *ctx, SourceLoc loc);
static void lower_init_list(IRFunction *fn, IRSymTable *st, IRValue base,
                            const Type *ty, const Expr *e, SourceLoc loc);

/* Control-flow helpers used by short-circuit lowering (defined below lower_stmt). */
static int  new_label(IRFunction *fn);
static void emit_label(IRFunction *fn, int label, SourceLoc loc);
static void emit_br(IRFunction *fn, int label, SourceLoc loc);
static void emit_cbr(IRFunction *fn, IRValue cond, int t_label, int f_label,
                     SourceLoc loc);

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

/* Convert value `v` between float and int domains (or resize float↔double).
 *   to_float: 1 → convert int→float (SITOFP); 0 → float→int (FPTOSI).
 *   For float↔float, FPEXT (widen) or FPTRUNC (narrow).
 * Returns the (possibly new) value id, with value_is_float set appropriately. */
static IRValue convert_numeric(IRFunction *fn, IRValue v,
                               int src_w, int dst_w, int dst_u,
                               int to_float, SourceLoc loc) {
    int src_float = get_value_is_float(fn, v);
    IRValue res = new_value(fn);
    IROpcode op;
    if (src_float && to_float) {
        /* float → float (width change). */
        op = (dst_w > src_w) ? IR_FPEXT : IR_FPTRUNC;
        emit_inst_f(fn, op, res, v, -1, dst_w, loc);
        return res;
    }
    if (src_float && !to_float) {
        /* float → int. */
        op = IR_FPTOSI;
        emit_inst_w(fn, op, res, v, -1, src_w, dst_w, dst_u, loc);
        set_value_float(fn, res, 0);
        return res;
    }
    /* int → float. */
    op = IR_SITOFP;
    emit_inst_w(fn, op, res, v, -1, src_w, dst_w, 0, loc);
    set_value_float(fn, res, 1);
    return res;
}

/* Map a BinOp (AST) to the matching IROpcode (IR). */
static IROpcode bop_to_ir(BinOp op) {
    switch (op) {
    case BOP_ADD:    return IR_ADD;
    case BOP_SUB:    return IR_SUB;
    case BOP_MUL:    return IR_MUL;
    case BOP_DIV:    return IR_DIV;
    case BOP_MOD:    return IR_MOD;
    case BOP_EQ:     return IR_EQ;
    case BOP_NE:     return IR_NE;
    case BOP_LT:     return IR_LT;
    case BOP_LE:     return IR_LE;
    case BOP_GT:     return IR_GT;
    case BOP_GE:     return IR_GE;
    case BOP_BITAND: return IR_BAND;
    case BOP_BITOR:  return IR_BOR;
    case BOP_BITXOR: return IR_BXOR;
    case BOP_SHL:    return IR_SHL;
    case BOP_SHR:    return IR_SHR;
    default:         return IR_ADD;
    }
}

/* Prepare the right-hand side of a compound assignment: for pointer += / -=,
 * scale the integer rvalue by sizeof(pointee); otherwise coerce it to the
 * lvalue's (width, is_unsigned). */
static IRValue scale_rhs(IRFunction *fn, IRValue rhs, int is_ptr, Type lv_ty,
                         BinOp op, SourceLoc loc) {
    int rw = get_value_width(fn, rhs), ru = get_value_is_unsigned(fn, rhs);
    if (is_ptr && (op == BOP_ADD || op == BOP_SUB)) {
        /* Scale integer rvalue by sizeof(pointee), in 8-byte unsigned. */
        IRValue r8 = coerce(fn, rhs, rw, ru, 8, 1, loc);
        int esize = type_size(*lv_ty.pointee);
        IRValue ev = new_value(fn);
        emit_inst_w(fn, IR_CONST, ev, -1, -1, esize, 8, 1, loc);
        return emit_bin_w(fn, IR_MUL, r8, ev, 8, 1, loc);
    }
    int lw = is_ptr ? 8 : (lv_ty.width ? lv_ty.width : 4);
    int lu = is_ptr ? 1 : lv_ty.is_unsigned;
    return coerce(fn, rhs, rw, ru, lw, lu, loc);
}

/* Compute the address of an lvalue expression (EX_VAR/EX_DEREF/EX_INDEX/EX_MEMBER).
 * Returns a pointer-typed SSA value. */
static IRValue lower_lvalue_addr(IRFunction *fn, IRSymTable *st, const Expr *e) {
    switch (e->kind) {
    case EX_VAR: {
        const IRSlot *entry = irsymtable_find(st, e->u.var.name);
        if (entry->is_global) return emit_gaddr(fn, slot_global_name(entry), e->loc);
        return emit_bin_w(fn, IR_ADDR, entry->slot, -1, 8, 1, e->loc);
    }
    case EX_DEREF:
        return lower_expr(fn, st, e->u.deref.operand);
    case EX_CALL: {
        /* A struct call returns a pointer to its bytes (the sret slot address,
         * or a caller-allocated slot).  That pointer is the base address for
         * member access — e.g. `make(3,4).a`.  Lower the call to materialize
         * it (emitting the sret setup), then use the result as the lvalue. */
        return lower_expr(fn, st, e);
    }
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
    case EX_COMPOUND_LITERAL:
        /* A compound literal yields an lvalue: its address is the base for `&`
         * or member access (e.g. `(struct S){1,2}.x`).  Lowering the literal
         * materializes the storage; the result is the object's address. */
        return lower_expr(fn, st, e);
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
    case EX_FLOAT_LIT: {
        /* Parse the source text for full precision.  float/double bit-cast
         * into an int64 payload (emitted into the XMM file at codegen).
         * long double (width 16) needs 80 bits a double cannot hold, so its
         * bytes live in a 10-byte rodata global; the IR value is a slot-backed
         * constant initialized from that global (see codegen IR_CONST). */
        int w = e->type.width ? e->type.width : 8;
        if (w == 16) {
            extern IRModule *g_ir_module;   /* set by ir_generate */
            extern int g_flt_counter;
            long double val = strtold(e->u.float_text, NULL);
            char name[32];
            snprintf(name, sizeof name, "__fld.%d", g_flt_counter++);
            char *init = malloc(10);
            memcpy(init, &val, 10);
            /* anonymous constant — file-local by construction */
            ir_module_push_global(g_ir_module, name, 10, init, 1, 1, e->loc);
            IRValue v = new_value(fn);
            emit_inst_w(fn, IR_CONST, v, -1, -1, 0, 16, 0, e->loc);
            fn->insts.data[fn->insts.len - 1].is_float = 1;
            fn->insts.data[fn->insts.len - 1].call_name = xstrdup(name);
            set_value_float(fn, v, 1);
            return v;
        }
        int64_t bits = 0;
        if (w == 4) { float f = (float)strtod(e->u.float_text, NULL); *(float*)&bits = f; }
        else { double d = strtod(e->u.float_text, NULL); *(double*)&bits = d; }
        return emit_float_const(fn, w, bits, e->loc);
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
        /* anonymous constant — file-local by construction */
        ir_module_push_global(g_ir_module, name, bytes, init, 1, 1, e->loc);
        return emit_gaddr(fn, name, e->loc);
    }
    case EX_UNARY:
        switch (e->u.un.op) {
        case UOP_NEG: {
            IRValue x = lower_expr(fn, st, e->u.un.operand);
            int sw = get_value_width(fn, x);
            int su = get_value_is_unsigned(fn, x);
            int sf = get_value_is_float(fn, x);
            int rw = e->type.width ? e->type.width : 4;
            int ru = e->type.is_unsigned;
            if (sf) {
                /* Float negation: -x = 0.0 - x. */
                IRValue zero = emit_float_const(fn, rw, 0, e->loc);
                IRValue neg = emit_bin_w(fn, IR_FSUB, zero, x, rw, 0, e->loc);
                set_value_float(fn, neg, 1);
                return neg;
            }
            IRValue px = coerce(fn, x, sw, su, rw, ru, e->loc);
            return emit_bin_w(fn, IR_NEG, px, -1, rw, ru, e->loc);
        }
        case UOP_POS:
            return lower_expr(fn, st, e->u.un.operand);
        case UOP_NOT: {
            /* Logical NOT: !x → (x == 0). Result is int 0/1. */
            IRValue x = lower_expr(fn, st, e->u.un.operand);
            int xw = get_value_width(fn, x), xu = get_value_is_unsigned(fn, x);
            IRValue zero = new_value(fn);
            emit_inst_w(fn, IR_CONST, zero, -1, -1, 0, xw, xu, e->loc);
            IRValue cmp = emit_bin_w(fn, IR_EQ, x, zero, xw, xu, e->loc);
            /* Retag result to int(4,signed). */
            set_value_type(fn, cmp, 4, 0);
            return cmp;
        }
        case UOP_BITNOT: {
            IRValue x = lower_expr(fn, st, e->u.un.operand);
            int sw = get_value_width(fn, x);
            int su = get_value_is_unsigned(fn, x);
            int rw = e->type.width ? e->type.width : 4;
            int ru = e->type.is_unsigned;
            IRValue px = coerce(fn, x, sw, su, rw, ru, e->loc);
            IRValue v = new_value(fn);
            emit_inst_w(fn, IR_BNOT, v, px, -1, 0, rw, ru, e->loc);
            return v;
        }
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
        if (bop == BOP_AND || bop == BOP_OR) {
            /* Short-circuit logical operators: lower to control flow writing a
             * temporary alloca, then let mem2reg promote it into a φ-merged
             * SSA value (no IR_PHI opcode needed).
             *   a && b  →  t = a; if (!t) goto L_false; t = (b != 0);
             *              goto L_done; L_false: t = 0; L_done: ... t
             *   a || b  →  t = a; if (t) goto L_true; t = (b != 0);
             *              goto L_done; L_true: t = 1; L_done: ... t */
            IRValue slot = new_value(fn);
            emit_inst_w(fn, IR_ALLOCA, slot, -1, -1, 0, 4, 0, e->loc);

            int L_true  = new_label(fn);
            int L_false = new_label(fn);
            int L_done  = new_label(fn);

            IRValue lv = lower_expr(fn, st, e->u.bin.l);
            emit_cbr(fn, lv, L_true, L_false, e->loc);

            emit_label(fn, L_true, e->loc);
            IRValue true_val;
            if (bop == BOP_OR) {
                /* a != 0 → result is 1, b not evaluated (short-circuit). */
                true_val = new_value(fn);
                emit_inst_w(fn, IR_CONST, true_val, -1, -1, 1, 4, 0, e->loc);
            } else {
                IRValue rv = lower_expr(fn, st, e->u.bin.r);
                int rw = get_value_width(fn, rv), ru = get_value_is_unsigned(fn, rv);
                IRValue ri = coerce(fn, rv, rw, ru, 4, 0, e->loc);
                IRValue zero = new_value(fn);
                emit_inst_w(fn, IR_CONST, zero, -1, -1, 0, 4, 0, e->loc);
                true_val = emit_bin_w(fn, IR_NE, ri, zero, 4, 0, e->loc);
            }
            emit_inst_w(fn, IR_STORE, -1, slot, true_val, 0, 4, 0, e->loc);
            emit_br(fn, L_done, e->loc);

            emit_label(fn, L_false, e->loc);
            IRValue false_val;
            if (bop == BOP_AND) {
                /* a == 0 → result is 0, b not evaluated (short-circuit). */
                false_val = new_value(fn);
                emit_inst_w(fn, IR_CONST, false_val, -1, -1, 0, 4, 0, e->loc);
            } else {
                IRValue rv = lower_expr(fn, st, e->u.bin.r);
                int rw = get_value_width(fn, rv), ru = get_value_is_unsigned(fn, rv);
                IRValue ri = coerce(fn, rv, rw, ru, 4, 0, e->loc);
                IRValue zero = new_value(fn);
                emit_inst_w(fn, IR_CONST, zero, -1, -1, 0, 4, 0, e->loc);
                false_val = emit_bin_w(fn, IR_NE, ri, zero, 4, 0, e->loc);
            }
            emit_inst_w(fn, IR_STORE, -1, slot, false_val, 0, 4, 0, e->loc);
            emit_br(fn, L_done, e->loc);

            emit_label(fn, L_done, e->loc);
            IRValue result = new_value(fn);
            emit_inst_w(fn, IR_LOAD, result, slot, -1, 0, 4, 0, e->loc);
            return result;
        }
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
        int lf = get_value_is_float(fn, l), rf = get_value_is_float(fn, r);

        int is_cmp = (e->u.bin.op >= BOP_EQ && e->u.bin.op <= BOP_GE);
        /* For arithmetic the result type follows e->type; for comparisons the
         * result is always int, but a float comparison (either operand float)
         * must lower to IR_FCMP, not integer IR_GT/etc. */
        int res_float = (e->type.kind == TY_FLOAT) || (is_cmp && (lf || rf));
        /* Operand width for the op = e->type for arith; UAC result for cmp. */
        int op_w, op_u;
        if (is_cmp) {
            /* Reapply UAC to operand types to know cmp width/signedness. */
            int aw = lf ? lw : (lw < 4 ? 4 : lw), au = lf ? 0 : (lw < 4 ? 0 : lu);
            int bw = rf ? rw : (rw < 4 ? 4 : rw), bu = rf ? 0 : (rw < 4 ? 0 : ru);
            if (lf || rf) {
                /* Float comparison: width = max float width, neither signed. */
                op_w = aw > bw ? aw : bw; op_u = 0;
            } else if (aw == bw && au == bu) { op_w = aw; op_u = au; }
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
        IRValue pl, pr;
        if (res_float) {
            /* Coerce both operands to the result float width. */
            pl = lf ? (lw == op_w ? l : convert_numeric(fn, l, lw, op_w, 0, 1, e->loc))
                    : convert_numeric(fn, l, lw, op_w, 0, 1, e->loc);
            pr = rf ? (rw == op_w ? r : convert_numeric(fn, r, rw, op_w, 0, 1, e->loc))
                    : convert_numeric(fn, r, rw, op_w, 0, 1, e->loc);
        } else {
            pl = coerce(fn, l, lw, lu, op_w, op_u, e->loc);
            pr = coerce(fn, r, rw, ru, op_w, op_u, e->loc);
        }
        IROpcode op;
        if (res_float && !is_cmp) {
            switch (e->u.bin.op) {
            case BOP_ADD: op = IR_FADD; break;
            case BOP_SUB: op = IR_FSUB; break;
            case BOP_MUL: op = IR_FMUL; break;
            case BOP_DIV: op = IR_FDIV; break;
            default:      op = IR_FADD; break;
            }
        } else {
            switch (e->u.bin.op) {
            case BOP_ADD:     op = IR_ADD;  break;
            case BOP_SUB:     op = IR_SUB;  break;
            case BOP_MUL:     op = IR_MUL;  break;
            case BOP_DIV:     op = IR_DIV;  break;
            case BOP_MOD:     op = IR_MOD;  break;
            case BOP_EQ:      op = IR_EQ;   break;
            case BOP_NE:      op = IR_NE;   break;
            case BOP_LT:      op = IR_LT;   break;
            case BOP_LE:      op = IR_LE;   break;
            case BOP_GT:      op = IR_GT;   break;
            case BOP_GE:      op = IR_GE;   break;
            case BOP_BITAND:  op = IR_BAND; break;
            case BOP_BITOR:   op = IR_BOR;  break;
            case BOP_BITXOR:  op = IR_BXOR; break;
            case BOP_SHL:     op = IR_SHL;  break;
            case BOP_SHR:     op = IR_SHR;  break;
            default: op = IR_ADD; break;
            }
        }
        int rw_res = is_cmp ? 4 : op_w;
        int ru_res = is_cmp ? 0 : op_u;
        IRValue result;
        if (is_cmp && res_float) {
            /* Float comparison — emit IR_FCMP with the comparison encoded in
             * is_unsigned (0=LT 1=LE 2=GT 3=GE 4=EQ 5=NE).  The BOP ordering
             * (EQ,NE,LT,LE,GT,GE) differs from the encoding order, so remap. */
            static const unsigned char cmp_enc[] = {4,5,0,1,2,3};
            unsigned char enc = cmp_enc[e->u.bin.op - BOP_EQ];
            result = new_value(fn);
            emit_inst_f(fn, IR_FCMP, result, pl, pr, op_w, e->loc);
            fn->insts.data[fn->insts.len - 1].is_unsigned = enc;
            set_value_float(fn, result, 0);  /* result is int 0/1 */
            set_value_type(fn, result, 4, 0);
            return result;
        }
        /* Comparison ops carry operand width/signedness for codegen — we
         * encode it as the instruction's own width for cmps too (used only
         * by cmp encoding). Result value's width/signedness recorded via
         * emit_bin_w's set_value_type. */
        result = emit_bin_w(fn, op, pl, pr, op_w, op_u, e->loc);
        if (res_float) set_value_float(fn, result, 1);
        if (is_cmp) {
            /* Retag the result's own SSA width to int(4,signed). */
            set_value_type(fn, result, rw_res, ru_res);
        }
        return result;
    }
    case EX_TERNARY: {
        /* Lower cond ? then : else to control flow writing a temporary
         * alloca, then let mem2reg promote it into a φ-merged SSA value
         * (no IR_PHI opcode needed).
         *   c ? t : e  →  slot = alloca; if (c) goto L_then; goto L_else;
         *                  L_then: store slot = t; goto L_done;
         *                  L_else: store slot = e; goto L_done;
         *                  L_done: result = load slot */
        int rw = e->type.width ? e->type.width : 4;
        int ru = e->type.is_unsigned;
        IRValue slot = new_value(fn);
        emit_inst_w(fn, IR_ALLOCA, slot, -1, -1, 0, rw, ru, e->loc);
        int L_then = new_label(fn);
        int L_else = new_label(fn);
        int L_done = new_label(fn);
        IRValue cond = lower_expr(fn, st, e->u.tern.cond);
        emit_cbr(fn, cond, L_then, L_else, e->loc);
        emit_label(fn, L_then, e->loc);
        IRValue tv = lower_expr(fn, st, e->u.tern.then);
        int tw = get_value_width(fn, tv), tu = get_value_is_unsigned(fn, tv);
        IRValue tc = coerce(fn, tv, tw, tu, rw, ru, e->loc);
        emit_inst_w(fn, IR_STORE, -1, slot, tc, 0, rw, ru, e->loc);
        emit_br(fn, L_done, e->loc);
        emit_label(fn, L_else, e->loc);
        IRValue ev = lower_expr(fn, st, e->u.tern.else_);
        int ew = get_value_width(fn, ev), eu = get_value_is_unsigned(fn, ev);
        IRValue ec = coerce(fn, ev, ew, eu, rw, ru, e->loc);
        emit_inst_w(fn, IR_STORE, -1, slot, ec, 0, rw, ru, e->loc);
        emit_br(fn, L_done, e->loc);
        emit_label(fn, L_done, e->loc);
        IRValue result = new_value(fn);
        emit_inst_w(fn, IR_LOAD, result, slot, -1, 0, rw, ru, e->loc);
        return result;
    }
    case EX_VAR: {
        const IRSlot *entry = irsymtable_find(st, e->u.var.name);
        if (!entry) {
            /* Not a variable — is it a function name?  A function lvalue
             * (e.g. `add` used as a value, or `&add`) decays to a function
             * pointer; emit an FADDR so the function's address is loaded
             * (patched against the code symbol table, not .data). */
            if (g_ir_tu) {
                for (size_t i = 0; i < g_ir_tu->functions.len; i++) {
                    if (strcmp(g_ir_tu->functions.data[i].name, e->u.var.name) == 0) {
                        IRValue v = new_value(fn);
                        emit_inst_w(fn, IR_FADDR, v, -1, -1, 0, 8, 1, e->loc);
                        fn->insts.data[fn->insts.len - 1].call_name = xstrdup(e->u.var.name);
                        return v;
                    }
                }
            }
            return -1;
        }
        if (entry->is_global) {
            IRValue addr = emit_gaddr(fn, slot_global_name(entry), e->loc);
            if (entry->ty.kind == TY_ARRAY || entry->ty.kind == TY_STRUCT) return addr;
            IRValue v = new_value(fn);
            emit_inst_w(fn, IR_LOAD_PTR, v, addr, -1, 0,
                        entry->width, entry->is_unsigned, e->loc);
            if (entry->ty.kind == TY_FLOAT) set_value_float(fn, v, 1);
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
            if (entry->ty.kind == TY_FLOAT) set_value_float(fn, v, 1);
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
        int rf = get_value_is_float(fn, rv);
        int lw = lv->type.kind == TY_PTR ? 8 : (lv->type.width ? lv->type.width : 4);
        int lu = lv->type.is_unsigned;
        int lf = (lv->type.kind == TY_FLOAT);
        IRValue coerced;
        if (lv->type.is_bool) {
            IRValue n = bool_normalize(fn, rv, rw, ru, rf, e->loc);
            coerced = coerce(fn, n, 4, 0, lw, lu, e->loc);
        } else if (rf != lf)
            coerced = convert_numeric(fn, rv, rw, lw, lu, lf, e->loc);
        else if (lf)
            coerced = (rw == lw) ? rv : convert_numeric(fn, rv, rw, lw, lu, 1, e->loc);
        else
            coerced = coerce(fn, rv, rw, ru, lw, lu, e->loc);
        /* Slice 13 — struct lvalue: both sides are pointers (a struct value is
         * its byte address), so copy the bytes instead of storing the pointer. */
        if (lv->type.kind == TY_STRUCT) {
            IRValue dst;
            if (lv->kind == EX_VAR) {
                const IRSlot *entry = irsymtable_find(st, lv->u.var.name);
                if (!entry) return -1;
                if (entry->is_global)
                    dst = emit_gaddr(fn, slot_global_name(entry), e->loc);
                else
                    dst = emit_bin_w(fn, IR_ADDR, entry->slot, -1, 8, 1, e->loc);
            } else if (lv->kind == EX_DEREF) {
                dst = lower_expr(fn, st, lv->u.deref.operand);
            } else if (lv->kind == EX_INDEX || lv->kind == EX_MEMBER) {
                dst = lower_lvalue_addr(fn, st, lv);
            } else {
                return coerced;
            }
            emit_struct_copy(fn, dst, rv, type_size(lv->type), e->loc);
            return coerced;
        }
        if (lv->kind == EX_VAR) {
            const IRSlot *entry = irsymtable_find(st, lv->u.var.name);
            if (!entry) return -1;
            if (entry->is_global) {
                IRValue addr = emit_gaddr(fn, slot_global_name(entry), e->loc);
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
            int bit_width = 0, bit_offset = 0, unit_width = 0;
            if (lv->kind == EX_MEMBER
                && member_bitfield(lv, &bit_width, &bit_offset, &unit_width)) {
                /* Bitfield store: read-modify-write the storage unit.
                 * unit = load(addr);
                 * unit = unit & ~(mask << bit_offset);   // clear the field
                 * unit = unit | ((val & mask) << bit_offset); // set the field
                 * store(addr, unit); */
                int uw = unit_width ? unit_width : 4;
                IRValue unit = new_value(fn);
                emit_inst_w(fn, IR_LOAD_PTR, unit, addr, -1, 0, uw, 1, e->loc);
                int mask = (1 << bit_width) - 1;
                /* Clear the field bits: unit &= ~(mask << bit_offset). */
                IRValue m = new_value(fn);
                emit_inst_w(fn, IR_CONST, m, -1, -1, mask, uw, 1, e->loc);
                IRValue so = new_value(fn);
                emit_inst_w(fn, IR_CONST, so, -1, -1, bit_offset, 8, 1, e->loc);
                IRValue ms = new_value(fn);
                emit_inst_w(fn, IR_SHL, ms, m, so, 0, uw, 1, e->loc);
                IRValue nm = new_value(fn);
                emit_inst_w(fn, IR_BNOT, nm, ms, -1, 0, uw, 1, e->loc);
                IRValue cleared = new_value(fn);
                emit_inst_w(fn, IR_BAND, cleared, unit, nm, 0, uw, 1, e->loc);
                /* Position the new value's bits: (val & mask) << bit_offset. */
                IRValue vm = new_value(fn);
                emit_inst_w(fn, IR_BAND, vm, coerced, m, 0, uw, 1, e->loc);
                IRValue vs = new_value(fn);
                emit_inst_w(fn, IR_SHL, vs, vm, so, 0, uw, 1, e->loc);
                IRValue merged = new_value(fn);
                emit_inst_w(fn, IR_BOR, merged, cleared, vs, 0, uw, 1, e->loc);
                emit_inst_w(fn, IR_STORE_PTR, -1, addr, merged, 0, uw, 1, e->loc);
                return coerced;
            }
            emit_inst_w(fn, IR_STORE_PTR, -1, addr, coerced, 0, lw, lu, e->loc);
            return coerced;
        }
        return coerced;
    }
    case EX_CALL: {
        /* The va_start / va_arg / va_end builtins.  They are lowered to a
         * named IR_CALL (call_name = the builtin) so codegen can dispatch by
         * name, exactly like __syscall.  The va_list is already a pointer to
         * the struct's bytes (fakecc's struct-as-pointer value model), so the
         * first arg lowers directly to the struct base address. */
        if (e->u.call.callee->kind == EX_VAR) {
            const char *cname = e->u.call.callee->u.var.name;
            if (strcmp(cname, "va_start") == 0 || strcmp(cname, "va_end") == 0
                || strcmp(cname, "va_arg") == 0) {
                IRValue ap = lower_expr(fn, st, e->u.call.args.data[0]);
                /* va_start's second arg (last named param) must stay live so the
                 * optimizer doesn't eliminate the param as dead.  Lower it and
                 * keep it in call_args (codegen ignores it). */
                IRValue last = -1;
                if (strcmp(cname, "va_start") == 0
                    && e->u.call.args.len >= 2)
                    last = lower_expr(fn, st, e->u.call.args.data[1]);
                IRInst inst;
                inst.op = IR_CALL;
                inst.a = -1; inst.b = -1; inst.imm = 0;
                inst.loc = e->loc;
                inst.call_name = xstrdup(cname);
                inst.call_callee = -1;
                inst.call_nargs = (last >= 0) ? 2 : 1;
                inst.call_args[0] = ap;
                inst.call_args[1] = last;
                if (strcmp(cname, "va_arg") == 0) {
                    inst.dst = new_value(fn);
                    inst.width = e->va_arg_type.width ? e->va_arg_type.width : 4;
                    inst.is_unsigned = e->va_arg_type.is_unsigned;
                    inst.is_float = (e->va_arg_type.kind == TY_FLOAT);
                } else {
                    inst.dst = -1;   /* void: va_start / va_end */
                    inst.width = 0;
                    inst.is_unsigned = 0;
                    inst.is_float = 0;
                }
                ir_inst_array_push(&fn->insts, inst);
                if (inst.dst >= 0)
                    set_value_type(fn, inst.dst, inst.width, inst.is_unsigned);
                if (inst.is_float) set_value_float(fn, inst.dst, 1);
                return inst.dst;
            }
        }
        IRValue arg_vals[IR_CALL_MAX_ARGS];
        int nargs = (int)e->u.call.args.len;
        for (int i = 0; i < nargs; i++)
            arg_vals[i] = lower_expr(fn, st, e->u.call.args.data[i]);
        /* Void call: no result value (width 0).  Still emit the call for its
         * side effects; dst=-1 marks "no result". */
        int is_void = (e->type.width == 0);
        /* Slice 13 — sret: a struct-returning call needs a destination slot.
         * Allocate it, pass &slot as a hidden arg 0, shift real args up, and
         * take the slot address as the call's result "value" (a pointer). */
        int is_ret_struct = (!is_void && e->type.kind == TY_STRUCT);
        IRValue sret_addr = -1;
        if (is_ret_struct) {
            int total = type_size(e->type);
            IRValue slot = emit_alloca(fn, total, 8, 1, e->loc);
            sret_addr = emit_bin_w(fn, IR_ADDR, slot, -1, 8, 1, e->loc);
        }
        /* A struct call produces no scalar result (dst=-1): the bytes land in
         * the slot backing sret_addr, which is also passed as hidden arg 0 and
         * kept live across the call.  The expression's "value" is sret_addr. */
        IRValue v = is_ret_struct ? sret_addr : (is_void ? -1 : new_value(fn));
        int total_nargs = nargs + (is_ret_struct ? 1 : 0);
        IRInst inst;
        inst.op = IR_CALL;
        inst.dst = is_ret_struct ? -1 : v;
        inst.a = -1;
        inst.b = -1;
        inst.imm = 0;
        inst.loc = e->loc;
        inst.call_name = NULL;
        inst.call_callee = -1;
        /* Direct named call: callee is `EX_VAR` whose name is a known function
         * in the TU (matches sema's direct-call path).  Otherwise it's an
         * indirect call through a function pointer. */
        if (e->u.call.callee->kind == EX_VAR) {
            const char *cname = e->u.call.callee->u.var.name;
            /* `__syscall` is an intrinsic — treat it as a named call so codegen
             * emits the raw `syscall` instruction.  It is not in the function
             * table, so the direct-call lookup below would miss it. */
            if (strcmp(cname, "__syscall") == 0) {
                inst.call_name = xstrdup(cname);
            } else if (strcmp(cname, "__builtin_ctzll") == 0) {
                /* ctz intrinsic — codegen emits a `bsf` + fixup. */
                inst.call_name = xstrdup(cname);
            } else {
                int is_direct = 0;
                if (g_ir_tu) {
                    for (size_t i = 0; i < g_ir_tu->functions.len; i++) {
                        if (strcmp(g_ir_tu->functions.data[i].name, cname) == 0) {
                            is_direct = 1;
                            break;
                        }
                    }
                }
                if (is_direct) {
                    inst.call_name = xstrdup(cname);
                } else {
                    /* Function-pointer variable: the callee is loaded from a slot.
                     * lower_expr returns the loaded SSA value. */
                    inst.call_callee = lower_expr(fn, st, e->u.call.callee);
                }
            }
        } else {
            /* Indirect call: lower the callee expression to an SSA value. */
            inst.call_callee = lower_expr(fn, st, e->u.call.callee);
        }
        inst.call_nargs = total_nargs;
        if (is_ret_struct) {
            inst.call_args[0] = sret_addr;
            for (int i = 0; i < nargs; i++)
                inst.call_args[i + 1] = arg_vals[i];
        } else {
            for (int i = 0; i < nargs; i++) inst.call_args[i] = arg_vals[i];
        }
        /* A struct call's "value" is the destination pointer (width 8). */
        inst.width = is_ret_struct ? 8
                      : (is_void ? 0 : (e->type.width ? e->type.width : 4));
        inst.is_unsigned = is_ret_struct ? 1 : e->type.is_unsigned;
        ir_inst_array_push(&fn->insts, inst);
        if (!is_void) {
            set_value_type(fn, v, inst.width, inst.is_unsigned);
            if (e->type.kind == TY_FLOAT)
                set_value_float(fn, v, 1);
        }
        return v;
    }
    case EX_ADDR: {
        Expr *op = e->u.addr.operand;
        if (op->kind == EX_VAR) {
            const IRSlot *entry = irsymtable_find(st, op->u.var.name);
            if (!entry) {
                /* Not a variable — must be a function name (`&func`).  Emit an
                 * FADDR so the function's address is loaded into a register
                 * (patched against the code symbol table, not .data). */
                IRValue v = new_value(fn);
                emit_inst_w(fn, IR_FADDR, v, -1, -1, 0, 8, 1, e->loc);
                fn->insts.data[fn->insts.len - 1].call_name = xstrdup(op->u.var.name);
                return v;
            }
            if (entry->is_global) return emit_gaddr(fn, slot_global_name(entry), e->loc);
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
        /* Function lvalue (`*fp` where fp : ptr(func)): no load — the function
         * pointer value IS the caldehyde; it decays at the call site. */
        if (e->type.kind == TY_FUNC) return ptr;
        int w = e->type.kind == TY_PTR ? 8 : (e->type.width ? e->type.width : 4);
        int u = e->type.is_unsigned;
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, v, ptr, -1, 0, w, u, e->loc);
        if (e->type.kind == TY_FLOAT) set_value_float(fn, v, 1);
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
        if (e->type.kind == TY_FLOAT) set_value_float(fn, v, 1);
        return v;
    }
    case EX_MEMBER: {
        IRValue addr = lower_lvalue_addr(fn, st, e);
        /* Struct/array members: return address (decay). Scalar/pointer: load. */
        if (e->type.kind == TY_STRUCT) return addr;
        if (e->type.kind == TY_ARRAY)  return addr;
        int bit_width = 0, bit_offset = 0, unit_width = 0;
        int is_bf = member_bitfield(e, &bit_width, &bit_offset, &unit_width);
        /* Bitfields are loaded as their storage unit (e.g. 4-byte int), then
         * shifted/masked to the member's declared width below. */
        int w = is_bf ? (unit_width ? unit_width : 4)
                      : (e->type.kind == TY_PTR ? 8 : (e->type.width ? e->type.width : 4));
        int u = is_bf ? 1 : e->type.is_unsigned;  /* bitfields read as unsigned unit */
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, v, addr, -1, 0, w, u, e->loc);
        if (is_bf) {
            /* Extract the bitfield: v = (v >> bit_offset) & ((1<<bit_width)-1). */
            if (bit_offset > 0) {
                IRValue s = new_value(fn);
                emit_inst_w(fn, IR_CONST, s, -1, -1, bit_offset, 8, 1, e->loc);
                IRValue shifted = new_value(fn);
                emit_inst_w(fn, IR_SHR, shifted, v, s, 0, w, 1, e->loc);
                v = shifted;
            }
            if (bit_width < w * 8) {
                int mask = (1 << bit_width) - 1;
                IRValue m = new_value(fn);
                emit_inst_w(fn, IR_CONST, m, -1, -1, mask, w, 1, e->loc);
                IRValue masked = new_value(fn);
                emit_inst_w(fn, IR_BAND, masked, v, m, 0, w, 1, e->loc);
                v = masked;
            }
            /* Result is a small unsigned int; coerce to the member's declared
             * width (semantically int/bool). */
            return coerce(fn, v, w, u, e->type.width ? e->type.width : 4,
                         e->type.is_unsigned, e->loc);
        }
        if (e->type.kind == TY_FLOAT) set_value_float(fn, v, 1);
        return v;
    }
    case EX_CAST: {
        IRValue x = lower_expr(fn, st, e->u.cast.operand);
        int sw = get_value_width(fn, x), su = get_value_is_unsigned(fn, x);
        int sf = get_value_is_float(fn, x);
        int dw = e->type.kind == TY_PTR ? 8 : (e->type.width ? e->type.width : 4);
        int du = e->type.kind == TY_PTR ? 1 : e->type.is_unsigned;
        int df = (e->type.kind == TY_FLOAT);
        if (e->type.is_bool) {
            IRValue n = bool_normalize(fn, x, sw, su, sf, e->loc);
            return coerce(fn, n, 4, 0, dw, du, e->loc);
        }
        if (sf != df) {
            /* Cross-domain cast: float ↔ int (SITOFP / FPTOSI). */
            return convert_numeric(fn, x, sw, dw, du, df, e->loc);
        }
        if (df) {
            /* float ↔ float width change (float ↔ double). */
            if (sw == dw) return x;
            return convert_numeric(fn, x, sw, dw, du, 1, e->loc);
        }
        return coerce(fn, x, sw, su, dw, du, e->loc);
    }
    case EX_INC_DEC: {
        /* ++lvalue / --lvalue (prefix or postfix).
         * Sema guarantees operand is an lvalue of int or pointer type.
         * Step = 1 for int, sizeof(pointee) for pointer. */
        Expr *lv = e->u.incdec.operand;
        int is_inc = e->u.incdec.is_inc;
        int is_prefix = e->u.incdec.is_prefix;
        int is_ptr = (lv->type.kind == TY_PTR);
        int step = is_ptr ? type_size(*lv->type.pointee) : 1;
        int lw = is_ptr ? 8 : (lv->type.width ? lv->type.width : 4);
        int lu = is_ptr ? 1 : lv->type.is_unsigned;

        /* Promotable path: simple non-pinned, non-global scalar variable.
         * Use IR_LOAD/IR_STORE on the slot so mem2reg can still promote it. */
        if (lv->kind == EX_VAR) {
            const IRSlot *entry = irsymtable_find(st, lv->u.var.name);
            if (entry && !entry->is_global && !entry->pinned) {
                IRValue old = new_value(fn);
                emit_inst_w(fn, IR_LOAD, old, entry->slot, -1, 0, lw, lu, e->loc);
                IRValue s = new_value(fn);
                emit_inst_w(fn, IR_CONST, s, -1, -1, step, lw, lu, e->loc);
                IRValue neu = emit_bin_w(fn, is_inc ? IR_ADD : IR_SUB,
                                         old, s, lw, lu, e->loc);
                emit_inst_w(fn, IR_STORE, -1, entry->slot, neu, 0, lw, lu, e->loc);
                return is_prefix ? neu : old;
            }
        }

        /* General path (pinned var, global, deref, index, member): go
         * through the address so the store side-effect reaches memory. */
        IRValue addr = lower_lvalue_addr(fn, st, lv);
        IRValue old = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, old, addr, -1, 0, lw, lu, e->loc);
        IRValue s = new_value(fn);
        emit_inst_w(fn, IR_CONST, s, -1, -1, step, lw, lu, e->loc);
        IRValue neu = emit_bin_w(fn, is_inc ? IR_ADD : IR_SUB,
                                 old, s, lw, lu, e->loc);
        emit_inst_w(fn, IR_STORE_PTR, -1, addr, neu, 0, lw, lu, e->loc);
        return is_prefix ? neu : old;
    }
    case EX_COMPOUND_ASSIGN: {
        /* lvalue op= rvalue  →  read lvalue, compute lvalue op rvalue, store
         * back. The lvalue address is evaluated once. For pointers, += / -=
         * scale the rvalue by sizeof(pointee). */
        Expr *lv = e->u.comp.lvalue;
        BinOp op = e->u.comp.op;
        int is_ptr = (lv->type.kind == TY_PTR);
        int lw = is_ptr ? 8 : (lv->type.width ? lv->type.width : 4);
        int lu = is_ptr ? 1 : lv->type.is_unsigned;
        IROpcode ir_op = bop_to_ir(op);

        /* Promotable path: simple non-pinned, non-global scalar variable. */
        if (lv->kind == EX_VAR) {
            const IRSlot *entry = irsymtable_find(st, lv->u.var.name);
            if (entry && !entry->is_global && !entry->pinned) {
                IRValue old = new_value(fn);
                emit_inst_w(fn, IR_LOAD, old, entry->slot, -1, 0, lw, lu, e->loc);
                IRValue rhs = lower_expr(fn, st, e->u.comp.rvalue);
                IRValue scaled = scale_rhs(fn, rhs, is_ptr, lv->type, op, e->loc);
                IRValue neu = emit_bin_w(fn, ir_op, old, scaled, lw, lu, e->loc);
                emit_inst_w(fn, IR_STORE, -1, entry->slot, neu, 0, lw, lu, e->loc);
                return neu;
            }
        }

        /* General path (pinned var, global, deref, index, member). */
        IRValue addr = lower_lvalue_addr(fn, st, lv);
        IRValue old = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, old, addr, -1, 0, lw, lu, e->loc);
        IRValue rhs = lower_expr(fn, st, e->u.comp.rvalue);
        IRValue scaled = scale_rhs(fn, rhs, is_ptr, lv->type, op, e->loc);
        IRValue neu = emit_bin_w(fn, ir_op, old, scaled, lw, lu, e->loc);
        emit_inst_w(fn, IR_STORE_PTR, -1, addr, neu, 0, lw, lu, e->loc);
        return neu;
    }
    case EX_COMMA: {
        /* a, b: evaluate a for side effects (discard result), then b. */
        lower_expr(fn, st, e->u.comma.lhs);
        return lower_expr(fn, st, e->u.comma.rhs);
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
    case EX_ALIGNOF_TYPE: {
        /* _Alignof(T) — compile-time alignment, lowered like sizeof but using
         * type_align() instead of type_size(). */
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_CONST, v, -1, -1, type_align(e->u.alignof_t.target),
                    8, 1, e->loc);
        return v;
    }
    case EX_COMPOUND_LITERAL: {
        /* (Type){ ... }: materialize stack storage, initialize it, and return a
         * pointer to it (aggregates) or the loaded scalar value (scalars).  The
         * sema pass has already inferred any unspecified array length, so
         * type_size(target) is authoritative. */
        Type target = e->u.compound.target_type;
        int total = type_size(target);
        if (total <= 0) total = 8;
        IRValue slot = emit_alloca(fn, total, 8, 1, e->loc);
        IRValue addr = emit_bin_w(fn, IR_ADDR, slot, -1, 8, 1, e->loc);
        lower_init_list(fn, st, addr, &target, e->u.compound.init, e->loc);
        if (target.kind == TY_ARRAY || target.kind == TY_STRUCT)
            return addr;    /* aggregate value is its byte address, like struct/array vars */
        /* Scalar: load the value back so the literal behaves as an rvalue. */
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, v, addr, -1, 0,
                    target.width ? target.width : 4, target.is_unsigned, e->loc);
        if (target.kind == TY_FLOAT) set_value_float(fn, v, 1);
        return v;
    }
    default: break;   /* EX_INIT_LIST is lowered in ST_DECL, not here */
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

/* Label map: label name → IR label id.  Populated by a pre-pass over the
 * function body so forward gotos resolve to ids assigned before lowering. */
typedef struct {
    char **names;
    int   *ids;
    size_t len;
    size_t cap;
} LabelMap;

static void labelmap_init(LabelMap *lm) {
    lm->names = NULL; lm->ids = NULL; lm->len = 0; lm->cap = 0;
}

static void labelmap_free(LabelMap *lm) {
    for (size_t i = 0; i < lm->len; i++) free(lm->names[i]);
    free(lm->names); free(lm->ids);
    lm->names = NULL; lm->ids = NULL; lm->len = 0; lm->cap = 0;
}

static int labelmap_find(const LabelMap *lm, const char *name) {
    for (size_t i = 0; i < lm->len; i++)
        if (strcmp(lm->names[i], name) == 0) return lm->ids[i];
    return -1;
}

static void labelmap_add(LabelMap *lm, const char *name, int id) {
    if (lm->len >= lm->cap) {
        lm->cap = lm->cap ? lm->cap * 2 : 8;
        lm->names = realloc(lm->names, lm->cap * sizeof(char *));
        lm->ids   = realloc(lm->ids, lm->cap * sizeof(int));
        if (!lm->names || !lm->ids) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    }
    lm->names[lm->len] = xstrdup(name);
    lm->ids[lm->len] = id;
    lm->len++;
}

/* Recursively assign a label id to every ST_LABEL in a statement. */
static void assign_label_ids(IRFunction *fn, LabelMap *lm, const Stmt *s) {
    if (!s) return;
    switch (s->kind) {
    case ST_LABEL: {
        int id = new_label(fn);
        labelmap_add(lm, s->u.label_s.name, id);
        assign_label_ids(fn, lm, s->u.label_s.stmt);
        break;
    }
    case ST_IF:
        assign_label_ids(fn, lm, s->u.if_s.then_s);
        if (s->u.if_s.else_s) assign_label_ids(fn, lm, s->u.if_s.else_s);
        break;
    case ST_WHILE:
        assign_label_ids(fn, lm, s->u.while_s.body);
        break;
    case ST_DO_WHILE:
        assign_label_ids(fn, lm, s->u.do_s.body);
        break;
    case ST_FOR:
        if (s->u.for_s.init) assign_label_ids(fn, lm, s->u.for_s.init);
        assign_label_ids(fn, lm, s->u.for_s.body);
        break;
    case ST_BLOCK:
        for (size_t i = 0; i < s->u.block.len; i++)
            assign_label_ids(fn, lm, &s->u.block.data[i]);
        break;
    default:
        break;
    }
}

/* File-scope pointer to the current function's label map, consulted by
 * lower_stmt when lowering ST_LABEL / ST_GOTO. */
static LabelMap *g_ir_label_map = NULL;

/* Lower a single statement, emitting instructions as needed. */
static void lower_stmt(IRFunction *fn, IRSymTable *st, const Stmt *s,
                       const FunctionDecl *cur_fd);

static void lower_stmt(IRFunction *fn, IRSymTable *st, const Stmt *s,
                       const FunctionDecl *cur_fd) {
    switch (s->kind) {
    case ST_DECL: {
        Type dty = s->u.decl.type;
        int dw = dty.kind == TY_ARRAY ? type_size(*dty.elem_type)
                : (dty.kind == TY_PTR ? 8
                   : (dty.kind == TY_STRUCT ? 8 : dty.width));
        int du = dty.is_unsigned;

        /* static local → allocate in static storage via a mangled global
         * `fn.varname`.  Reads/writes resolve through the global_name path. */
        if (s->u.decl.storage_class == 1) {
            int sz = type_size(dty);
            if (sz <= 0) sz = 8;
            char *bytes = calloc(sz, 1);
            if (!bytes) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
            if (s->u.decl.init)
                pack_init(g_ir_module, &dty, s->u.decl.init, bytes, sz,
                          s->u.decl.name, s->loc);
            char mangled[256];
            snprintf(mangled, sizeof mangled, "%s.%s", cur_fd->name,
                     s->u.decl.name);
            /* mangled name is file-local by construction → LOCAL linkage */
            ir_module_push_global(g_ir_module, mangled, sz, bytes, 0, 1,
                                  s->loc);
            irsymtable_push_static_local(st, s->u.decl.name, mangled, dty);
            break;
        }

        int pinned = is_pinned_in_body(cur_fd, s->u.decl.name, dty);
        /* Function-pointer variables are always pinned: their value must
         * survive in memory so indirect calls can load them.  Without this,
         * mem2reg promotes them and the store/load chain breaks. */
        if (dty.kind == TY_PTR && dty.pointee && dty.pointee->kind == TY_FUNC)
            pinned = 1;
        /* Float variables are always pinned too: their SSA values live in the
         * XMM register file (separate from the GP file the scalar regalloc
         * targets), so mem2reg must not promote them.  Codegen loads/stores
         * through the pinned slot on every use. */
        if (dty.kind == TY_FLOAT)
            pinned = 1;
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
            if (s->u.decl.init->kind == EX_INIT_LIST) {
                if (dty.kind == TY_ARRAY || dty.kind == TY_STRUCT) {
                    IRValue addr = emit_bin_w(fn, IR_ADDR, v, -1, 8, 1, s->loc);
                    lower_init_list(fn, st, addr, &dty, s->u.decl.init, s->loc);
                } else {
                    /* Scalar target with a single-element brace list
                     * (`int x = {5}`): store element[0]. */
                    IRValue rv = lower_expr(fn, st,
                                            s->u.decl.init->u.init_list.elements[0]);
                    int rw = get_value_width(fn, rv), ru = get_value_is_unsigned(fn, rv);
                    IRValue coerced = coerce(fn, rv, rw, ru, dw, du, s->loc);
                    if (pinned) {
                        IRValue addr = emit_bin_w(fn, IR_ADDR, v, -1, 8, 1, s->loc);
                        emit_inst_w(fn, IR_STORE_PTR, -1, addr, coerced, 0, dw, du, s->loc);
                    } else {
                        emit_inst_w(fn, IR_STORE, -1, v, coerced, 0, dw, du, s->loc);
                    }
                }
            } else {
                IRValue rv = lower_expr(fn, st, s->u.decl.init);
                /* Slice 13 — struct init from an expression (e.g. `struct S r =
                 * make(3,4)`): both sides are pointers; copy the bytes instead
                 * of storing the pointer. */
                if (dty.kind == TY_STRUCT) {
                    IRValue addr = emit_bin_w(fn, IR_ADDR, v, -1, 8, 1, s->loc);
                    emit_struct_copy(fn, addr, rv, type_size(dty), s->loc);
                    break;
                }
                int rw = get_value_width(fn, rv), ru = get_value_is_unsigned(fn, rv);
                int rf = get_value_is_float(fn, rv);
                int df = (dty.kind == TY_FLOAT);
                IRValue coerced;
                if (dty.is_bool) {
                    /* _Bool initializer: normalize to 0/1 (width 4) then narrow. */
                    IRValue n = bool_normalize(fn, rv, rw, ru, rf, s->loc);
                    coerced = coerce(fn, n, 4, 0, dw, du, s->loc);
                } else if (rf != df)
                    coerced = convert_numeric(fn, rv, rw, dw, du, df, s->loc);
                else if (df)
                    coerced = (rw == dw) ? rv : convert_numeric(fn, rv, rw, dw, du, 1, s->loc);
                else
                    coerced = coerce(fn, rv, rw, ru, dw, du, s->loc);
                if (pinned) {
                    IRValue addr = emit_bin_w(fn, IR_ADDR, v, -1, 8, 1, s->loc);
                    emit_inst_w(fn, IR_STORE_PTR, -1, addr, coerced, 0, dw, du, s->loc);
                } else {
                    emit_inst_w(fn, IR_STORE, -1, v, coerced, 0, dw, du, s->loc);
                }
            }
        }
        break;
    }
    case ST_EXPR:
        lower_expr(fn, st, s->u.expr);
        break;
    case ST_RETURN: {
        if (fn->ret_width == 0 && !fn->ret_is_float) {
            /* void function: bare `return;` — no value. */
            emit_inst_w(fn, IR_RETURN, -1, -1, -1, 0, 0, 0, s->loc);
        } else if (fn->ret_is_struct) {
            /* Slice 13 — sret: the returned expression lowers to a pointer to
             * the local struct; copy its bytes into the hidden destination
             * (*sret_value) and return the sret pointer in RAX (SysV AMD64
             * struct-return convention). */
            IRValue v = lower_expr(fn, st, s->u.value);
            emit_struct_copy(fn, fn->sret_value, v, fn->ret_width, s->loc);
            emit_inst_w(fn, IR_RETURN, -1, fn->sret_value, -1, 0,
                        8, 1, s->loc);
        } else {
            IRValue v = lower_expr(fn, st, s->u.value);
            int vw = get_value_width(fn, v), vu = get_value_is_unsigned(fn, v);
            int vf = get_value_is_float(fn, v);
            int dw = fn->ret_width, du = fn->ret_is_unsigned;
            int df = fn->ret_is_float;
            IRValue coerced;
            if (fn->ret_is_bool) {
                IRValue n = bool_normalize(fn, v, vw, vu, vf, s->loc);
                coerced = coerce(fn, n, 4, 0, dw, du, s->loc);
            } else if (vf != df)
                coerced = convert_numeric(fn, v, vw, dw, du, df, s->loc);
            else if (df)
                coerced = (vw == dw) ? v : convert_numeric(fn, v, vw, dw, du, 1, s->loc);
            else
                coerced = coerce(fn, v, vw, vu, dw, du, s->loc);
            emit_inst_w(fn, IR_RETURN, -1, coerced, -1, 0,
                        dw, du, s->loc);
        }
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
    case ST_DO_WHILE: {
        /* Lowering:
         *   L_body: body
         *           goto L_cond
         *   L_cond: [cond ? goto L_body : goto L_exit]
         *   L_exit:
         * continue → L_cond, break → L_exit. */
        int L_body = new_label(fn);
        int L_cond = new_label(fn);
        int L_exit = new_label(fn);
        emit_br(fn, L_body, s->loc);
        emit_label(fn, L_body, s->loc);
        /* Body is its own block (the loop header) so mem2reg places φ
         * nodes here; the back-edge from L_cond merges into it. */
        push_loop(L_cond, L_exit);
        lower_stmt(fn, st, s->u.do_s.body, cur_fd);
        pop_loop();
        emit_br(fn, L_cond, s->loc);
        emit_label(fn, L_cond, s->loc);
        IRValue cond = lower_expr(fn, st, s->u.do_s.cond);
        emit_cbr(fn, cond, L_body, L_exit, s->loc);
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
        if (s->u.for_s.init) {
            if (s->u.for_s.init->kind == ST_BLOCK) {
                /* Multi-declarator init wrapped in a block by the parser.
                 * Lower its stmts directly so the declarations stay in the
                 * for scope (lower_stmt(ST_BLOCK) would pop them, hiding the
                 * names from cond/step/body). */
                StmtArray *blk = &s->u.for_s.init->u.block;
                for (size_t i = 0; i < blk->len; i++)
                    lower_stmt(fn, st, &blk->data[i], cur_fd);
            } else {
                lower_stmt(fn, st, s->u.for_s.init, cur_fd);
            }
        }
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
    case ST_GOTO: {
        int id = labelmap_find(g_ir_label_map, s->u.goto_s.target);
        if (id < 0) {
            fprintf(stderr, "fakecc: goto to unknown label '%s'\n",
                    s->u.goto_s.target);
            exit(1);
        }
        emit_br(fn, id, s->loc);
        break;
    }
    case ST_LABEL: {
        int id = labelmap_find(g_ir_label_map, s->u.label_s.name);
        if (id < 0) {
            fprintf(stderr, "fakecc: label '%s' has no assigned id\n",
                    s->u.label_s.name);
            exit(1);
        }
        emit_label(fn, id, s->loc);
        lower_stmt(fn, st, s->u.label_s.stmt, cur_fd);
        break;
    }
    case ST_SWITCH: {
        /* Lowering (fall-through semantics):
         *   v = <cond>
         *   if (v == val_0) goto L_body_0
         *   if (v == val_1) goto L_body_1
         *   ...
         *   goto L_default_or_exit        // no case matched
         *   L_body_0: <body 0>            // falls through to L_body_1
         *   L_body_1: <body 1>
         *   ...
         *   L_default: <default body>     (only if a default arm exists)
         *   L_exit:
         * break → L_exit.  Bodies are laid out sequentially so a missing
         * break falls through to the next arm. */
        IRValue v = lower_expr(fn, st, s->u.switch_s.cond);
        int vw = get_value_width(fn, v), vu = get_value_is_unsigned(fn, v);

        int n = s->u.switch_s.num_cases;
        int has_default = 0;
        for (int i = 0; i < n; i++)
            if (s->u.switch_s.cases[i].is_default) { has_default = 1; break; }

        /* Pre-create a body label for every arm. */
        int *body_label = xmalloc(n * sizeof(int));
        for (int i = 0; i < n; i++) body_label[i] = new_label(fn);
        int exit_label = new_label(fn);

        /* Collect the non-default arm indices and create a "check" label for
         * each, forming a chain: L_check_i does the comparison and on miss
         * falls through to L_check_{i+1}. */
        int ndispatch = 0;
        for (int i = 0; i < n; i++)
            if (!s->u.switch_s.cases[i].is_default) ndispatch++;

        int *check_label = xmalloc(ndispatch * sizeof(int));
        int *check_case = xmalloc(ndispatch * sizeof(int)); // arm index
        int dc = 0;
        for (int i = 0; i < n; i++) {
            if (s->u.switch_s.cases[i].is_default) continue;
            check_label[dc] = new_label(fn);
            check_case[dc] = i;
            dc++;
        }
        /* The label to jump to when no case matches. */
        int fallthrough_label = has_default ? -1 : exit_label;
        if (has_default) {
            for (int i = 0; i < n; i++)
                if (s->u.switch_s.cases[i].is_default) { fallthrough_label = body_label[i]; break; }
        }

        /* Emit the dispatch chain. */
        for (int c = 0; c < ndispatch; c++) {
            int arm_idx = check_case[c];
            SwitchCase *arm = &s->u.switch_s.cases[arm_idx];
            emit_label(fn, check_label[c], s->loc);
            IRValue cmp_val = new_value(fn);
            emit_inst_w(fn, IR_CONST, cmp_val, -1, -1, arm->value, vw, vu, s->loc);
            IRValue eq = emit_bin_w(fn, IR_EQ, v, cmp_val, vw, vu, s->loc);
            int next = (c + 1 < ndispatch) ? check_label[c + 1] : fallthrough_label;
            emit_cbr(fn, eq, body_label[arm_idx], next, s->loc);
        }

        /* Bodies, laid out sequentially for fall-through.  Push a loop
         * frame so `break` inside any arm jumps to the exit label. */
        push_loop(/*cont*/ exit_label, /*brk*/ exit_label);
        for (int i = 0; i < n; i++) {
            SwitchCase *arm = &s->u.switch_s.cases[i];
            emit_label(fn, body_label[i], s->loc);
            for (size_t j = 0; j < arm->stmts.len; j++)
                lower_stmt(fn, st, &arm->stmts.data[j], cur_fd);
        }
        pop_loop();

        emit_label(fn, exit_label, s->loc);
        free(body_label);
        free(check_label);
        free(check_case);
        break;
    }
    }
}

/* Find a previously-packed global by name (for const-global references in
 * initializers, e.g. `.regs = ALLOCATABLE_REGS`).  Returns NULL if not found. */
static const IRGlobal *find_packed_global(const IRModule *m, const char *name) {
    for (size_t i = 0; i < m->globals.len; i++)
        if (m->globals.data[i].name
            && strcmp(m->globals.data[i].name, name) == 0)
            return &m->globals.data[i];
    return NULL;
}

/* Pack a compile-time constant initializer into `bytes` (size `sz`) for a
 * global of type `ty`.  Handles integer literals, negated literals, nested
 * initializer lists (positional, per-element/per-member offset), string
 * literals for `char[]="..."`, cast expressions, and references to previously
 * packed const globals.  `ir` is the module whose globals may be referenced.
 * Sema guarantees every element is constant. */
static void pack_init(const IRModule *ir, const Type *ty, const Expr *e,
                      char *bytes, int sz, const char *ctx, SourceLoc loc) {
    if (e->kind == EX_INIT_LIST) {
        int n = e->u.init_list.num_elements;
        switch (ty->kind) {
        case TY_ARRAY: {
            int esz = type_size(*ty->elem_type);
            for (int i = 0; i < n; i++)
                pack_init(ir, ty->elem_type, e->u.init_list.elements[i],
                          bytes + i * esz, esz, ctx, loc);
            break;
        }
        case TY_STRUCT: {
            const StructDef *sd = struct_registry_find_c(g_ir_structs, ty->tag);
            if (!sd) die_at(loc.file, loc.line, loc.col,
                            "unknown struct 'struct %s'", ty->tag);
            for (int i = 0; i < n; i++)
                pack_init(ir, &sd->members[i].type, e->u.init_list.elements[i],
                          bytes + sd->members[i].offset,
                          type_size(sd->members[i].type), ctx, loc);
            break;
        }
        default:
            /* Scalar with a single-element brace list: `int x = {5}`. */
            pack_init(ir, ty, e->u.init_list.elements[0], bytes, sz, ctx, loc);
            break;
        }
        return;
    }
    if (e->kind == EX_CAST) {
        /* `(T)expr` — the cast itself is a no-op at the bit level for the
         * cases we accept (e.g. `(int*)0`); pack the operand. */
        pack_init(ir, ty, e->u.cast.operand, bytes, sz, ctx, loc);
        return;
    }
    long long _fold_v;
    if (e->kind == EX_INT_LIT || fold_const_int(e, &_fold_v)) {
        long long v;
        if (e->kind == EX_INT_LIT)
            v = e->u.int_val;
        else
            v = _fold_v; /* already confirmed foldable above */
        if (ty->is_bool)
            v = (v != 0) ? 1 : 0;  /* _Bool: normalize to 0/1 */
        for (int b = 0; b < sz && b < 8; b++)
            bytes[b] = (char)((v >> (8 * b)) & 0xff);
        return;
    }
    if (e->kind == EX_STR && ty->kind == TY_ARRAY && ty->elem_type
        && ty->elem_type->width == 1) {
        /* char[] = "..." — copy bytes (excluding the lexer's implicit NUL,
         * which we re-add) and NUL-terminate. */
        int n = e->u.str.len;
        if (n > sz - 1) n = sz - 1;
        memcpy(bytes, e->u.str.bytes, n);
        bytes[n] = '\0';
        return;
    }
    /* Reference to a previously-defined const global: `.regs = ALLOCATABLE_REGS`.
     * Copy its already-packed bytes (truncated/padded to `sz`). */
    if (e->kind == EX_VAR && ir) {
        const IRGlobal *g = find_packed_global(ir, e->u.var.name);
        if (g) {
            int n = sz < g->size ? sz : g->size;
            memcpy(bytes, g->init_bytes, n);
            return;
        }
    }
    die_at(loc.file, loc.line, loc.col,
           "global '%s' initializer must be a compile-time constant", ctx);
}

/* Recursively assign concrete byte offsets for a (possibly nested) scalar
 * initializer list element when the target is an array/struct.  Used by the
 * local (non-global) ST_DECL lowering to emit per-element stores.  `base` is
 * the pointer value to the object; each element is stored at base+offset. */
static void lower_init_list(IRFunction *fn, IRSymTable *st, IRValue base,
                            const Type *ty, const Expr *e, SourceLoc loc) {
    if (e->kind == EX_INIT_LIST) {
        int n = e->u.init_list.num_elements;
        switch (ty->kind) {
        case TY_ARRAY: {
            int esz = type_size(*ty->elem_type);
            for (int i = 0; i < n; i++) {
                IRValue off = new_value(fn);
                emit_inst_w(fn, IR_CONST, off, -1, -1, i * esz, 8, 1, loc);
                IRValue ptr = emit_bin_w(fn, IR_ADD, base, off, 8, 1, loc);
                lower_init_list(fn, st, ptr, ty->elem_type,
                                e->u.init_list.elements[i], loc);
            }
            break;
        }
        case TY_STRUCT: {
            const StructDef *sd = struct_registry_find_c(g_ir_structs, ty->tag);
            if (!sd) die_at(loc.file, loc.line, loc.col,
                            "unknown struct 'struct %s'", ty->tag);
            for (int i = 0; i < n; i++) {
                IRValue off = new_value(fn);
                emit_inst_w(fn, IR_CONST, off, -1, -1,
                            sd->members[i].offset, 8, 1, loc);
                IRValue ptr = emit_bin_w(fn, IR_ADD, base, off, 8, 1, loc);
                lower_init_list(fn, st, ptr, &sd->members[i].type,
                                e->u.init_list.elements[i], loc);
            }
            break;
        }
        default:
            lower_init_list(fn, st, base, ty, e->u.init_list.elements[0], loc);
            break;
        }
        return;
    }
    /* Scalar element: lower, coerce to the target width, store via pointer. */
    IRValue rv = lower_expr(fn, st, e);
    int rw = get_value_width(fn, rv), ru = get_value_is_unsigned(fn, rv);
    int sw = ty->kind == TY_PTR ? 8 : (ty->width ? ty->width : 4);
    int su = ty->is_unsigned;
    IRValue coerced = coerce(fn, rv, rw, ru, sw, su, loc);
    emit_inst_w(fn, IR_STORE_PTR, -1, base, coerced, 0, sw, su, loc);
}

void ir_generate(const TranslationUnit *tu, IRModule *ir) {
    /* Publish module + reset string counter for lower_expr's use. */
    g_ir_module = ir;
    g_str_counter = 0;
    g_ir_structs = &tu->structs;
    g_ir_tu = tu;

    /* Register named globals from tu->globals.  `extern` globals are
     * declarations only — no storage, no emission (single-TU model: they can
     * never be defined, so any use is an unresolved symbol).  `static` globals
     * are emitted normally here (linkage has no effect in a single-TU static
     * ELF, but the storage + initializer are real). */
    for (size_t i = 0; i < tu->globals.len; i++) {
        const Stmt *s = &tu->globals.data[i];
        if (s->kind != ST_DECL) continue;
        /* extern → declaration only: skip emission entirely. */
        if (s->u.decl.storage_class == 2) continue;
        int sz = type_size(s->u.decl.type);
        if (sz <= 0) sz = 8;
        char *bytes = calloc(sz, 1);
        if (!bytes) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        if (s->u.decl.init)
            pack_init(ir, &s->u.decl.type, s->u.decl.init, bytes, sz,
                      s->u.decl.name, s->loc);
        int is_static = (s->u.decl.storage_class == 1);
        ir_module_push_global(ir, s->u.decl.name, sz, bytes, 0, is_static,
                              s->loc);
    }

    for (size_t i = 0; i < tu->functions.len; i++) {
        const FunctionDecl *fd = &tu->functions.data[i];

        /* `extern` declarations have no body — no IR is generated for them. */
        if (fd->is_extern) continue;

        IRFunction irfn;
        irfn.name = xstrdup(fd->name);
        irfn.loc = fd->loc;
        irfn.insts.data = NULL;
        irfn.insts.len = 0;
        irfn.insts.cap = 0;
        irfn.next_value_id = 0;
        irfn.next_label_id = 0;
        irfn.ra = NULL;
        irfn.ra_xmm = NULL;
        irfn.value_width = NULL;
        irfn.value_is_unsigned = NULL;
        irfn.value_is_float = NULL;
        irfn.value_meta_cap = 0;
        irfn.ret_width = fd->ret_type.width;
        irfn.ret_is_unsigned = fd->ret_type.is_unsigned;
        irfn.ret_is_float = (fd->ret_type.kind == TY_FLOAT);
        irfn.ret_is_struct = (fd->ret_type.kind == TY_STRUCT);
        irfn.ret_is_bool = fd->ret_type.is_bool;
        irfn.is_variadic = fd->is_variadic;
        irfn.is_static = fd->is_static;
        irfn.sret_value = -1;

        IRSymTable st;
        irsymtable_init(&st);

        /* Bind globals so function-body EX_VAR lookups find them. */
        for (size_t g = 0; g < tu->globals.len; g++) {
            const Stmt *gs = &tu->globals.data[g];
            if (gs->kind == ST_DECL)
                irsymtable_push_global(&st, gs->u.decl.name, gs->u.decl.type);
        }

        /* Emit all PARAM instructions up front so they stay contiguous at
         * function start (codegen's prologue relies on this layout).  The
         * alloca/addr/store chains follow afterward. */
        IRValue param_vs[64];
        /* Slice 13 — sret: a struct-returning function receives a hidden
         * destination pointer as param 0 (in RDI per SysV AMD64).  Real params
         * are shifted up to indices 1..n. */
        if (irfn.ret_is_struct) {
            irfn.sret_value = new_value(&irfn);
            emit_inst_w(&irfn, IR_PARAM, irfn.sret_value, -1, -1, 0, 8, 1,
                        fd->loc);
        }
        for (size_t p = 0; p < fd->params.len; p++) {
            Type pty = fd->params.data[p].type;
            /* A struct arrives by reference (caller passes &struct), so the
             * incoming param is an 8-byte pointer regardless of struct size. */
            int pw = (pty.kind == TY_PTR || pty.kind == TY_STRUCT) ? 8
                      : (pty.width ? pty.width : 4);
            int pu = (pty.kind == TY_PTR || pty.kind == TY_STRUCT) ? 1
                      : pty.is_unsigned;
            int pidx = (int)p + (irfn.ret_is_struct ? 1 : 0);

            param_vs[p] = new_value(&irfn);
            emit_inst_w(&irfn, IR_PARAM, param_vs[p], -1, -1, pidx, pw, pu,
                        fd->params.data[p].loc);
            if (pty.kind == TY_FLOAT)
                set_value_float(&irfn, param_vs[p], 1);
        }
        for (size_t p = 0; p < fd->params.len; p++) {
            Type pty = fd->params.data[p].type;
            int pw = pty.kind == TY_PTR ? 8 : (pty.width ? pty.width : 4);
            int pu = pty.kind == TY_PTR ? 1 : pty.is_unsigned;
            const char *pname = fd->params.data[p].name;
            int pinned = is_pinned_in_body(fd, pname, pty);
            /* Float params are always pinned: their SSA value lives in the XMM
             * register file (separate from the GP file the scalar regalloc
             * targets), so mem2reg must not promote them.  long double (width 16)
             * is the exception — it lives in a 16-byte SysV stack slot and moves
             * through x87 st0, so it is NOT pinned (copied to an alloca); it is
             * read/written directly at its [rbp+..] stack offset. */
            if (pty.kind == TY_FLOAT && pty.width != 16)
                pinned = 1;

            IRValue slot;
            if (pinned) {
                int total = type_size(pty);
                slot = emit_alloca(&irfn, total, pw, pu, fd->params.data[p].loc);
                IRValue addr = emit_bin_w(&irfn, IR_ADDR, slot, -1, 8, 1,
                                          fd->params.data[p].loc);
                if (pty.kind == TY_STRUCT) {
                    /* Struct by value: the incoming param is a pointer to the
                     * caller's struct — copy the bytes into our local slot
                     * instead of storing the pointer itself. */
                    emit_struct_copy(&irfn, addr, param_vs[p], total,
                                     fd->params.data[p].loc);
                } else {
                    emit_inst_w(&irfn, IR_STORE_PTR, -1, addr, param_vs[p], 0, pw, pu,
                                fd->params.data[p].loc);
                }
            } else {
                slot = new_value(&irfn);
                emit_inst_w(&irfn, IR_ALLOCA, slot, -1, -1, 0, pw, pu,
                            fd->params.data[p].loc);
                emit_inst_w(&irfn, IR_STORE, -1, slot, param_vs[p], 0, pw, pu,
                            fd->params.data[p].loc);
            }
            irsymtable_push(&st, pname, slot, pinned, pty);
        }

        /* Pre-pass: assign label ids to every label in this function so
         * forward gotos resolve. */
        LabelMap lm;
        labelmap_init(&lm);
        g_ir_label_map = &lm;
        for (size_t j = 0; j < fd->body.len; j++)
            assign_label_ids(&irfn, &lm, &fd->body.data[j]);

        for (size_t j = 0; j < fd->body.len; j++) {
            lower_stmt(&irfn, &st, &fd->body.data[j], fd);
        }

        g_ir_label_map = NULL;
        labelmap_free(&lm);

        irsymtable_free(&st);
        ir_func_array_push(&ir->functions, irfn);
    }
}
