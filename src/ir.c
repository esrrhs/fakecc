#include "fakecc/ir.h"
#include "fakecc/ast.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* File-scope handles used by lower_expr when it needs to allocate a new
 * anonymous rodata global for a string literal.  Set at the start of
 * ir_generate; cleared at the end.
 *
 * Every reader lives in this file and sees these definitions directly.  Do
 * not re-declare them `extern` inside a function body: fakecc currently
 * lowers a block-scope `extern` as a fresh zero-initialized local, so such
 * a redeclaration silently shadows the global in the self-hosted build. */
IRModule *g_ir_module = NULL;
int g_str_counter = 0;
int g_flt_counter = 0;
const StructRegistry *g_ir_structs = NULL;   /* set by ir_generate */
const TranslationUnit *g_ir_tu = NULL;       /* set by ir_generate */
static int g_ir_pin_locals = 0;              /* -O0: keep scalars in memory */
static const FunctionDecl *g_ir_cur_fd = NULL; /* set by ir_generate */

/* Forward declarations for LabelMap and g_ir_label_map used by lower_expr
 * and lower_stmt (defined later in this file). */
typedef struct {
    char **names;
    int   *ids;
    size_t len;
    size_t cap;
} LabelMap;
static int labelmap_find(const LabelMap *lm, const char *name);
static LabelMap *g_ir_label_map = NULL;

/* Return the live struct registry during lowering, NULL outside it.
 * type_size() uses this to refresh stale cached struct widths. */
const StructRegistry *get_ir_structs(void) {
    return g_ir_tu ? &g_ir_tu->structs : NULL;
}

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
        for (size_t d = 0; d < m->functions.data[i].num_dbg_vars; d++) {
            IRDebugVar *dv = &m->functions.data[i].dbg_vars[d];
            free(dv->name);
            free(dv->struct_tag);
            for (int mi = 0; mi < dv->num_members; mi++)
                free(dv->members[mi].name);
            free(dv->members);
        }
        free(m->functions.data[i].dbg_vars);
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
        for (int f = 0; f < m->globals.data[i].num_fixups; f++)
            free(m->globals.data[i].fixups[f].sym);
        free(m->globals.data[i].fixups);
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
    g->fixups = NULL;
    g->num_fixups = 0;
    g->cap_fixups = 0;
    return g;
}

/* Record a pointer-slot fixup on global `g`: the slot at byte offset `offset`
 * within g->init_bytes must be patched with the link-time address of symbol
 * `sym` (an array/struct global decaying to a pointer). */
static void add_global_fixup(IRGlobal *g, int offset, const char *sym, int addend) {
    if (g->num_fixups >= g->cap_fixups) {
        size_t nc = g->cap_fixups ? g->cap_fixups * 2 : 4;
        g->fixups = realloc(g->fixups, nc * sizeof(GlobalFixup));
        if (!g->fixups) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        g->cap_fixups = (int)nc;
    }
    g->fixups[g->num_fixups].offset = offset;
    g->fixups[g->num_fixups].sym = xstrdup(sym);
    g->fixups[g->num_fixups].addend = addend;
    g->num_fixups++;
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
    case EX_STMT_EXPR:
        if (e->u.stmt_expr.stmts) {
            for (size_t i = 0; i < e->u.stmt_expr.stmts->len; i++)
                if (stmt_takes_addr_of(&e->u.stmt_expr.stmts->data[i], name)) return 1;
        }
        return 0;
    case EX_LABEL_ADDR:
        return 0; /* &&label references no variable */
    }
    return 0;
}

static int expr_has_label_addr(const Expr *e) {
    if (!e) return 0;
    if (e->kind == EX_LABEL_ADDR) return 1;
    if (e->kind == EX_INIT_LIST) {
        for (int i = 0; i < e->u.init_list.num_elements; i++)
            if (expr_has_label_addr(e->u.init_list.elements[i])) return 1;
    }
    if (e->kind == EX_CAST) return expr_has_label_addr(e->u.cast.operand);
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
        return s->u.goto_s.target_expr && expr_takes_addr_of(s->u.goto_s.target_expr, name);
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

static int64_t bitfield_mask64(int bit_width) {
    if (bit_width <= 0) return 0;
    if (bit_width >= 64) return -1;
    return (1LL << bit_width) - 1;
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
    int off = 0;
    const StructMember *m = struct_lookup_member(g_ir_structs, sd,
                                                 e->u.member.name, &off);
    if (!m || m->bit_width <= 0) return 0;
    *bit_width = m->bit_width;
    *bit_offset = m->bit_offset;
    *unit_width = type_size(m->type);
    return 1;
}

/* Returns 1 if the EX_MEMBER refers to a struct field whose declared type
 * is TY_ARRAY.  This is used to detect array-to-pointer decay that sema
 * performed in-place by overwriting the node's type with TY_PTR: in that
 * case lower_expr must return the member's address (decay) rather than
 * loading the pointer value stored there. */
static int member_field_is_array(const Expr *e) {
    if (e->kind != EX_MEMBER) return 0;
    if (e->u.member.obj->type.kind != TY_STRUCT) return 0;
    const char *tag = e->u.member.obj->type.tag;
    if (!tag) return 0;
    const StructDef *sd = struct_registry_find_c(g_ir_structs, tag);
    if (!sd) return 0;
    const StructMember *m = struct_lookup_member(g_ir_structs, sd,
                                                 e->u.member.name, NULL);
    return m && m->type.kind == TY_ARRAY;
}

static int stmt_has_computed_goto(const Stmt *s) {
    if (!s) return 0;
    switch (s->kind) {
    case ST_DECL: return s->u.decl.init && expr_has_label_addr(s->u.decl.init);
    case ST_EXPR: return expr_has_label_addr(s->u.expr);
    case ST_RETURN: return expr_has_label_addr(s->u.value);
    case ST_GOTO: return s->u.goto_s.target_expr != NULL;
    case ST_IF:
        return (s->u.if_s.cond && expr_has_label_addr(s->u.if_s.cond))
            || stmt_has_computed_goto(s->u.if_s.then_s)
            || (s->u.if_s.else_s && stmt_has_computed_goto(s->u.if_s.else_s));
    case ST_WHILE:
        return (s->u.while_s.cond && expr_has_label_addr(s->u.while_s.cond))
            || stmt_has_computed_goto(s->u.while_s.body);
    case ST_DO_WHILE:
        return (s->u.do_s.cond && expr_has_label_addr(s->u.do_s.cond))
            || stmt_has_computed_goto(s->u.do_s.body);
    case ST_FOR:
        return (s->u.for_s.init && stmt_has_computed_goto(s->u.for_s.init))
            || (s->u.for_s.cond && expr_has_label_addr(s->u.for_s.cond))
            || (s->u.for_s.step && expr_has_label_addr(s->u.for_s.step))
            || stmt_has_computed_goto(s->u.for_s.body);
    case ST_LABEL: return stmt_has_computed_goto(s->u.label_s.stmt);
    case ST_SWITCH:
        for (int i = 0; i < s->u.switch_s.num_cases; i++)
            for (size_t j = 0; j < s->u.switch_s.cases[i].stmts.len; j++)
                if (stmt_has_computed_goto(&s->u.switch_s.cases[i].stmts.data[j])) return 1;
        return 0;
    case ST_BLOCK:
        for (size_t i = 0; i < s->u.block.len; i++)
            if (stmt_has_computed_goto(&s->u.block.data[i])) return 1;
        return 0;
    default: return 0;
    }
}

static int fd_has_computed_goto(const FunctionDecl *fd) {
    if (!fd) return 0;
    for (size_t i = 0; i < fd->body.len; i++)
        if (stmt_has_computed_goto(&fd->body.data[i])) return 1;
    return 0;
}

/* Is `name` pinned in the given function body? True iff array-typed
 * (its decl is TY_ARRAY, or param would be TY_PTR — the latter is fine) or
 * `&name` appears anywhere in the function, or the function uses computed gotos. */
static int is_pinned_in_body(const FunctionDecl *fd, const char *name, Type ty) {
    if (ty.kind == TY_ARRAY) return 1;
    if (ty.kind == TY_STRUCT) return 1;
    if (fd_has_computed_goto(fd)) return 1;
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
                        int64_t imm, int width, int is_unsigned, SourceLoc loc) {
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
    inst.force_stack = 0;
    memset(inst.call_arg_on_stack, 0, sizeof(inst.call_arg_on_stack));
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
/* Materialize a long double constant: its 80 bits do not fit in an immediate,
 * so the bytes go into an anonymous rodata global and the IR_CONST names it. */
static IRValue emit_ld_const(IRFunction *fn, long double val, SourceLoc loc) {
    char name[32];
    snprintf(name, sizeof name, "__fld.%d", g_flt_counter++);
    char *init = malloc(10);
    memcpy(init, &val, 10);
    ir_module_push_global(g_ir_module, name, 10, init, 1, 1, loc);
    IRValue v = new_value(fn);
    emit_inst_w(fn, IR_CONST, v, -1, -1, 0, 16, 0, loc);
    fn->insts.data[fn->insts.len - 1].is_float = 1;
    fn->insts.data[fn->insts.len - 1].call_name = xstrdup(name);
    set_value_float(fn, v, 1);
    return v;
}

static IRValue emit_float_const(IRFunction *fn, int width, int64_t bits, SourceLoc loc) {
    if (width == 16) {
        double d;
        memcpy(&d, &bits, sizeof d);
        return emit_ld_const(fn, (long double)d, loc);
    }
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
                      int64_t imm, SourceLoc loc) {
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

/* Load the SysV register eightbytes of an aggregate at `addr` into SSA
 * values.  `n`/`cls` come from sysv_classify_agg.  Each eightbyte becomes a
 * width-8 INTEGER or float SSA value (zero-extended from a shorter chunk). */
static void load_agg_regs(IRFunction *fn, IRValue addr, int size, int n,
                          const SysVRegClass cls[2], IRValue out[2],
                          SourceLoc loc) {
    for (int i = 0; i < n; i++) {
        int off = i * 8;
        int remain = size - off;
        if (remain > 8) remain = 8;
        if (remain <= 0) remain = 1;
        int remain_n = remain;
        int off_b = 0;
        IRValue v = -1;
        int is_sse = (cls[i] == SYSV_CLS_SSE);
        while (remain_n > 0) {
            int chunk = remain_n >= 8 ? 8 : remain_n >= 4 ? 4 : remain_n >= 2 ? 2 : 1;
            IRValue a = emit_add_const(fn, addr, off + off_b, loc);
            IRValue piece = new_value(fn);
            emit_inst_w(fn, IR_LOAD_PTR, piece, a, -1, 0, chunk, 1, loc);
            if (!is_sse && chunk < 8) {
                IRValue wide = new_value(fn);
                emit_inst_w(fn, IR_ZEXT, wide, piece, -1, chunk, 8, 1, loc);
                piece = wide;
            }
            if (off_b > 0 && !is_sse) {
                IRValue sh = new_value(fn);
                emit_inst_w(fn, IR_CONST, sh, -1, -1, (long long)off_b * 8, 8, 1, loc);
                IRValue shifted = new_value(fn);
                emit_inst_w(fn, IR_SHL, shifted, piece, sh, 0, 8, 1, loc);
                IRValue merged = new_value(fn);
                emit_inst_w(fn, IR_BOR, merged, v, shifted, 0, 8, 1, loc);
                v = merged;
            } else {
                v = piece;
            }
            off_b += chunk;
            remain_n -= chunk;
        }
        if (is_sse) {
            set_value_type(fn, v, remain <= 4 ? 4 : 8, 0);
            set_value_float(fn, v, 1);
        } else {
            set_value_type(fn, v, 8, 1);
        }
        out[i] = v;
    }
}

/* Store register eightbytes into an aggregate at `addr`. */
static void store_agg_regs(IRFunction *fn, IRValue addr, int size, int n,
                           const SysVRegClass cls[2], const IRValue vals[2],
                           SourceLoc loc) {
    (void)cls;
    for (int i = 0; i < n; i++) {
        int off = i * 8;
        int remain = size - off;
        if (remain <= 0) break;
        int remain_n = remain;
        int off_b = 0;
        IRValue srcv = vals[i];
        while (remain_n > 0) {
            int w = remain_n >= 8 ? 8 : remain_n >= 4 ? 4 : remain_n >= 2 ? 2 : 1;
            IRValue a = emit_add_const(fn, addr, off + off_b, loc);
            IRValue v = srcv;
            if (off_b > 0 && !get_value_is_float(fn, srcv)) {
                IRValue sh = new_value(fn);
                emit_inst_w(fn, IR_CONST, sh, -1, -1, (long long)off_b * 8, 8, 1, loc);
                IRValue shifted = new_value(fn);
                emit_inst_w(fn, IR_SHR, shifted, srcv, sh, 0, 8, 1, loc);
                v = shifted;
            }
            int vw = get_value_width(fn, v);
            if (vw != w && !get_value_is_float(fn, v)) {
                if (vw > w) {
                    IRValue t = new_value(fn);
                    emit_inst_w(fn, IR_TRUNC, t, v, -1, vw, w, 1, loc);
                    v = t;
                } else {
                    IRValue t = new_value(fn);
                    emit_inst_w(fn, IR_ZEXT, t, v, -1, vw, w, 1, loc);
                    v = t;
                }
            }
            emit_inst_w(fn, IR_STORE_PTR, -1, a, v, 0, w, 1, loc);
            off_b += w;
            remain_n -= w;
        }
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

static void ir_dbg_fill_struct(IRDebugVar *dv, Type ty) {
    dv->struct_tag = NULL;
    dv->struct_size = 0;
    dv->members = NULL;
    dv->num_members = 0;
    const char *tag = NULL;
    if (ty.kind == TY_STRUCT && ty.tag) tag = ty.tag;
    else if (ty.kind == TY_PTR && ty.pointee && ty.pointee->kind == TY_STRUCT
             && ty.pointee->tag)
        tag = ty.pointee->tag;
    if (!tag) return;
    dv->struct_tag = xstrdup(tag);
    const StructDef *sd = struct_registry_find_c(g_ir_structs, tag);
    if (!sd) return;
    dv->struct_size = sd->size > 0 ? sd->size : (ty.kind == TY_STRUCT ? type_size(ty) : 0);
    if (sd->num_members <= 0) return;
    dv->members = xmalloc((size_t)sd->num_members * sizeof(IRDebugMember));
    dv->num_members = sd->num_members;
    for (int i = 0; i < sd->num_members; i++) {
        const StructMember *sm = &sd->members[i];
        IRDebugMember *m = &dv->members[i];
        memset(m, 0, sizeof(*m));
        m->name = sm->name ? xstrdup(sm->name) : xstrdup("");
        m->offset = sm->offset;
        m->bit_width = sm->bit_width;
        m->bit_offset = sm->bit_offset;
        m->type_kind = (int)sm->type.kind;
        m->width = sm->type.kind == TY_PTR ? 8
                 : (sm->type.kind == TY_ARRAY ? type_size(sm->type)
                    : (sm->type.kind == TY_STRUCT ? type_size(sm->type)
                       : (sm->type.width ? sm->type.width : 4)));
        m->is_unsigned = sm->type.is_unsigned;
        m->is_bool = sm->type.is_bool;
    }
}

static void ir_add_dbg_var(IRFunction *fn, const char *name, SourceLoc loc,
                           IRDebugVarKind kind, Type ty, int alloca_ssa,
                           int param_idx) {
    if (fn->num_dbg_vars >= fn->cap_dbg_vars) {
        size_t nc = fn->cap_dbg_vars ? fn->cap_dbg_vars * 2 : 4;
        fn->dbg_vars = realloc(fn->dbg_vars, nc * sizeof(IRDebugVar));
        if (!fn->dbg_vars) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        fn->cap_dbg_vars = nc;
    }
    IRDebugVar *dv = &fn->dbg_vars[fn->num_dbg_vars++];
    memset(dv, 0, sizeof(*dv));
    dv->name = xstrdup(name);
    dv->loc = loc;
    dv->kind = kind;
    dv->type_kind = (int)ty.kind;
    dv->width = ty.kind == TY_PTR ? 8
              : (ty.kind == TY_ARRAY ? type_size(ty)
                 : (ty.width ? ty.width : 4));
    dv->is_unsigned = ty.is_unsigned;
    dv->is_bool = ty.is_bool;
    dv->array_len = ty.kind == TY_ARRAY ? ty.length : 0;
    dv->alloca_ssa = alloca_ssa;
    dv->param_idx = param_idx;
    ir_dbg_fill_struct(dv, ty);
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

static const IRSlot *irsymtable_find(const IRSymTable *st, const char *name) {
    for (size_t i = st->len; i > 0; i--)
        if (strcmp(st->data[i-1].name, name) == 0) return &st->data[i-1];
    return NULL;
}

/* Forward decl. */
static IRValue lower_expr(IRFunction *fn, IRSymTable *st, const Expr *e);
static IRValue lower_lvalue_addr(IRFunction *fn, IRSymTable *st, const Expr *e);
static void lower_stmt(IRFunction *fn, IRSymTable *st, const Stmt *s,
                       const FunctionDecl *cur_fd);

/* Initializer-list lowering (defined after ir_generate). */
static void pack_init(const IRModule *ir, const Type *ty, const Expr *e,
                      char *bytes, int sz, const char *ctx, SourceLoc loc,
                      IRGlobal *g);
/* Push the rodata globals queued while packing an initializer. */
static void flush_rodata(IRModule *m);
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
        set_value_type(fn, res, dst_w, 0);
        return res;
    }
    if (src_float && !to_float) {
        /* float → int. */
        op = IR_FPTOSI;
        emit_inst_w(fn, op, res, v, -1, src_w, dst_w, dst_u, loc);
        set_value_type(fn, res, dst_w, dst_u);
        set_value_float(fn, res, 0);
        return res;
    }
    /* int → float.  The destination is a float, so is_unsigned is free to
     * carry the SOURCE signedness — codegen needs it because the x86
     * conversions (cvtsi2sd, fild) are signed. */
    op = IR_SITOFP;
    emit_inst_w(fn, op, res, v, -1, src_w, dst_w, get_value_is_unsigned(fn, v),
                loc);
    set_value_type(fn, res, dst_w, 0);
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
        if (sd)
            struct_lookup_member(g_ir_structs, sd, e->u.member.name, &off);
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

static IRValue lower_expr(IRFunction *fn, IRSymTable *st, const Expr *e);

static IRValue lower_complex_binop(IRFunction *fn, IRSymTable *st, const Expr *e,
                                  Type lt, Type rt, BinOp bop) {
    int lt_is_cplx = (lt.kind == TY_STRUCT && lt.tag && strncmp(lt.tag, "__complex_", 10) == 0);
    int rt_is_cplx = (rt.kind == TY_STRUCT && rt.tag && strncmp(rt.tag, "__complex_", 10) == 0);
    
    int l_elem_sz = lt_is_cplx ? type_size(lt) / 2 : (lt.kind == TY_PTR ? 8 : (lt.width ? lt.width : 4));
    int r_elem_sz = rt_is_cplx ? type_size(rt) / 2 : (rt.kind == TY_PTR ? 8 : (rt.width ? rt.width : 4));
    int l_is_float = lt_is_cplx ? ((strstr(lt.tag, "float") || strstr(lt.tag, "double") || strstr(lt.tag, "ldouble")) ? 1 : 0) : (lt.kind == TY_FLOAT);
    int r_is_float = rt_is_cplx ? ((strstr(rt.tag, "float") || strstr(rt.tag, "double") || strstr(rt.tag, "ldouble")) ? 1 : 0) : (rt.kind == TY_FLOAT);
    
    int is_float = (l_is_float || r_is_float);
    int elem_sz = l_elem_sz > r_elem_sz ? l_elem_sz : r_elem_sz;
    if (is_float && elem_sz < 4) elem_sz = 4;
    
    int l_is_unsigned = lt_is_cplx ? (strstr(lt.tag, "unsigned") != NULL) : lt.is_unsigned;
    int r_is_unsigned = rt_is_cplx ? (strstr(rt.tag, "unsigned") != NULL) : rt.is_unsigned;
    
    IRValue lv_addr = -1, rv_addr = -1;
    IRValue l_real, l_imag, r_real, r_imag;
    
    /* Load LHS components */
    if (lt_is_cplx) {
        lv_addr = lower_expr(fn, st, e->u.bin.l);
        l_real = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, l_real, lv_addr, -1, 0, l_elem_sz, l_is_unsigned, e->loc);
        if (l_is_float) set_value_float(fn, l_real, 1);
        
        IRValue off = new_value(fn);
        emit_inst_w(fn, IR_CONST, off, -1, -1, l_elem_sz, 8, 1, e->loc);
        IRValue iaddr = emit_bin_w(fn, IR_ADD, lv_addr, off, 8, 1, e->loc);
        l_imag = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, l_imag, iaddr, -1, 0, l_elem_sz, l_is_unsigned, e->loc);
        if (l_is_float) set_value_float(fn, l_imag, 1);
    } else {
        l_real = lower_expr(fn, st, e->u.bin.l);
        if (l_is_float) {
            l_imag = emit_float_const(fn, l_elem_sz, 0, e->loc);
        } else {
            l_imag = new_value(fn);
            emit_inst_w(fn, IR_CONST, l_imag, -1, -1, 0, l_elem_sz, l_is_unsigned, e->loc);
        }
    }
    
    /* Convert LHS to common (elem_sz, is_float) */
    if (l_is_float != is_float || l_elem_sz != elem_sz) {
        if (l_is_float && !is_float) {
            l_real = convert_numeric(fn, l_real, l_elem_sz, elem_sz, 0, 0, e->loc);
            l_imag = convert_numeric(fn, l_imag, l_elem_sz, elem_sz, 0, 0, e->loc);
        } else if (!l_is_float && is_float) {
            l_real = convert_numeric(fn, l_real, l_elem_sz, elem_sz, 0, 1, e->loc);
            l_imag = convert_numeric(fn, l_imag, l_elem_sz, elem_sz, 0, 1, e->loc);
        } else if (is_float) {
            l_real = convert_numeric(fn, l_real, l_elem_sz, elem_sz, 0, 1, e->loc);
            l_imag = convert_numeric(fn, l_imag, l_elem_sz, elem_sz, 0, 1, e->loc);
        } else {
            l_real = coerce(fn, l_real, l_elem_sz, l_is_unsigned, elem_sz, l_is_unsigned, e->loc);
            l_imag = coerce(fn, l_imag, l_elem_sz, l_is_unsigned, elem_sz, l_is_unsigned, e->loc);
        }
    }
    
    /* Load RHS components */
    if (rt_is_cplx) {
        rv_addr = lower_expr(fn, st, e->u.bin.r);
        r_real = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, r_real, rv_addr, -1, 0, r_elem_sz, r_is_unsigned, e->loc);
        if (r_is_float) set_value_float(fn, r_real, 1);
        
        IRValue off = new_value(fn);
        emit_inst_w(fn, IR_CONST, off, -1, -1, r_elem_sz, 8, 1, e->loc);
        IRValue iaddr = emit_bin_w(fn, IR_ADD, rv_addr, off, 8, 1, e->loc);
        r_imag = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, r_imag, iaddr, -1, 0, r_elem_sz, r_is_unsigned, e->loc);
        if (r_is_float) set_value_float(fn, r_imag, 1);
    } else {
        r_real = lower_expr(fn, st, e->u.bin.r);
        if (r_is_float) {
            r_imag = emit_float_const(fn, r_elem_sz, 0, e->loc);
        } else {
            r_imag = new_value(fn);
            emit_inst_w(fn, IR_CONST, r_imag, -1, -1, 0, r_elem_sz, r_is_unsigned, e->loc);
        }
    }
    
    /* Convert RHS to common (elem_sz, is_float) */
    if (r_is_float != is_float || r_elem_sz != elem_sz) {
        if (r_is_float && !is_float) {
            r_real = convert_numeric(fn, r_real, r_elem_sz, elem_sz, 0, 0, e->loc);
            r_imag = convert_numeric(fn, r_imag, r_elem_sz, elem_sz, 0, 0, e->loc);
        } else if (!r_is_float && is_float) {
            r_real = convert_numeric(fn, r_real, r_elem_sz, elem_sz, 0, 1, e->loc);
            r_imag = convert_numeric(fn, r_imag, r_elem_sz, elem_sz, 0, 1, e->loc);
        } else if (is_float) {
            r_real = convert_numeric(fn, r_real, r_elem_sz, elem_sz, 0, 1, e->loc);
            r_imag = convert_numeric(fn, r_imag, r_elem_sz, elem_sz, 0, 1, e->loc);
        } else {
            r_real = coerce(fn, r_real, r_elem_sz, r_is_unsigned, elem_sz, r_is_unsigned, e->loc);
            r_imag = coerce(fn, r_imag, r_elem_sz, r_is_unsigned, elem_sz, r_is_unsigned, e->loc);
        }
    }
    
    if (bop == BOP_EQ || bop == BOP_NE) {
        if (is_float) {
            int cmp_sub = (bop == BOP_EQ) ? 4 : 5;
            IRValue cmp_r = emit_bin_w(fn, IR_FCMP, l_real, r_real, elem_sz, cmp_sub, e->loc);
            IRValue cmp_i = emit_bin_w(fn, IR_FCMP, l_imag, r_imag, elem_sz, cmp_sub, e->loc);
            if (bop == BOP_EQ) return emit_bin_w(fn, IR_BAND, cmp_r, cmp_i, 4, 0, e->loc);
            else return emit_bin_w(fn, IR_BOR, cmp_r, cmp_i, 4, 0, e->loc);
        } else {
            IROpcode cmp_op = (bop == BOP_EQ ? IR_EQ : IR_NE);
            IRValue cmp_r = emit_bin_w(fn, cmp_op, l_real, r_real, elem_sz, 1, e->loc);
            IRValue cmp_i = emit_bin_w(fn, cmp_op, l_imag, r_imag, elem_sz, 1, e->loc);
            if (bop == BOP_EQ) return emit_bin_w(fn, IR_BAND, cmp_r, cmp_i, 4, 0, e->loc);
            else return emit_bin_w(fn, IR_BOR, cmp_r, cmp_i, 4, 0, e->loc);
        }
    }
    
    /* Arithmetic operations */
    IRValue out_r, out_i;
    IROpcode add_op = is_float ? IR_FADD : IR_ADD;
    IROpcode sub_op = is_float ? IR_FSUB : IR_SUB;
    IROpcode mul_op = is_float ? IR_FMUL : IR_MUL;
    IROpcode div_op = is_float ? IR_FDIV : IR_DIV;
    
    if (bop == BOP_ADD) {
        out_r = emit_bin_w(fn, add_op, l_real, r_real, elem_sz, 1, e->loc);
        out_i = emit_bin_w(fn, add_op, l_imag, r_imag, elem_sz, 1, e->loc);
    } else if (bop == BOP_SUB) {
        out_r = emit_bin_w(fn, sub_op, l_real, r_real, elem_sz, 1, e->loc);
        out_i = emit_bin_w(fn, sub_op, l_imag, r_imag, elem_sz, 1, e->loc);
    } else if (bop == BOP_MUL) {
        IRValue r1 = emit_bin_w(fn, mul_op, l_real, r_real, elem_sz, 1, e->loc);
        IRValue r2 = emit_bin_w(fn, mul_op, l_imag, r_imag, elem_sz, 1, e->loc);
        out_r = emit_bin_w(fn, sub_op, r1, r2, elem_sz, 1, e->loc);
        
        IRValue i1 = emit_bin_w(fn, mul_op, l_real, r_imag, elem_sz, 1, e->loc);
        IRValue i2 = emit_bin_w(fn, mul_op, l_imag, r_real, elem_sz, 1, e->loc);
        out_i = emit_bin_w(fn, add_op, i1, i2, elem_sz, 1, e->loc);
    } else if (bop == BOP_DIV) {
        IRValue d1 = emit_bin_w(fn, mul_op, r_real, r_real, elem_sz, 1, e->loc);
        IRValue d2 = emit_bin_w(fn, mul_op, r_imag, r_imag, elem_sz, 1, e->loc);
        IRValue denom = emit_bin_w(fn, add_op, d1, d2, elem_sz, 1, e->loc);
        
        IRValue n_r1 = emit_bin_w(fn, mul_op, l_real, r_real, elem_sz, 1, e->loc);
        IRValue n_r2 = emit_bin_w(fn, mul_op, l_imag, r_imag, elem_sz, 1, e->loc);
        IRValue num_r = emit_bin_w(fn, add_op, n_r1, n_r2, elem_sz, 1, e->loc);
        out_r = emit_bin_w(fn, div_op, num_r, denom, elem_sz, 1, e->loc);
        
        IRValue n_i1 = emit_bin_w(fn, mul_op, l_imag, r_real, elem_sz, 1, e->loc);
        IRValue n_i2 = emit_bin_w(fn, mul_op, l_real, r_imag, elem_sz, 1, e->loc);
        IRValue num_i = emit_bin_w(fn, sub_op, n_i1, n_i2, elem_sz, 1, e->loc);
        out_i = emit_bin_w(fn, div_op, num_i, denom, elem_sz, 1, e->loc);
    } else {
        out_r = l_real; out_i = l_imag;
    }
    if (is_float) {
        set_value_float(fn, out_r, 1);
        set_value_float(fn, out_i, 1);
    }
    
    /* Allocate stack slot for result struct */
    int total_sz = elem_sz * 2;
    IRValue slot = emit_alloca(fn, total_sz, 8, 1, e->loc);
    IRValue addr = emit_bin_w(fn, IR_ADDR, slot, -1, 8, 1, e->loc);
    emit_inst_w(fn, IR_STORE_PTR, -1, addr, out_r, 0, elem_sz, 1, e->loc);
    IRValue off = new_value(fn);
    emit_inst_w(fn, IR_CONST, off, -1, -1, elem_sz, 8, 1, e->loc);
    IRValue iaddr = emit_bin_w(fn, IR_ADD, addr, off, 8, 1, e->loc);
    emit_inst_w(fn, IR_STORE_PTR, -1, iaddr, out_i, 0, elem_sz, 1, e->loc);
    return addr;
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
        if (w == 16)
            return emit_ld_const(fn, strtold(e->u.float_text, NULL), e->loc);
        int64_t bits = 0;
        if (w == 4) { float f = (float)strtod(e->u.float_text, NULL); memcpy(&bits, &f, sizeof(f)); }
        else { double d = strtod(e->u.float_text, NULL); memcpy(&bits, &d, sizeof(d)); }
        return emit_float_const(fn, w, bits, e->loc);
    }
    case EX_STR: {
        /* Register a rodata global holding the bytes, then produce its
         * address via IR_GADDR. */
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
                /* Float negation: -x = (-0.0) - x.  Subtracting from +0.0
                 * would turn -0.0 into +0.0, losing the sign IEEE keeps. */
                int64_t negzero = (rw == 4) ? (int64_t)0x80000000
                                            : (int64_t)0x8000000000000000LL;
                IRValue zero = emit_float_const(fn, rw, negzero, e->loc);
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
            if (e->u.un.operand->type.kind == TY_STRUCT && e->u.un.operand->type.tag &&
                strncmp(e->u.un.operand->type.tag, "__complex_", 10) == 0) {
                Type cty = e->u.un.operand->type;
                int total_sz = type_size(cty);
                int elem_sz = total_sz / 2;
                int is_float = (strstr(cty.tag, "float") != NULL || strstr(cty.tag, "double") != NULL || strstr(cty.tag, "ldouble") != NULL);
                int is_unsigned = (strstr(cty.tag, "unsigned") != NULL);
                IRValue op_addr = lower_expr(fn, st, e->u.un.operand);
                IRValue vr = new_value(fn);
                emit_inst_w(fn, IR_LOAD_PTR, vr, op_addr, -1, 0, elem_sz, is_unsigned, e->loc);
                if (is_float) set_value_float(fn, vr, 1);
                
                IRValue off = new_value(fn);
                emit_inst_w(fn, IR_CONST, off, -1, -1, elem_sz, 8, 1, e->loc);
                IRValue iaddr = emit_bin_w(fn, IR_ADD, op_addr, off, 8, 1, e->loc);
                IRValue vi = new_value(fn);
                emit_inst_w(fn, IR_LOAD_PTR, vi, iaddr, -1, 0, elem_sz, is_unsigned, e->loc);
                if (is_float) set_value_float(fn, vi, 1);
                
                IRValue neg_vi;
                if (is_float) {
                    int64_t negzero = (elem_sz == 4) ? (int64_t)0x80000000 : (int64_t)0x8000000000000000LL;
                    IRValue zero = emit_float_const(fn, elem_sz, negzero, e->loc);
                    neg_vi = emit_bin_w(fn, IR_FSUB, zero, vi, elem_sz, 0, e->loc);
                    set_value_float(fn, neg_vi, 1);
                } else {
                    neg_vi = emit_bin_w(fn, IR_NEG, vi, -1, elem_sz, 0, e->loc);
                }
                
                IRValue slot = emit_alloca(fn, total_sz, 8, 1, e->loc);
                IRValue addr = emit_bin_w(fn, IR_ADDR, slot, -1, 8, 1, e->loc);
                emit_inst_w(fn, IR_STORE_PTR, -1, addr, vr, 0, elem_sz, 1, e->loc);
                IRValue out_iaddr = emit_bin_w(fn, IR_ADD, addr, off, 8, 1, e->loc);
                emit_inst_w(fn, IR_STORE_PTR, -1, out_iaddr, neg_vi, 0, elem_sz, 1, e->loc);
                return addr;
            }
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
        if ((lt.kind == TY_STRUCT && lt.tag && strncmp(lt.tag, "__complex_", 10) == 0) ||
            (rt.kind == TY_STRUCT && rt.tag && strncmp(rt.tag, "__complex_", 10) == 0)) {
            return lower_complex_binop(fn, st, e, lt, rt, bop);
        }
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
        } else if (e->type.bitfield_width > 0 && e->type.bitfield_width < op_w * 8) {
            int64_t msk = bitfield_mask64(e->type.bitfield_width);
            IRValue mv = new_value(fn);
            emit_inst_w(fn, IR_CONST, mv, -1, -1, msk, op_w, 1, e->loc);
            IRValue masked = new_value(fn);
            emit_inst_w(fn, IR_BAND, masked, result, mv, 0, op_w, op_u, e->loc);
            result = masked;
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
        if (strcmp(e->u.var.name, "NULL") == 0) {
            IRValue v = new_value(fn);
            emit_inst_w(fn, IR_CONST, v, -1, -1, 0, 8, 1, e->loc);
            return v;
        }
        const IRSlot *entry = irsymtable_find(st, e->u.var.name);
        if (!entry) {
            /* Not a variable — is it a function name?  A function lvalue
             * (e.g. `add` used as a value, or `&add`) decays to a function
             * pointer; emit an FADDR so the function's address is loaded
             * (patched against the code symbol table, not .data). */
            if (e->u.var.pkg) {
                /* Qualified import (runtime.printf): ELF name is unmangled. */
                IRValue v = new_value(fn);
                emit_inst_w(fn, IR_FADDR, v, -1, -1, 0, 8, 1, e->loc);
                fn->insts.data[fn->insts.len - 1].call_name = xstrdup(e->u.var.name);
                return v;
            }
            if (g_ir_tu) {
                for (size_t i = 0; i < g_ir_tu->functions.len; i++) {
                    if (strcmp(g_ir_tu->functions.data[i].name, e->u.var.name) == 0) {
                        IRValue v = new_value(fn);
                        emit_inst_w(fn, IR_FADDR, v, -1, -1, 0, 8, 1, e->loc);
                        fn->insts.data[fn->insts.len - 1].call_name =
                            xstrdup(e->u.var.name);
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
        if (entry->ty.kind == TY_ARRAY || entry->ty.kind == TY_STRUCT)
            return emit_bin_w(fn, IR_ADDR, entry->slot, -1, 8, 1, e->loc);
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
            if (strncmp(lv->type.tag, "__complex_", 10) == 0 &&
                (e->u.assign.rvalue->type.kind != TY_STRUCT ||
                 !e->u.assign.rvalue->type.tag ||
                 strcmp(lv->type.tag, e->u.assign.rvalue->type.tag) != 0)) {
                /* Scalar to complex assignment or different precision complex */
                Expr fake_cast;
                memset(&fake_cast, 0, sizeof(fake_cast));
                fake_cast.kind = EX_CAST;
                fake_cast.type = lv->type;
                fake_cast.loc = e->loc;
                fake_cast.u.cast.target = lv->type;
                fake_cast.u.cast.operand = e->u.assign.rvalue;
                IRValue cplx_rv = lower_expr(fn, st, &fake_cast);
                emit_struct_copy(fn, dst, cplx_rv, type_size(lv->type), e->loc);
                return dst;
            }
            emit_struct_copy(fn, dst, rv, type_size(lv->type), e->loc);
            return dst;
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
                int64_t mask = bitfield_mask64(bit_width);
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
        if (e->u.call.callee->kind == EX_VAR &&
            (strcmp(e->u.call.callee->u.var.name, "__builtin_conjf") == 0 ||
             strcmp(e->u.call.callee->u.var.name, "__builtin_conj") == 0 ||
             strcmp(e->u.call.callee->u.var.name, "__builtin_conjl") == 0)) {
            Expr fake_un;
            memset(&fake_un, 0, sizeof(fake_un));
            fake_un.kind = EX_UNARY;
            fake_un.type = e->type;
            fake_un.loc = e->loc;
            fake_un.u.un.op = UOP_BITNOT;
            fake_un.u.un.operand = e->u.call.args.data[0];
            return lower_expr(fn, st, &fake_un);
        }
        if (e->u.call.callee->kind == EX_VAR && strcmp(e->u.call.callee->u.var.name, "__builtin_prefetch") == 0) {
            for (size_t i = 0; i < e->u.call.args.len; i++)
                lower_expr(fn, st, e->u.call.args.data[i]);
            return -1;
        }
        if (e->u.call.callee->kind == EX_VAR && strcmp(e->u.call.callee->u.var.name, "__builtin_frame_address") == 0) {
            long long level = 0;
            if (e->u.call.args.len > 0) fold_const_int(e->u.call.args.data[0], &level);
            IRValue v = new_value(fn);
            emit_inst_w(fn, IR_FRAME_ADDR, v, -1, -1, (int64_t)level, 8, 1, e->loc);
            set_value_type(fn, v, 8, 1);
            return v;
        }
        if (e->u.call.callee->kind == EX_VAR && strcmp(e->u.call.callee->u.var.name, "__builtin_return_address") == 0) {
            long long level = 0;
            if (e->u.call.args.len > 0) fold_const_int(e->u.call.args.data[0], &level);
            IRValue v = new_value(fn);
            emit_inst_w(fn, IR_RETURN_ADDR, v, -1, -1, (int64_t)level, 8, 1, e->loc);
            set_value_type(fn, v, 8, 1);
            return v;
        }
        if (e->u.call.callee->kind == EX_VAR && (strcmp(e->u.call.callee->u.var.name, "__builtin_alloca") == 0 ||
                                                strcmp(e->u.call.callee->u.var.name, "alloca") == 0)) {
            IRValue sz = lower_expr(fn, st, e->u.call.args.data[0]);
            IRValue v = new_value(fn);
            emit_inst_w(fn, IR_DYN_ALLOCA, v, sz, -1, 0, 8, 1, e->loc);
            set_value_type(fn, v, 8, 1);
            fn->has_dyn_alloca = 1;
            return v;
        }
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
                memset(&inst, 0, sizeof(inst));
                inst.op = IR_CALL;
                inst.a = -1; inst.b = -1; inst.imm = 0;
                inst.loc = e->loc;
                inst.call_name = xstrdup(cname);
                inst.call_callee = -1;
                inst.call_nargs = (last >= 0) ? 2 : 1;
                inst.call_args[0] = ap;
                inst.call_args[1] = last;
                if (strcmp(cname, "va_arg") == 0) {
                    if (e->va_arg_type.kind == TY_STRUCT) {
                        int sz = type_size(e->va_arg_type);
                        if (sz <= 0) sz = 8;
                        SysVRegClass cls[2];
                        int nreg = sysv_classify_agg(e->va_arg_type, cls);
                        IRValue slot = emit_alloca(fn, sz < 8 ? 8 : sz, 8, 1, e->loc);
                        IRValue addr = emit_bin_w(fn, IR_ADDR, slot, -1, 8, 1, e->loc);
                        inst.dst = new_value(fn);
                        inst.width = 8;
                        inst.is_unsigned = 1;
                        inst.is_float = 0;
                        inst.imm = sz;
                        inst.force_stack = (nreg == 0);
                        inst.call_nargs = 2;
                        inst.call_args[1] = addr;
                        ir_inst_array_push(&fn->insts, inst);
                        set_value_type(fn, inst.dst, 8, 1);
                        return inst.dst;
                    }
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
        unsigned char arg_on_stack[IR_CALL_MAX_ARGS];
        int nargs = 0;
        memset(arg_on_stack, 0, sizeof(arg_on_stack));
        /* Reserve a slot if the return needs a hidden sret pointer. */
        int is_void_pre = (e->type.width == 0);
        int is_ret_struct_pre = (!is_void_pre && e->type.kind == TY_STRUCT);
        SysVRegClass ret_cls_pre[2];
        int ret_nreg_pre = is_ret_struct_pre
                           ? sysv_classify_agg(e->type, ret_cls_pre) : 0;
        int ret_in_mem_pre = is_ret_struct_pre && ret_nreg_pre == 0;
        int arg_limit = IR_CALL_MAX_ARGS - (ret_in_mem_pre ? 1 : 0);
        /* Expand aggregates into per-eightbyte SSA args.  Register-class
         * eightbytes travel in GP/XMM; MEMORY-class eightbytes are forced
         * onto the stack (SysV). */
        for (int i = 0; i < (int)e->u.call.args.len; i++) {
            Expr *arg = e->u.call.args.data[i];
            IRValue av = lower_expr(fn, st, arg);
            if (arg->type.kind == TY_STRUCT) {
                SysVRegClass cls[2];
                int nreg = sysv_classify_agg(arg->type, cls);
                int asz = type_size(arg->type);
                if (nreg > 0) {
                    IRValue ebs[2];
                    load_agg_regs(fn, av, asz, nreg, cls, ebs, e->loc);
                    for (int k = 0; k < nreg; k++) {
                        if (nargs >= arg_limit) break;
                        arg_vals[nargs] = ebs[k];
                        arg_on_stack[nargs] = 0;
                        nargs++;
                    }
                    continue;
                }
                /* MEMORY: copy eightbytes onto the outgoing stack. */
                int nmem = (asz + 7) / 8;
                if (nmem < 1) nmem = 1;
                for (int k = 0; k < nmem; k++) {
                    if (nargs >= arg_limit) break;
                    int off = k * 8;
                    int remain = asz - off;
                    int w = remain >= 8 ? 8 : remain >= 4 ? 4 : remain >= 2 ? 2 : 1;
                    IRValue a = emit_add_const(fn, av, off, e->loc);
                    IRValue v = new_value(fn);
                    emit_inst_w(fn, IR_LOAD_PTR, v, a, -1, 0, w, 1, e->loc);
                    if (w < 8) {
                        IRValue wide = new_value(fn);
                        emit_inst_w(fn, IR_ZEXT, wide, v, -1, w, 8, 1, e->loc);
                        v = wide;
                    }
                    arg_vals[nargs] = v;
                    arg_on_stack[nargs] = 1;
                    nargs++;
                }
                continue;
            }
            if (nargs < arg_limit) {
                arg_vals[nargs] = av;
                arg_on_stack[nargs] = 0;
                nargs++;
            }
        }
        /* Void call: no result value (width 0).  Still emit the call for its
         * side effects; dst=-1 marks "no result". */
        int is_void = is_void_pre;
        /* Struct return: MEMORY class uses hidden sret; register class
         * returns bits in RAX(/RDX) or XMM0(/XMM1). */
        int is_ret_struct = is_ret_struct_pre;
        SysVRegClass ret_cls[2];
        ret_cls[0] = ret_cls_pre[0];
        ret_cls[1] = ret_cls_pre[1];
        int ret_nreg = ret_nreg_pre;
        int ret_in_mem = ret_in_mem_pre;
        IRValue sret_addr = -1;
        if (ret_in_mem) {
            int total = type_size(e->type);
            IRValue slot = emit_alloca(fn, total, 8, 1, e->loc);
            sret_addr = emit_bin_w(fn, IR_ADDR, slot, -1, 8, 1, e->loc);
        }
        IRValue slot_addr = -1;
        if (is_ret_struct && ret_nreg > 0) {
            int total = type_size(e->type);
            IRValue slot = emit_alloca(fn, total, 8, 1, e->loc);
            slot_addr = emit_bin_w(fn, IR_ADDR, slot, -1, 8, 1, e->loc);
        }
        IRValue v = ret_in_mem ? sret_addr
                  : (is_ret_struct && ret_nreg > 0) ? slot_addr
                  : (is_void ? -1 : new_value(fn));
        IRValue ret_lo = -1, ret_hi = -1;
        if (is_ret_struct && ret_nreg > 0) {
            ret_lo = new_value(fn);
            if (ret_nreg > 1) ret_hi = new_value(fn);
        }
        int total_nargs = nargs + (ret_in_mem ? 1 : 0);
        IRInst inst;
        memset(&inst, 0, sizeof(inst));
        inst.op = IR_CALL;
        inst.dst = ret_in_mem ? -1
                 : (is_ret_struct && ret_nreg > 0) ? ret_lo
                 : v;
        inst.a = -1;
        inst.b = ret_hi;   /* second return eightbyte, or -1 */
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
            } else if (strncmp(cname, "__builtin_", 10) == 0) {
                if (strstr(cname, "__builtin_clz") || strstr(cname, "__builtin_ctz")
                    || strstr(cname, "__builtin_ffs") || strstr(cname, "__builtin_popcount")
                    || strstr(cname, "__builtin_parity") || strstr(cname, "__builtin_clrsb")
                    || strstr(cname, "__builtin_bswap"))
                    inst.call_name = xstrdup(cname);
                else if (strcmp(cname, "__builtin_abort") == 0) inst.call_name = xstrdup("abort");
                else if (strcmp(cname, "__builtin_exit") == 0) inst.call_name = xstrdup("exit");
                else if (strcmp(cname, "__builtin_trap") == 0) inst.call_name = xstrdup("abort");
                else if (strcmp(cname, "__builtin_memset") == 0) inst.call_name = xstrdup("memset");
                else if (strcmp(cname, "__builtin_memcpy") == 0) inst.call_name = xstrdup("memcpy");
                else if (strcmp(cname, "__builtin_memcmp") == 0) inst.call_name = xstrdup("memcmp");
                else if (strcmp(cname, "__builtin_strcmp") == 0) inst.call_name = xstrdup("strcmp");
                else if (strcmp(cname, "__builtin_strncmp") == 0) inst.call_name = xstrdup("strncmp");
                else if (strcmp(cname, "__builtin_strlen") == 0) inst.call_name = xstrdup("strlen");
                else if (strcmp(cname, "__builtin_strcpy") == 0) inst.call_name = xstrdup("strcpy");
                else if (strcmp(cname, "__builtin_strcat") == 0) inst.call_name = xstrdup("strcat");
                else if (strcmp(cname, "__builtin_fabs") == 0) inst.call_name = xstrdup("fabs");
                else if (strcmp(cname, "__builtin_fabsf") == 0) inst.call_name = xstrdup("fabsf");
                else if (strcmp(cname, "__builtin_fabsl") == 0) inst.call_name = xstrdup("fabsl");
                else inst.call_name = xstrdup(cname + 10);
            } else {
                int is_direct = 0;
                /* If the callee's type is a bare function type (TY_FUNC),
                 * it's a direct call — even if the function is only declared
                 * `extern` in this TU (not defined, so absent from g_ir_tu).
                 * Without this, extern function calls are wrongly lowered to
                 * indirect calls through an uninitialized function-pointer
                 * variable, which crashes at runtime. */
                if (e->u.call.callee->type.kind == TY_FUNC) {
                    is_direct = 1;
                } else if (e->u.call.callee->u.var.pkg) {
                    /* Package-qualified name (runtime.printf) — always a direct
                     * named call; the defining TU is linked separately. */
                    is_direct = 1;
                } else if (g_ir_tu) {
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
        if (ret_in_mem) {
            inst.call_args[0] = sret_addr;
            inst.call_arg_on_stack[0] = 0;
            for (int i = 0; i < nargs; i++) {
                inst.call_args[i + 1] = arg_vals[i];
                inst.call_arg_on_stack[i + 1] = arg_on_stack[i];
            }
        } else {
            for (int i = 0; i < nargs; i++) {
                inst.call_args[i] = arg_vals[i];
                inst.call_arg_on_stack[i] = arg_on_stack[i];
            }
        }
        /* Register-returned structs: result eightbytes land in dst(/b); the
         * expression value is still the slot address.  MEMORY structs: value
         * is the sret pointer (width 8). */
        if (is_ret_struct && ret_nreg > 0) {
            /* Result eightbytes are INTEGER (RAX/RDX) or SSE (XMM0/XMM1).
             * Mark float class on the SSA results so codegen picks XMM. */
            inst.width = 8;
            inst.is_float = (ret_cls[0] == SYSV_CLS_SSE);
            inst.is_unsigned = 1;
            ir_inst_array_push(&fn->insts, inst);
            set_value_type(fn, ret_lo, 8, 1);
            if (ret_cls[0] == SYSV_CLS_SSE) set_value_float(fn, ret_lo, 1);
            if (ret_hi >= 0) {
                set_value_type(fn, ret_hi, 8, 1);
                if (ret_cls[1] == SYSV_CLS_SSE) set_value_float(fn, ret_hi, 1);
            }
            IRValue ebs[2] = { ret_lo, ret_hi };
            store_agg_regs(fn, slot_addr, type_size(e->type), ret_nreg,
                           ret_cls, ebs, e->loc);
            return slot_addr;
        }
        inst.width = ret_in_mem ? 8
                      : (is_void ? 0 : (e->type.width ? e->type.width : 4));
        inst.is_unsigned = ret_in_mem ? 1 : e->type.is_unsigned;
        ir_inst_array_push(&fn->insts, inst);
        if (!is_void && !is_ret_struct) {
            set_value_type(fn, v, inst.width, inst.is_unsigned);
            if (e->type.kind == TY_FLOAT)
                set_value_float(fn, v, 1);
        } else if (ret_in_mem) {
            set_value_type(fn, v, 8, 1);
        }
        return v;
    }
    case EX_ADDR: {
        Expr *op = e->u.addr.operand;
        if (op->kind == EX_VAR) {
            const IRSlot *entry = irsymtable_find(st, op->u.var.name);
            if (!entry) {
                /* Not a variable — must be a function name (`&func`). */
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
        if (op->kind == EX_INDEX || op->kind == EX_MEMBER
            || op->kind == EX_COMPOUND_LITERAL) {
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
        /* Struct/array members: return address (decay). Scalar/pointer: load.
         * Also handle array-to-pointer decay performed in-place by sema: the
         * node's type may be TY_PTR but the underlying struct field is TY_ARRAY
         * — in that case the member's address IS the pointer value. */
        if (e->type.kind == TY_STRUCT) return addr;
        if (e->type.kind == TY_ARRAY)  return addr;
        if (e->type.kind == TY_PTR && member_field_is_array(e)) return addr;
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
                int is_signed_bf = (!e->type.is_unsigned && !e->type.is_bool);
                if (e->type.enum_id > 0) {
                    is_signed_bf = 0;
                    if (g_ir_tu && (size_t)(e->type.enum_id - 1) < g_ir_tu->enums.len) {
                        const EnumDef *ed = &g_ir_tu->enums.data[e->type.enum_id - 1];
                        for (int k = 0; k < ed->num_constants; k++) {
                            if (ed->constants[k].value < 0) {
                                is_signed_bf = 1;
                                break;
                            }
                        }
                    }
                }
                if (is_signed_bf) {
                    /* A signed bitfield holds a two's-complement value in
                     * bit_width bits: shift it up to the unit's sign bit and
                     * back down arithmetically.  Masking alone would read
                     * `int a : 3` holding -3 back as 5. */
                    int shift = w * 8 - bit_width;
                    IRValue s = new_value(fn);
                    emit_inst_w(fn, IR_CONST, s, -1, -1, shift, 8, 1, e->loc);
                    /* The left shift is tagged signed so its result is
                     * sign-extended in the register — the arithmetic shift
                     * below reads the full 64-bit value. */
                    IRValue up = new_value(fn);
                    emit_inst_w(fn, IR_SHL, up, v, s, 0, w, 0, e->loc);
                    IRValue down = new_value(fn);
                    emit_inst_w(fn, IR_SHR, down, up, s, 0, w, 0, e->loc);
                    v = down;
                    u = 0;
                } else {
                    int64_t mask = bitfield_mask64(bit_width);
                    IRValue m = new_value(fn);
                    emit_inst_w(fn, IR_CONST, m, -1, -1, mask, w, 1, e->loc);
                    IRValue masked = new_value(fn);
                    emit_inst_w(fn, IR_BAND, masked, v, m, 0, w, 1, e->loc);
                    v = masked;
                    u = 1;
                }
            }
            /* Result is a small int; coerce to the member's declared width. */
            return coerce(fn, v, w, u, e->type.width ? e->type.width : 4,
                          u, e->loc);
        }
        if (e->type.kind == TY_FLOAT) set_value_float(fn, v, 1);
        return v;
    }
    case EX_CAST: {
        if (e->type.kind == TY_STRUCT && e->type.tag && strncmp(e->type.tag, "__complex_", 10) == 0) {
            Type cty = e->type;
            int total_sz = type_size(cty);
            int elem_sz = total_sz / 2;
            int is_float = (strstr(cty.tag, "float") != NULL || strstr(cty.tag, "double") != NULL || strstr(cty.tag, "ldouble") != NULL);
            
            int is_unsigned = (strstr(cty.tag, "unsigned") != NULL);
            IRValue vr, vi;
            if (e->u.cast.operand->type.kind == TY_STRUCT && e->u.cast.operand->type.tag &&
                strncmp(e->u.cast.operand->type.tag, "__complex_", 10) == 0) {
                /* Complex to complex cast (e.g. float to double, double to int) */
                IRValue src_addr = lower_expr(fn, st, e->u.cast.operand);
                int src_elem_sz = type_size(e->u.cast.operand->type) / 2;
                int src_is_float = (strstr(e->u.cast.operand->type.tag, "float") || strstr(e->u.cast.operand->type.tag, "double") || strstr(e->u.cast.operand->type.tag, "ldouble")) ? 1 : 0;
                int src_is_unsigned = (strstr(e->u.cast.operand->type.tag, "unsigned") != NULL);
                vr = new_value(fn);
                emit_inst_w(fn, IR_LOAD_PTR, vr, src_addr, -1, 0, src_elem_sz, src_is_unsigned, e->loc);
                if (src_is_float) set_value_float(fn, vr, 1);
                IRValue off = new_value(fn);
                emit_inst_w(fn, IR_CONST, off, -1, -1, src_elem_sz, 8, 1, e->loc);
                IRValue iaddr = emit_bin_w(fn, IR_ADD, src_addr, off, 8, 1, e->loc);
                vi = new_value(fn);
                emit_inst_w(fn, IR_LOAD_PTR, vi, iaddr, -1, 0, src_elem_sz, src_is_unsigned, e->loc);
                if (src_is_float) set_value_float(fn, vi, 1);
                
                if (src_is_float != is_float) {
                    if (src_is_float) {
                        vr = convert_numeric(fn, vr, src_elem_sz, elem_sz, is_unsigned, 0, e->loc);
                        vi = convert_numeric(fn, vi, src_elem_sz, elem_sz, is_unsigned, 0, e->loc);
                    } else {
                        vr = convert_numeric(fn, vr, src_elem_sz, elem_sz, 0, 1, e->loc);
                        vi = convert_numeric(fn, vi, src_elem_sz, elem_sz, 0, 1, e->loc);
                    }
                } else if (is_float) {
                    if (src_elem_sz != elem_sz) {
                        vr = convert_numeric(fn, vr, src_elem_sz, elem_sz, 0, 1, e->loc);
                        vi = convert_numeric(fn, vi, src_elem_sz, elem_sz, 0, 1, e->loc);
                    }
                } else {
                    if (src_elem_sz != elem_sz) {
                        vr = coerce(fn, vr, src_elem_sz, src_is_unsigned, elem_sz, is_unsigned, e->loc);
                        vi = coerce(fn, vi, src_elem_sz, src_is_unsigned, elem_sz, is_unsigned, e->loc);
                    }
                }
            } else {
                /* Scalar to complex cast: imag is 0 */
                vr = lower_expr(fn, st, e->u.cast.operand);
                if (is_float) {
                    if (!get_value_is_float(fn, vr) || get_value_width(fn, vr) != elem_sz)
                        vr = convert_numeric(fn, vr, get_value_width(fn, vr), elem_sz, 0, 1, e->loc);
                    vi = emit_float_const(fn, elem_sz, 0, e->loc);
                } else {
                    if (get_value_is_float(fn, vr)) {
                        vr = convert_numeric(fn, vr, get_value_width(fn, vr), elem_sz, is_unsigned, 0, e->loc);
                    } else if (get_value_width(fn, vr) != elem_sz) {
                        vr = coerce(fn, vr, get_value_width(fn, vr), get_value_is_unsigned(fn, vr), elem_sz, is_unsigned, e->loc);
                    }
                    vi = new_value(fn);
                    emit_inst_w(fn, IR_CONST, vi, -1, -1, 0, elem_sz, is_unsigned, e->loc);
                }
            }
            IRValue slot = emit_alloca(fn, total_sz, 8, 1, e->loc);
            IRValue addr = emit_bin_w(fn, IR_ADDR, slot, -1, 8, 1, e->loc);
            emit_inst_w(fn, IR_STORE_PTR, -1, addr, vr, 0, elem_sz, is_unsigned, e->loc);
            IRValue off = new_value(fn);
            emit_inst_w(fn, IR_CONST, off, -1, -1, elem_sz, 8, 1, e->loc);
            IRValue iaddr = emit_bin_w(fn, IR_ADD, addr, off, 8, 1, e->loc);
            emit_inst_w(fn, IR_STORE_PTR, -1, iaddr, vi, 0, elem_sz, is_unsigned, e->loc);
            return addr;
        }
        if (e->u.cast.operand->type.kind == TY_STRUCT && e->u.cast.operand->type.tag &&
            strncmp(e->u.cast.operand->type.tag, "__complex_", 10) == 0 &&
            (e->type.kind == TY_INT || e->type.kind == TY_FLOAT || e->type.is_bool)) {
            IRValue src_addr = lower_expr(fn, st, e->u.cast.operand);
            int src_elem_sz = type_size(e->u.cast.operand->type) / 2;
            int src_is_float = (strstr(e->u.cast.operand->type.tag, "float") || strstr(e->u.cast.operand->type.tag, "double") || strstr(e->u.cast.operand->type.tag, "ldouble")) ? 1 : 0;
            IRValue vr = new_value(fn);
            emit_inst_w(fn, IR_LOAD_PTR, vr, src_addr, -1, 0, src_elem_sz, 1, e->loc);
            if (src_is_float) set_value_float(fn, vr, 1);
            int vw = get_value_width(fn, vr), vu = get_value_is_unsigned(fn, vr), vf = get_value_is_float(fn, vr);
            int dw = e->type.kind == TY_PTR ? 8 : (e->type.width ? e->type.width : 4);
            int du = e->type.is_unsigned, df = (e->type.kind == TY_FLOAT);
            if (e->type.is_bool) {
                IRValue n = bool_normalize(fn, vr, vw, vu, vf, e->loc);
                return coerce(fn, n, 4, 0, dw, du, e->loc);
            }
            if (vf != df) return convert_numeric(fn, vr, vw, dw, du, df, e->loc);
            if (df) return (vw == dw) ? vr : convert_numeric(fn, vr, vw, dw, du, 1, e->loc);
            return coerce(fn, vr, vw, vu, dw, du, e->loc);
        }
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

        if (lv->type.kind == TY_FLOAT) {
            /* ++x on a float steps by 1.0 in the float domain; the integer
             * path below would add the integer 1 to the bit pattern. */
            IRValue addr = lower_lvalue_addr(fn, st, lv);
            IRValue old = new_value(fn);
            emit_inst_w(fn, IR_LOAD_PTR, old, addr, -1, 0, lw, lu, e->loc);
            set_value_float(fn, old, 1);
            IRValue one;
            if (lw == 16) {
                one = emit_ld_const(fn, 1.0L, e->loc);
            } else {
                int64_t bits;
                if (lw == 4) { float f = 1.0f; bits = 0; memcpy(&bits, &f, sizeof f); }
                else { double d = 1.0; memcpy(&bits, &d, sizeof d); }
                one = emit_float_const(fn, lw, bits, e->loc);
            }
            IRValue neu = emit_bin_w(fn, is_inc ? IR_FADD : IR_FSUB, old, one,
                                     lw, lu, e->loc);
            set_value_float(fn, neu, 1);
            emit_inst_w(fn, IR_STORE_PTR, -1, addr, neu, 0, lw, lu, e->loc);
            return is_prefix ? neu : old;
        }

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

        /* Bitfield member: full RMW with masking. */
        int bf_width = 0, bf_offset = 0, bf_unit = 0;
        if (lv->kind == EX_MEMBER
            && member_bitfield(lv, &bf_width, &bf_offset, &bf_unit)) {
            int uw = bf_unit ? bf_unit : 4;
            int64_t mask = bitfield_mask64(bf_width);
            /* Load the full storage unit. */
            IRValue unit = new_value(fn);
            emit_inst_w(fn, IR_LOAD_PTR, unit, addr, -1, 0, uw, 1, e->loc);
            /* Extract the old bitfield value: (unit >> bf_offset) & mask. */
            IRValue old_bf;
            if (bf_offset > 0) {
                IRValue sh = new_value(fn);
                emit_inst_w(fn, IR_CONST, sh, -1, -1, bf_offset, 8, 1, e->loc);
                IRValue shifted = new_value(fn);
                emit_inst_w(fn, IR_SHR, shifted, unit, sh, 0, uw, 1, e->loc);
                old_bf = shifted;
            } else {
                old_bf = unit;
            }
            IRValue m_mask = new_value(fn);
            emit_inst_w(fn, IR_CONST, m_mask, -1, -1, mask, uw, 1, e->loc);
            IRValue old_val = new_value(fn);
            emit_inst_w(fn, IR_BAND, old_val, old_bf, m_mask, 0, uw, 1, e->loc);
            /* Compute new value = (old_val +/- 1) & mask. */
            IRValue one = new_value(fn);
            emit_inst_w(fn, IR_CONST, one, -1, -1, 1, uw, 1, e->loc);
            IRValue new_val = new_value(fn);
            emit_inst_w(fn, is_inc ? IR_ADD : IR_SUB, new_val, old_val, one, 0, uw, 1, e->loc);
            IRValue new_masked = new_value(fn);
            emit_inst_w(fn, IR_BAND, new_masked, new_val, m_mask, 0, uw, 1, e->loc);
            /* Shift new_masked into position. */
            IRValue shifted_new;
            if (bf_offset > 0) {
                IRValue sh2 = new_value(fn);
                emit_inst_w(fn, IR_CONST, sh2, -1, -1, bf_offset, 8, 1, e->loc);
                shifted_new = new_value(fn);
                emit_inst_w(fn, IR_SHL, shifted_new, new_masked, sh2, 0, uw, 1, e->loc);
            } else {
                shifted_new = new_masked;
            }
            /* Clear the bitfield in the unit and OR in the new value. */
            IRValue clr = new_value(fn);
            emit_inst_w(fn, IR_CONST, clr, -1, -1, ~(mask << bf_offset), uw, 1, e->loc);
            IRValue unit_cleared = new_value(fn);
            emit_inst_w(fn, IR_BAND, unit_cleared, unit, clr, 0, uw, 1, e->loc);
            IRValue unit_new = new_value(fn);
            emit_inst_w(fn, IR_BOR, unit_new, unit_cleared, shifted_new, 0, uw, 1, e->loc);
            emit_inst_w(fn, IR_STORE_PTR, -1, addr, unit_new, 0, uw, 1, e->loc);
            /* Return old or new bitfield value coerced to the member's type. */
            IRValue ret_bf = is_prefix ? new_masked : old_val;
            return coerce(fn, ret_bf, uw, 1,
                          lv->type.width ? lv->type.width : 4,
                          lv->type.is_unsigned, e->loc);
        }

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
        if (lv->type.kind == TY_STRUCT && lv->type.tag && strncmp(lv->type.tag, "__complex_", 10) == 0) {
            IRValue addr = lower_lvalue_addr(fn, st, lv);
            Expr fake_bin;
            memset(&fake_bin, 0, sizeof(fake_bin));
            fake_bin.kind = EX_BINOP;
            fake_bin.type = lv->type;
            fake_bin.loc = e->loc;
            fake_bin.u.bin.op = op;
            fake_bin.u.bin.l = lv;
            fake_bin.u.bin.r = e->u.comp.rvalue;
            IRValue res_addr = lower_complex_binop(fn, st, &fake_bin, lv->type, e->u.comp.rvalue->type, op);
            emit_struct_copy(fn, addr, res_addr, type_size(lv->type), e->loc);
            return addr;
        }
        int is_ptr = (lv->type.kind == TY_PTR);
        int is_float = (lv->type.kind == TY_FLOAT);
        int lw = is_ptr ? 8 : (lv->type.width ? lv->type.width : 4);
        int lu = is_ptr ? 1 : lv->type.is_unsigned;
        /* C: E1 op= E2 is E1 = E1 op E2 (E1 once).  Integer operands are
         * promoted before the op, then the result converted back for the store
         * (20030128-1.c: unsigned char x /= short y). */
        int arith_w = lw, arith_u = lu;
        if (!is_ptr && !is_float && lw < 4) {
            arith_w = 4;
            arith_u = 0;
        }
        IROpcode ir_op;
        if (is_float) {
            switch (op) {
            case BOP_SUB: ir_op = IR_FSUB; break;
            case BOP_MUL: ir_op = IR_FMUL; break;
            case BOP_DIV: ir_op = IR_FDIV; break;
            default:      ir_op = IR_FADD; break;
            }
        } else {
            ir_op = bop_to_ir(op);
        }

        /* Promotable path: simple non-pinned, non-global scalar variable. */
        if (lv->kind == EX_VAR && !is_float) {
            const IRSlot *entry = irsymtable_find(st, lv->u.var.name);
            if (entry && !entry->is_global && !entry->pinned) {
                IRValue old = new_value(fn);
                emit_inst_w(fn, IR_LOAD, old, entry->slot, -1, 0, lw, lu, e->loc);
                IRValue rhs = lower_expr(fn, st, e->u.comp.rvalue);
                IRValue scaled = is_ptr
                    ? scale_rhs(fn, rhs, is_ptr, lv->type, op, e->loc)
                    : coerce(fn, rhs, get_value_width(fn, rhs),
                             get_value_is_unsigned(fn, rhs), arith_w, arith_u, e->loc);
                IRValue old_p = (arith_w == lw && arith_u == lu)
                    ? old : coerce(fn, old, lw, lu, arith_w, arith_u, e->loc);
                IRValue neu = emit_bin_w(fn, ir_op, old_p, scaled, arith_w, arith_u, e->loc);
                IRValue back = (arith_w == lw && arith_u == lu)
                    ? neu : coerce(fn, neu, arith_w, arith_u, lw, lu, e->loc);
                emit_inst_w(fn, IR_STORE, -1, entry->slot, back, 0, lw, lu, e->loc);
                return back;
            }
        }

        /* General path (pinned var, global, deref, index, member). */
        IRValue addr = lower_lvalue_addr(fn, st, lv);

        /* Bitfield member: extract, op, insert.  A plain LOAD/STORE of the
         * storage unit would clobber adjacent fields sharing the same word
         * (gcc.c-torture/execute/20000113-1.c). */
        int bf_width = 0, bf_offset = 0, bf_unit = 0;
        if (!is_float && !is_ptr && lv->kind == EX_MEMBER
            && member_bitfield(lv, &bf_width, &bf_offset, &bf_unit)) {
            int uw = bf_unit ? bf_unit : 4;
            int64_t mask = bitfield_mask64(bf_width);
            IRValue unit = new_value(fn);
            emit_inst_w(fn, IR_LOAD_PTR, unit, addr, -1, 0, uw, 1, e->loc);
            IRValue extracted;
            if (bf_offset > 0) {
                IRValue sh = new_value(fn);
                emit_inst_w(fn, IR_CONST, sh, -1, -1, bf_offset, 8, 1, e->loc);
                extracted = new_value(fn);
                emit_inst_w(fn, IR_SHR, extracted, unit, sh, 0, uw, 1, e->loc);
            } else {
                extracted = unit;
            }
            IRValue old_val;
            if (!lv->type.is_unsigned && !lv->type.is_bool && bf_width < uw * 8) {
                int shift = uw * 8 - bf_width;
                IRValue s = new_value(fn);
                emit_inst_w(fn, IR_CONST, s, -1, -1, shift, 8, 1, e->loc);
                IRValue up = new_value(fn);
                emit_inst_w(fn, IR_SHL, up, extracted, s, 0, uw, 0, e->loc);
                old_val = new_value(fn);
                emit_inst_w(fn, IR_SHR, old_val, up, s, 0, uw, 0, e->loc);
            } else {
                IRValue m_mask = new_value(fn);
                emit_inst_w(fn, IR_CONST, m_mask, -1, -1, mask, uw, 1, e->loc);
                old_val = new_value(fn);
                emit_inst_w(fn, IR_BAND, old_val, extracted, m_mask, 0, uw, 1, e->loc);
            }
            int cw = (uw > 4) ? uw : 4;
            int cu = (lv->type.is_unsigned && bf_width >= 32) ? 1 : 0;
            if (uw > 4) cu = lv->type.is_unsigned;
            IRValue old_p = coerce(fn, old_val, uw,
                                   (!lv->type.is_unsigned && !lv->type.is_bool) ? 0 : 1,
                                   cw, cu, e->loc);
            IRValue rhs = lower_expr(fn, st, e->u.comp.rvalue);
            int rw = get_value_width(fn, rhs), ru = get_value_is_unsigned(fn, rhs);
            IRValue rhs_p = coerce(fn, rhs, rw, ru, cw, cu, e->loc);
            IRValue neu = emit_bin_w(fn, ir_op, old_p, rhs_p, cw, cu, e->loc);
            IRValue neu_u = coerce(fn, neu, cw, cu, uw, 1, e->loc);
            IRValue m_mask2 = new_value(fn);
            emit_inst_w(fn, IR_CONST, m_mask2, -1, -1, mask, uw, 1, e->loc);
            IRValue new_masked = new_value(fn);
            emit_inst_w(fn, IR_BAND, new_masked, neu_u, m_mask2, 0, uw, 1, e->loc);
            IRValue shifted_new;
            if (bf_offset > 0) {
                IRValue sh2 = new_value(fn);
                emit_inst_w(fn, IR_CONST, sh2, -1, -1, bf_offset, 8, 1, e->loc);
                shifted_new = new_value(fn);
                emit_inst_w(fn, IR_SHL, shifted_new, new_masked, sh2, 0, uw, 1, e->loc);
            } else {
                shifted_new = new_masked;
            }
            IRValue clr = new_value(fn);
            emit_inst_w(fn, IR_CONST, clr, -1, -1, ~(mask << bf_offset), uw, 1, e->loc);
            IRValue unit_cleared = new_value(fn);
            emit_inst_w(fn, IR_BAND, unit_cleared, unit, clr, 0, uw, 1, e->loc);
            IRValue unit_new = new_value(fn);
            emit_inst_w(fn, IR_BOR, unit_new, unit_cleared, shifted_new, 0, uw, 1, e->loc);
            emit_inst_w(fn, IR_STORE_PTR, -1, addr, unit_new, 0, uw, 1, e->loc);
            return coerce(fn, new_masked, uw, 1,
                          lv->type.width ? lv->type.width : 4,
                          lv->type.is_unsigned, e->loc);
        }

        IRValue old = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, old, addr, -1, 0, lw, lu, e->loc);
        if (is_float) set_value_float(fn, old, 1);
        IRValue rhs = lower_expr(fn, st, e->u.comp.rvalue);
        IRValue scaled;
        if (is_float) {
            /* The right operand joins the lvalue's float type, whether it
             * arrives as an int or as a float of another width. */
            int rw = get_value_width(fn, rhs);
            scaled = (get_value_is_float(fn, rhs) && rw == lw)
                     ? rhs
                     : convert_numeric(fn, rhs, rw, lw, 0, 1, e->loc);
        } else if (is_ptr) {
            scaled = scale_rhs(fn, rhs, is_ptr, lv->type, op, e->loc);
        } else {
            scaled = coerce(fn, rhs, get_value_width(fn, rhs),
                            get_value_is_unsigned(fn, rhs), arith_w, arith_u, e->loc);
        }
        IRValue old_p = old;
        if (!is_float && !is_ptr && (arith_w != lw || arith_u != lu))
            old_p = coerce(fn, old, lw, lu, arith_w, arith_u, e->loc);
        IRValue neu = emit_bin_w(fn, ir_op, old_p, scaled,
                                 is_float ? lw : arith_w,
                                 is_float ? lu : arith_u, e->loc);
        if (is_float) set_value_float(fn, neu, 1);
        IRValue back = neu;
        if (!is_float && !is_ptr && (arith_w != lw || arith_u != lu))
            back = coerce(fn, neu, arith_w, arith_u, lw, lu, e->loc);
        emit_inst_w(fn, IR_STORE_PTR, -1, addr, back, 0, lw, lu, e->loc);
        return back;
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
    case EX_STMT_EXPR: {
        size_t mark = st->len;
        IRValue res = -1;
        const StmtArray *stmts = e->u.stmt_expr.stmts;
        if (stmts) {
            for (size_t i = 0; i < stmts->len; i++) {
                const Stmt *s = &stmts->data[i];
                if (i == stmts->len - 1 && s->kind == ST_EXPR && s->u.expr) {
                    res = lower_expr(fn, st, s->u.expr);
                } else {
                    lower_stmt(fn, st, s, g_ir_cur_fd);
                }
            }
        }
        st->len = mark;
        return res;
    }
    case EX_LABEL_ADDR: {
        /* &&label — emit IR_LADDR with the label's IR id.
         * The label must have been pre-assigned in the label pre-pass. */
        const char *lbl_name = e->u.label_addr.label;
        int lbl_id = g_ir_label_map ? labelmap_find(g_ir_label_map, lbl_name) : -1;
        if (lbl_id < 0) {
            fprintf(stderr, "fakecc: &&%s: undeclared label\n", lbl_name);
            exit(1);
        }
        IRValue dst = new_value(fn);
        emit_inst(fn, IR_LADDR, dst, -1, -1, lbl_id, e->loc);
        return dst;
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

static IRValue lower_condition(IRFunction *fn, IRSymTable *st, const Expr *e) {
    if (e->type.kind == TY_STRUCT && e->type.tag && strncmp(e->type.tag, "__complex_", 10) == 0) {
        Type cty = e->type;
        int elem_sz = type_size(cty) / 2;
        int is_float = (strstr(cty.tag, "float") || strstr(cty.tag, "double") || strstr(cty.tag, "ldouble")) ? 1 : 0;
        IRValue addr = lower_expr(fn, st, e);
        IRValue vr = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, vr, addr, -1, 0, elem_sz, 1, e->loc);
        if (is_float) set_value_float(fn, vr, 1);
        IRValue off = new_value(fn);
        emit_inst_w(fn, IR_CONST, off, -1, -1, elem_sz, 8, 1, e->loc);
        IRValue iaddr = emit_bin_w(fn, IR_ADD, addr, off, 8, 1, e->loc);
        IRValue vi = new_value(fn);
        emit_inst_w(fn, IR_LOAD_PTR, vi, iaddr, -1, 0, elem_sz, 1, e->loc);
        if (is_float) set_value_float(fn, vi, 1);
        
        IRValue cmp_r, cmp_i;
        if (is_float) {
            IRValue zero = emit_float_const(fn, elem_sz, 0, e->loc);
            cmp_r = emit_bin_w(fn, IR_FCMP, vr, zero, elem_sz, 5 /* NE */, e->loc);
            cmp_i = emit_bin_w(fn, IR_FCMP, vi, zero, elem_sz, 5 /* NE */, e->loc);
        } else {
            IRValue zero = new_value(fn);
            emit_inst_w(fn, IR_CONST, zero, -1, -1, 0, elem_sz, 1, e->loc);
            cmp_r = emit_bin_w(fn, IR_NE, vr, zero, elem_sz, 1, e->loc);
            cmp_i = emit_bin_w(fn, IR_NE, vi, zero, elem_sz, 1, e->loc);
        }
        return emit_bin_w(fn, IR_BOR, cmp_r, cmp_i, 4, 0, e->loc);
    }
    return lower_expr(fn, st, e);
}

/* Label map: label name → IR label id.  Populated by a pre-pass over the
 * function body so forward gotos resolve to ids assigned before lowering. */
/* (LabelMap typedef and g_ir_label_map are forward-declared at the top of this file) */

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

/* (g_ir_label_map is declared at the top of this file.) */


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

        /* Block-scope `extern` is a re-declaration of a file-scope symbol, not
         * a definition: it must not allocate storage.  Bind the name to the
         * global so reads/writes go through IR_GADDR, exactly as a file-scope
         * declaration would.  Falling through to the ordinary local path
         * instead would give the name a fresh zero-initialized stack slot that
         * silently shadows the global.
         *
         * A block-scope `extern` function declaration needs no binding at all:
         * the call site dispatches on the callee's TY_FUNC type, and binding
         * the name as a data global would misroute an address-of. */
        if (dty.kind == TY_FUNC || s->u.decl.storage_class == 2) {
            if (dty.kind != TY_FUNC)
                irsymtable_push_global(st, s->u.decl.name, dty);
            break;
        }

        /* static local → allocate in static storage via a mangled global
         * `fn.varname`.  Reads/writes resolve through the global_name path.
         * Exception: if the initializer contains code label addresses (&&label),
         * it must be initialized dynamically within the function stack. */
        if (s->u.decl.storage_class == 1 && s->u.decl.init && expr_has_label_addr(s->u.decl.init)) {
            /* fall through to local stack allocation + lower_init_list */
        } else if (s->u.decl.storage_class == 1) {
            int sz = type_size(dty);
            if (sz <= 0) sz = 8;
            char *bytes = calloc(sz, 1);
            if (!bytes) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
            char mangled[256];
            snprintf(mangled, sizeof mangled, "%s.%s", cur_fd->name,
                     s->u.decl.name);
            /* mangled name is file-local by construction → LOCAL linkage.
             * Created before packing so a pointer slot in the initializer can
             * attach its link-time fixup to this global. */
            IRGlobal *sg = ir_module_push_global(g_ir_module, mangled, sz, bytes,
                                                 0, 1, s->loc);
            if (s->u.decl.init) {
                pack_init(g_ir_module, &dty, s->u.decl.init, bytes, sz,
                          s->u.decl.name, s->loc, sg);
                flush_rodata(g_ir_module);
            }
            irsymtable_push_static_local(st, s->u.decl.name, mangled, dty);
            break;
        }

        int pinned = is_pinned_in_body(cur_fd, s->u.decl.name, dty);
        if (g_ir_pin_locals)
            pinned = 1;
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
        ir_add_dbg_var(fn, s->u.decl.name, s->loc, IR_DBG_LOCAL, dty, v, -1);
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
            } else if (dty.kind == TY_STRUCT) {
                IRValue addr = emit_bin_w(fn, IR_ADDR, v, -1, 8, 1, s->loc);
                if (dty.tag && strncmp(dty.tag, "__complex_", 10) == 0 &&
                    (s->u.decl.init->type.kind != TY_STRUCT ||
                     !s->u.decl.init->type.tag ||
                     strcmp(dty.tag, s->u.decl.init->type.tag) != 0)) {
                    Expr fake_cast;
                    memset(&fake_cast, 0, sizeof(fake_cast));
                    fake_cast.kind = EX_CAST;
                    fake_cast.type = dty;
                    fake_cast.loc = s->loc;
                    fake_cast.u.cast.target = dty;
                    fake_cast.u.cast.operand = s->u.decl.init;
                    IRValue cplx_rv = lower_expr(fn, st, &fake_cast);
                    emit_struct_copy(fn, addr, cplx_rv, type_size(dty), s->loc);
                    break;
                }
                IRValue rv = lower_expr(fn, st, s->u.decl.init);
                emit_struct_copy(fn, addr, rv, type_size(dty), s->loc);
                break;
            } else {
                IRValue rv = lower_expr(fn, st, s->u.decl.init);
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
            /* void function: bare `return;` or `return void_expr;` */
            if (s->u.value) lower_expr(fn, st, s->u.value);
            emit_inst_w(fn, IR_RETURN, -1, -1, -1, 0, 0, 0, s->loc);
        } else if (fn->ret_is_struct) {
            IRValue v = lower_expr(fn, st, s->u.value);
            if (fn->ret_reg_n > 0) {
                /* SysV register return: load eightbytes into RAX(/RDX) or
                 * XMM0(/XMM1).  IR_RETURN.a = lo, .b = hi (or -1). */
                SysVRegClass cls[2];
                cls[0] = (SysVRegClass)fn->ret_reg_cls[0];
                cls[1] = (SysVRegClass)fn->ret_reg_cls[1];
                IRValue ebs[2];
                load_agg_regs(fn, v, fn->ret_width, fn->ret_reg_n, cls, ebs,
                              s->loc);
                IRValue hi = (fn->ret_reg_n > 1) ? ebs[1] : -1;
                emit_inst_w(fn, IR_RETURN, -1, ebs[0], hi, 0, 8, 1, s->loc);
                if (cls[0] == SYSV_CLS_SSE)
                    fn->insts.data[fn->insts.len - 1].is_float = 1;
            } else {
                /* MEMORY class: copy into hidden sret slot; return pointer. */
                emit_struct_copy(fn, fn->sret_value, v, fn->ret_width, s->loc);
                emit_inst_w(fn, IR_RETURN, -1, fn->sret_value, -1, 0,
                            8, 1, s->loc);
            }
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
        IRValue cond = lower_condition(fn, st, s->u.if_s.cond);
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
        IRValue cond = lower_condition(fn, st, s->u.while_s.cond);
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
        IRValue cond = lower_condition(fn, st, s->u.do_s.cond);
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
                 * loop variable from cond/step/body). */
                for (size_t i = 0; i < s->u.for_s.init->u.block.len; i++)
                    lower_stmt(fn, st, &s->u.for_s.init->u.block.data[i], cur_fd);
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
            IRValue cond = lower_condition(fn, st, s->u.for_s.cond);
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
    case ST_CONTINUE: {
        int cont = g_loops[g_loop_depth - 1].cont_label;
        /* Emit BR to the cont_label through an intermediate label so that
         * continues follow the same path as the initial loop entry.  This
         * works around a codegen issue that manifests in large functions
         * when many continues jump directly to the loop header. */
        int thunk = new_label(fn);
        emit_br(fn, thunk, s->loc);
        emit_label(fn, thunk, s->loc);
        emit_br(fn, cont, s->loc);
        break;
    }
    case ST_BLOCK: {
        for (size_t i = 0; i < s->u.block.len; i++) {
            lower_stmt(fn, st, &s->u.block.data[i], cur_fd);
        }
        break;
    }
    case ST_GOTO: {
        if (s->u.goto_s.target_expr) {
            /* Computed goto: goto *expr; — lower the pointer, emit IR_JMP_PTR */
            IRValue ptr = lower_expr(fn, st, s->u.goto_s.target_expr);
            emit_inst(fn, IR_JMP_PTR, -1, ptr, -1, -1, s->loc);
        } else {
            int id = labelmap_find(g_ir_label_map, s->u.goto_s.target);
            if (id < 0) {
                fprintf(stderr, "fakecc: goto to unknown label '%s'\n",
                        s->u.goto_s.target);
                exit(1);
            }
            emit_br(fn, id, s->loc);
        }
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

/* Rodata globals created while packing an initializer (`const char *p = "x"`
 * needs the bytes to live somewhere before the slot can point at them).  They
 * are queued instead of pushed immediately: pushing reallocs the module's
 * global array, which would dangle the `IRGlobal *g` that pack_init is
 * attaching fixups to. */
typedef struct {
    char *name;
    char *bytes;
    int   size;
    SourceLoc loc;
} PendingRodata;
static PendingRodata *g_pending_rodata = NULL;
static int g_pending_rodata_len = 0;
static int g_pending_rodata_cap = 0;

static void queue_rodata(const char *name, char *bytes, int size, SourceLoc loc) {
    if (g_pending_rodata_len >= g_pending_rodata_cap) {
        int nc = g_pending_rodata_cap ? g_pending_rodata_cap * 2 : 4;
        g_pending_rodata = realloc(g_pending_rodata, nc * sizeof(PendingRodata));
        if (!g_pending_rodata) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        g_pending_rodata_cap = nc;
    }
    g_pending_rodata[g_pending_rodata_len].name = xstrdup(name);
    g_pending_rodata[g_pending_rodata_len].bytes = bytes;
    g_pending_rodata[g_pending_rodata_len].size = size;
    g_pending_rodata[g_pending_rodata_len].loc = loc;
    g_pending_rodata_len++;
}

static void flush_rodata(IRModule *m) {
    for (int i = 0; i < g_pending_rodata_len; i++) {
        ir_module_push_global(m, g_pending_rodata[i].name, g_pending_rodata[i].size,
                              g_pending_rodata[i].bytes, 1, 1, g_pending_rodata[i].loc);
        free(g_pending_rodata[i].name);
    }
    g_pending_rodata_len = 0;
}

/* Evaluate a constant expression as a long double, so a float-typed global can
 * be packed in its own format.  Integer constants are accepted too: `double
 * d = 1;` must store 1.0, not the bit pattern of the integer 1. */
static int fold_const_float(const Expr *e, long double *out) {
    if (!e) return 0;
    if (e->kind == EX_FLOAT_LIT) {
        *out = strtold(e->u.float_text, NULL);
        return 1;
    }
    if (e->kind == EX_CAST)
        return fold_const_float(e->u.cast.operand, out);
    if (e->kind == EX_UNARY
        && (e->u.un.op == UOP_NEG || e->u.un.op == UOP_POS)) {
        if (!fold_const_float(e->u.un.operand, out)) return 0;
        if (e->u.un.op == UOP_NEG) *out = -*out;
        return 1;
    }
    if (e->kind == EX_INT_LIT) {
        *out = (long double)e->u.int_val;
        return 1;
    }
    if (e->kind == EX_BINOP) {
        long double l = 0, r = 0;
        if (!fold_const_float(e->u.bin.l, &l) || !fold_const_float(e->u.bin.r, &r))
            return 0;
        switch (e->u.bin.op) {
        case BOP_ADD: *out = l + r; return 1;
        case BOP_SUB: *out = l - r; return 1;
        case BOP_MUL: *out = l * r; return 1;
        case BOP_DIV: if (r == 0) return 0; *out = l / r; return 1;
        default: return 0;
        }
    }
    long long iv;
    if (fold_const_int(e, &iv)) {
        *out = (long double)iv;
        return 1;
    }
    return 0;
}

static int fold_const_complex(const Expr *e, long double *r_out, long double *i_out) {
    if (!e) return 0;
    if (e->kind == EX_FLOAT_LIT) {
        *r_out = strtold(e->u.float_text, NULL);
        *i_out = 0.0;
        return 1;
    }
    if (e->kind == EX_INT_LIT) {
        *r_out = (long double)e->u.int_val;
        *i_out = 0.0;
        return 1;
    }
    if (e->kind == EX_COMPOUND_LITERAL && e->u.compound.init && e->u.compound.init->kind == EX_INIT_LIST) {
        int n = e->u.compound.init->u.init_list.num_elements;
        long double r = 0.0, i = 0.0, dummy = 0.0;
        if (n >= 1) fold_const_complex(e->u.compound.init->u.init_list.elements[0], &r, &dummy);
        if (n >= 2) fold_const_complex(e->u.compound.init->u.init_list.elements[1], &i, &dummy);
        *r_out = r;
        *i_out = i;
        return 1;
    }
    if (e->kind == EX_UNARY) {
        long double r = 0.0, i = 0.0;
        if (!fold_const_complex(e->u.un.operand, &r, &i)) return 0;
        if (e->u.un.op == UOP_NEG) { *r_out = -r; *i_out = -i; return 1; }
        if (e->u.un.op == UOP_POS) { *r_out = r; *i_out = i; return 1; }
        if (e->u.un.op == UOP_BITNOT) { *r_out = r; *i_out = -i; return 1; }
    }
    if (e->kind == EX_BINOP) {
        long double lr = 0.0, li = 0.0, rr = 0.0, ri = 0.0;
        if (!fold_const_complex(e->u.bin.l, &lr, &li)) return 0;
        if (!fold_const_complex(e->u.bin.r, &rr, &ri)) return 0;
        if (e->u.bin.op == BOP_ADD) { *r_out = lr + rr; *i_out = li + ri; return 1; }
        if (e->u.bin.op == BOP_SUB) { *r_out = lr - rr; *i_out = li - ri; return 1; }
        if (e->u.bin.op == BOP_MUL) { *r_out = lr * rr - li * ri; *i_out = lr * ri + li * rr; return 1; }
        if (e->u.bin.op == BOP_DIV) {
            long double denom = rr * rr + ri * ri;
            if (denom == 0.0) return 0;
            *r_out = (lr * rr + li * ri) / denom;
            *i_out = (li * rr - lr * ri) / denom;
            return 1;
        }
    }
    return 0;
}

/* Pack a compile-time constant initializer into `bytes` (size `sz`) for a
 * global of type `ty`.  Handles integer literals, negated literals, nested
 * initializer lists (positional, per-element/per-member offset), string
 * literals for `char[]="..."`, cast expressions, and references to previously
 * packed const globals.  `ir` is the module whose globals may be referenced.
 * Sema guarantees every element is constant. */
static int get_global_elem_size(const Expr *e) {
    if (!e) return 1;
    if (e->type.kind == TY_PTR && e->type.pointee)
        return type_size(*e->type.pointee);
    if (e->type.kind == TY_ARRAY && e->type.elem_type)
        return type_size(*e->type.elem_type);
    if (e->kind == EX_VAR && g_ir_tu) {
        for (size_t i = 0; i < g_ir_tu->globals.len; i++) {
            if (g_ir_tu->globals.data[i].kind == ST_DECL &&
                strcmp(g_ir_tu->globals.data[i].u.decl.name, e->u.var.name) == 0) {
                Type gt = g_ir_tu->globals.data[i].u.decl.type;
                if (gt.kind == TY_ARRAY && gt.elem_type)
                    return type_size(*gt.elem_type);
                if (gt.kind == TY_PTR && gt.pointee)
                    return type_size(*gt.pointee);
            }
        }
    }
    if (e->kind == EX_BINOP) {
        return get_global_elem_size(e->u.bin.l);
    }
    if (e->kind == EX_CAST) {
        return get_global_elem_size(e->u.cast.operand);
    }
    return 1;
}

static Type get_global_obj_struct_type(const Expr *e) {
    Type t = e->type;
    if (t.kind == TY_VOID && e->kind == EX_DEREF) {
        return get_global_obj_struct_type(e->u.deref.operand);
    }
    if (t.kind == TY_VOID && e->kind == EX_VAR && g_ir_tu) {
        for (size_t i = 0; i < g_ir_tu->globals.len; i++) {
            if (g_ir_tu->globals.data[i].kind == ST_DECL &&
                strcmp(g_ir_tu->globals.data[i].u.decl.name, e->u.var.name) == 0) {
                t = g_ir_tu->globals.data[i].u.decl.type;
                break;
            }
        }
    }
    if (t.kind == TY_VOID && e->kind == EX_BINOP) {
        return get_global_obj_struct_type(e->u.bin.l);
    }
    if (t.kind == TY_PTR && t.pointee)
        t = *t.pointee;
    else if (t.kind == TY_ARRAY && t.elem_type)
        t = *t.elem_type;
    return t;
}

static int eval_global_addr_offset(const Expr *e, const char **out_sym, int *out_offset) {
    if (!e) return 0;
    if (e->kind == EX_CAST) {
        return eval_global_addr_offset(e->u.cast.operand, out_sym, out_offset);
    }
    if (e->kind == EX_VAR) {
        *out_sym = e->u.var.name;
        *out_offset = 0;
        return 1;
    }
    if (e->kind == EX_ADDR) {
        const Expr *sub = e->u.addr.operand;
        while (sub && sub->kind == EX_CAST) sub = sub->u.cast.operand;
        if (!sub) return 0;
        if (sub->kind == EX_VAR) {
            *out_sym = sub->u.var.name;
            *out_offset = 0;
            return 1;
        }
        if (sub->kind == EX_MEMBER) {
            const char *sym = NULL;
            int off = 0;
            if (!eval_global_addr_offset(sub->u.member.obj, &sym, &off))
                return 0;
            Type m_target_ty = get_global_obj_struct_type(sub->u.member.obj);
            if (m_target_ty.kind == TY_STRUCT && m_target_ty.tag) {
                const StructDef *sd = struct_registry_find_c(g_ir_structs, m_target_ty.tag);
                if (sd) {
                    int moff = 0;
                    if (struct_lookup_member(g_ir_structs, sd, sub->u.member.name, &moff)) {
                        off += moff;
                        *out_sym = sym;
                        *out_offset = off;
                        return 1;
                    }
                }
            }
            *out_sym = sym;
            *out_offset = off;
            return 1;
        }
        if (sub->kind == EX_DEREF) {
            return eval_global_addr_offset(sub->u.deref.operand, out_sym, out_offset);
        }
        if (sub->kind == EX_INDEX) {
            const char *sym = NULL;
            int off = 0;
            if (!eval_global_addr_offset(sub->u.idx.array, &sym, &off))
                return 0;
            long long idx = 0;
            if (!fold_const_int(sub->u.idx.index, &idx))
                return 0;
            int esz = get_global_elem_size(sub->u.idx.array);
            *out_sym = sym;
            *out_offset = off + (int)(idx * esz);
            return 1;
        }
        return eval_global_addr_offset(sub, out_sym, out_offset);
    }
    if (e->kind == EX_BINOP) {
        if (e->u.bin.op == BOP_ADD) {
            const char *sym = NULL;
            int off = 0;
            long long delta = 0;
            if (eval_global_addr_offset(e->u.bin.l, &sym, &off) && fold_const_int(e->u.bin.r, &delta)) {
                int esz = get_global_elem_size(e->u.bin.l);
                *out_sym = sym;
                *out_offset = off + (int)(delta * esz);
                return 1;
            }
            if (eval_global_addr_offset(e->u.bin.r, &sym, &off) && fold_const_int(e->u.bin.l, &delta)) {
                int esz = get_global_elem_size(e->u.bin.r);
                *out_sym = sym;
                *out_offset = off + (int)(delta * esz);
                return 1;
            }
        } else if (e->u.bin.op == BOP_SUB) {
            const char *sym = NULL;
            int off = 0;
            long long delta = 0;
            if (eval_global_addr_offset(e->u.bin.l, &sym, &off) && fold_const_int(e->u.bin.r, &delta)) {
                int esz = get_global_elem_size(e->u.bin.l);
                *out_sym = sym;
                *out_offset = off - (int)(delta * esz);
                return 1;
            }
        }
    }
    if (e->kind == EX_MEMBER) {
        const char *sym = NULL;
        int off = 0;
        if (eval_global_addr_offset(e->u.member.obj, &sym, &off)) {
            Type m_target_ty = get_global_obj_struct_type(e->u.member.obj);
            if (m_target_ty.kind == TY_STRUCT && m_target_ty.tag) {
                const StructDef *sd = struct_registry_find_c(g_ir_structs, m_target_ty.tag);
                if (sd) {
                    int moff = 0;
                    if (struct_lookup_member(g_ir_structs, sd, e->u.member.name, &moff)) {
                        off += moff;
                        *out_sym = sym;
                        *out_offset = off;
                        return 1;
                    }
                }
            }
            *out_sym = sym;
            *out_offset = off;
            return 1;
        }
    }
    if (e->kind == EX_DEREF) {
        return eval_global_addr_offset(e->u.deref.operand, out_sym, out_offset);
    }
    return 0;
}

static void pack_init(const IRModule *ir, const Type *ty, const Expr *e,
                      char *bytes, int sz, const char *ctx, SourceLoc loc,
                      IRGlobal *g) {
    if (e->kind == EX_VAR && strcmp(e->u.var.name, "NULL") == 0) {
        memset(bytes, 0, sz);
        return;
    }
    if (ty->kind == TY_STRUCT && ty->tag && strncmp(ty->tag, "__complex_", 10) == 0) {
        long double cr = 0.0, ci = 0.0;
        if (fold_const_complex(e, &cr, &ci)) {
            int esz = sz / 2;
            int is_float = (strstr(ty->tag, "float") || strstr(ty->tag, "double") || strstr(ty->tag, "ldouble")) ? 1 : 0;
            if (is_float) {
                if (esz == 16) {
                    memcpy(bytes, &cr, 10);
                    memcpy(bytes + esz, &ci, 10);
                } else if (esz == 4) {
                    float fr = (float)cr, fi = (float)ci;
                    memcpy(bytes, &fr, 4);
                    memcpy(bytes + esz, &fi, 4);
                } else {
                    double dr = (double)cr, di = (double)ci;
                    memcpy(bytes, &dr, 8);
                    memcpy(bytes + esz, &di, 8);
                }
            } else {
                if (esz == 1) {
                    char c_r = (char)cr, c_i = (char)ci;
                    memcpy(bytes, &c_r, 1);
                    memcpy(bytes + esz, &c_i, 1);
                } else if (esz == 2) {
                    short s_r = (short)cr, s_i = (short)ci;
                    memcpy(bytes, &s_r, 2);
                    memcpy(bytes + esz, &s_i, 2);
                } else if (esz == 4) {
                    int i_r = (int)cr, i_i = (int)ci;
                    memcpy(bytes, &i_r, 4);
                    memcpy(bytes + esz, &i_i, 4);
                } else {
                    int64_t l_r = (int64_t)cr, l_i = (int64_t)ci;
                    memcpy(bytes, &l_r, 8);
                    memcpy(bytes + esz, &l_i, 8);
                }
            }
            return;
        }
    }
    if (e->kind == EX_INIT_LIST) {
        int n = e->u.init_list.num_elements;
        switch (ty->kind) {
        case TY_ARRAY: {
            int esz = type_size(*ty->elem_type);
            for (int i = 0; i < n; i++)
                pack_init(ir, ty->elem_type, e->u.init_list.elements[i],
                          bytes + i * esz, esz, ctx, loc, g);
            break;
        }
        case TY_STRUCT: {
            const StructDef *sd = struct_registry_find_c(g_ir_structs, ty->tag);
            if (!sd) die_at(loc.file, loc.line, loc.col,
                            "unknown struct 'struct %s'", ty->tag);
            for (int i = 0; i < n; i++)
                pack_init(ir, &sd->members[i].type, e->u.init_list.elements[i],
                          bytes + sd->members[i].offset,
                          type_size(sd->members[i].type), ctx, loc, g);
            break;
        }
        default:
            /* Scalar with a single-element brace list: `int x = {5}`. */
            pack_init(ir, ty, e->u.init_list.elements[0], bytes, sz, ctx, loc, g);
            break;
        }
        return;
    }
    if (e->kind == EX_COMPOUND_LITERAL) {
        pack_init(ir, ty, e->u.compound.init, bytes, sz, ctx, loc, g);
        return;
    }
    if (e->kind == EX_CAST) {
        /* `(T)expr` — the cast itself is a no-op at the bit level for the
         * cases we accept (e.g. `(int*)0`); pack the operand. */
        pack_init(ir, ty, e->u.cast.operand, bytes, sz, ctx, loc, g);
        return;
    }
    if (ty->kind == TY_FLOAT) {
        /* Float slots must be packed as the target's binary format, not as the
         * integer the literal would fold to. */
        long double fv;
        if (fold_const_float(e, &fv)) {
            if (ty->width == 16) {
                memcpy(bytes, &fv, 10);
            } else if (ty->width == 4) {
                float f = (float)fv;
                memcpy(bytes, &f, sizeof f);
            } else {
                double d = (double)fv;
                memcpy(bytes, &d, sizeof d);
            }
            return;
        }
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
    if (e->kind == EX_STR && ty->kind == TY_PTR && g) {
        /* `const char *p = "x"` — the slot holds the address of the bytes, so
         * park them in an anonymous rodata global and let the linker patch the
         * pointer in. */
        char name[32];
        snprintf(name, sizeof name, "__str.%d", g_str_counter++);
        int nbytes = e->u.str.len + 1;
        char *init = malloc(nbytes);
        if (!init) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        memcpy(init, e->u.str.bytes, nbytes);
        queue_rodata(name, init, nbytes, loc);
        add_global_fixup(g, (int)(bytes - g->init_bytes), name, 0);
        memset(bytes, 0, sz);  /* patched by the linker */
        return;
    }
    if (e->kind == EX_STR && ty->kind == TY_ARRAY && ty->elem_type
        && ty->elem_type->width == 1) {
        /* char[] = "..." — copy bytes (excluding the lexer's implicit NUL,
         * which we re-add) and NUL-terminate. */
        if (sz > 0) {
            int n = e->u.str.len;
            if (n > sz - 1) n = sz - 1;
            if (n > 0) memcpy(bytes, e->u.str.bytes, n);
            bytes[n > 0 ? n : 0] = '\0';
        }
        return;
    }
    /* Reference to a previously-defined const global: `.regs = ALLOCATABLE_REGS`.
     * If the target slot is a pointer, the array/struct must decay to its
     * ADDRESS (a link-time constant) — record a fixup so codegen emits a
     * R_X86_64_64 relocation.  Otherwise copy its already-packed bytes. */
    if (e->kind == EX_VAR && ir) {
        const IRGlobal *src = find_packed_global(ir, e->u.var.name);
        if (src) {
            if (ty->kind == TY_PTR && g) {
                int off = (int)(bytes - g->init_bytes);
                add_global_fixup(g, off, src->name, 0);
                memset(bytes, 0, sz);  /* patched by linker */
                return;
            }
            int n = sz < src->size ? sz : src->size;
            memcpy(bytes, src->init_bytes, n);
            return;
        }
    }
    /* Address of a global object (with optional member/array offset/arithmetic). */
    const char *addr_sym = NULL;
    int addr_offset = 0;
    if (ty->kind == TY_PTR && g && eval_global_addr_offset(e, &addr_sym, &addr_offset)) {
        add_global_fixup(g, (int)(bytes - g->init_bytes), addr_sym, addr_offset);
        memset(bytes, 0, sz);
        return;
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
                const StructMember *sm = &sd->members[i];
                IRValue off = new_value(fn);
                emit_inst_w(fn, IR_CONST, off, -1, -1, sm->offset, 8, 1, loc);
                IRValue ptr = emit_bin_w(fn, IR_ADD, base, off, 8, 1, loc);
                if (sm->bit_width > 0) {
                    /* Bitfield member: read-modify-write the storage unit so
                     * that we don't overwrite adjacent bitfields sharing the
                     * same storage unit at the same byte offset. */
                    int uw = type_size(sm->type); /* unit width in bytes */
                    int64_t mask = bitfield_mask64(sm->bit_width);
                    /* Lower the initializer expression to get the value. */
                    IRValue rv = lower_expr(fn, st, e->u.init_list.elements[i]);
                    int rw = get_value_width(fn, rv), ru = get_value_is_unsigned(fn, rv);
                    /* Mask the value to the bitfield width. */
                    IRValue m_mask = new_value(fn);
                    emit_inst_w(fn, IR_CONST, m_mask, -1, -1, mask, uw, 1, loc);
                    IRValue val_masked = new_value(fn);
                    IRValue rv_coerced = coerce(fn, rv, rw, ru, uw, 1, loc);
                    emit_inst_w(fn, IR_BAND, val_masked, rv_coerced, m_mask, 0, uw, 1, loc);
                    /* Shift into position. */
                    IRValue shifted_val;
                    if (sm->bit_offset > 0) {
                        IRValue shift_v = new_value(fn);
                        emit_inst_w(fn, IR_CONST, shift_v, -1, -1, sm->bit_offset, 8, 1, loc);
                        shifted_val = new_value(fn);
                        emit_inst_w(fn, IR_SHL, shifted_val, val_masked, shift_v, 0, uw, 1, loc);
                    } else {
                        shifted_val = val_masked;
                    }
                    /* Read the current unit value. */
                    IRValue unit_old = new_value(fn);
                    emit_inst_w(fn, IR_LOAD_PTR, unit_old, ptr, -1, 0, uw, 1, loc);
                    /* Clear the bitfield's bits: unit &= ~(mask << bit_offset). */
                    IRValue clr_mask = new_value(fn);
                    emit_inst_w(fn, IR_CONST, clr_mask, -1, -1,
                                ~(mask << sm->bit_offset), uw, 1, loc);
                    IRValue unit_cleared = new_value(fn);
                    emit_inst_w(fn, IR_BAND, unit_cleared, unit_old, clr_mask, 0, uw, 1, loc);
                    /* OR in the new value. */
                    IRValue unit_new = new_value(fn);
                    emit_inst_w(fn, IR_BOR, unit_new, unit_cleared, shifted_val, 0, uw, 1, loc);
                    /* Store back. */
                    emit_inst_w(fn, IR_STORE_PTR, -1, ptr, unit_new, 0, uw, 1, loc);
                } else {
                    lower_init_list(fn, st, ptr, &sm->type,
                                    e->u.init_list.elements[i], loc);
                }
            }
            break;
        }
        default:
            lower_init_list(fn, st, base, ty, e->u.init_list.elements[0], loc);
            break;
        }
        return;
    }
    /* A string literal initializing a char array member copies the bytes into
     * the array (`struct { char name[8]; } q = {"xy"}`).  Without this the
     * generic scalar path below would store the literal's *address*, truncated
     * to one byte.  The top-level `char s[] = "hi"` form never reaches here:
     * sema rewrites it into an element list first. */
    if (e->kind == EX_STR && ty->kind == TY_ARRAY && ty->elem_type
        && ty->elem_type->width == 1) {
        int total = ty->length;
        int n = e->u.str.len;
        if (n > total) n = total;
        int eu = ty->elem_type->is_unsigned;
        for (int i = 0; i < total; i++) {
            IRValue off = new_value(fn);
            emit_inst_w(fn, IR_CONST, off, -1, -1, i, 8, 1, loc);
            IRValue ptr = emit_bin_w(fn, IR_ADD, base, off, 8, 1, loc);
            IRValue cv = new_value(fn);
            emit_inst_w(fn, IR_CONST, cv, -1, -1,
                        i < n ? (unsigned char)e->u.str.bytes[i] : 0, 1, eu, loc);
            emit_inst_w(fn, IR_STORE_PTR, -1, ptr, cv, 0, 1, eu, loc);
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

void ir_generate(const TranslationUnit *tu, IRModule *ir, int pin_locals) {
    /* Publish module + reset string counter for lower_expr's use. */
    g_ir_module = ir;
    g_str_counter = 0;
    g_ir_structs = &tu->structs;
    g_ir_tu = tu;
    g_ir_pin_locals = pin_locals;

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
        int is_static = (s->u.decl.storage_class == 1);
        /* Create the global FIRST so pack_init can attach pointer fixups to it
         * when an array/struct member decays to a pointer (e.g. `.regs = ARR`). */
        IRGlobal *g = ir_module_push_global(ir, s->u.decl.name, sz, bytes,
                                            0, is_static, s->loc);
        if (s->u.decl.init) {
            pack_init(ir, &s->u.decl.type, s->u.decl.init, bytes, sz,
                      s->u.decl.name, s->loc, g);
            flush_rodata(ir);
        }
    }

    for (size_t i = 0; i < tu->functions.len; i++) {
        const FunctionDecl *fd = &tu->functions.data[i];

        /* `extern` declarations have no body — no IR is generated for them. */
        if (fd->is_extern) continue;

        IRFunction irfn;
        memset(&irfn, 0, sizeof(irfn));
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
        irfn.ret_reg_n = 0;
        irfn.ret_reg_cls[0] = 0;
        irfn.ret_reg_cls[1] = 0;
        if (irfn.ret_is_struct) {
            SysVRegClass rcls[2];
            irfn.ret_reg_n = sysv_classify_agg(fd->ret_type, rcls);
            for (int i = 0; i < irfn.ret_reg_n; i++)
                irfn.ret_reg_cls[i] = (int)rcls[i];
            /* Struct size for copy/load lives in ret_width. */
            irfn.ret_width = type_size(fd->ret_type);
        }
        irfn.dbg_vars = NULL;
        irfn.num_dbg_vars = 0;
        irfn.cap_dbg_vars = 0;

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
         * alloca/addr/store chains follow afterward.
         *
         * SysV: MEMORY-class struct returns get a hidden sret pointer as
         * param 0.  Register-class struct returns need no hidden param.
         * Struct formals ≤16 bytes expand into 1–2 register PARAMs; larger
         * (MEMORY) formals expand into stack-only eightbyte PARAMs. */
        IRValue param_ebs[64][2];
        int param_nreg[64]; /* >0 reg ebs; 0 MEMORY; -1 scalar */
        IRValue param_mem_vals[256];
        int param_mem_base[64];
        int param_mem_n[64];
        int mem_nvals = 0;
        int next_pidx = 0;
        if (irfn.ret_is_struct && irfn.ret_reg_n == 0) {
            irfn.sret_value = new_value(&irfn);
            emit_inst_w(&irfn, IR_PARAM, irfn.sret_value, -1, -1, next_pidx++,
                        8, 1, fd->loc);
        }
        for (size_t p = 0; p < fd->params.len; p++) {
            Type pty = fd->params.data[p].type;
            SourceLoc ploc = fd->params.data[p].loc;
            param_mem_base[p] = 0;
            param_mem_n[p] = 0;
            if (pty.kind == TY_STRUCT) {
                SysVRegClass cls[2];
                int nreg = sysv_classify_agg(pty, cls);
                if (nreg > 0) {
                    param_nreg[p] = nreg;
                    for (int k = 0; k < nreg; k++) {
                        param_ebs[p][k] = new_value(&irfn);
                        int is_sse = (cls[k] == SYSV_CLS_SSE);
                        emit_inst_w(&irfn, IR_PARAM, param_ebs[p][k], -1, -1,
                                    next_pidx++, 8, 1, ploc);
                        if (is_sse)
                            set_value_float(&irfn, param_ebs[p][k], 1);
                    }
                } else {
                    /* MEMORY: eightbytes arrive on the stack only. */
                    int total = type_size(pty);
                    int nmem = (total + 7) / 8;
                    if (nmem < 1) nmem = 1;
                    if (mem_nvals + nmem > 256) nmem = 256 - mem_nvals;
                    param_nreg[p] = 0;
                    param_mem_base[p] = mem_nvals;
                    param_mem_n[p] = nmem;
                    for (int k = 0; k < nmem; k++) {
                        IRValue v = new_value(&irfn);
                        emit_inst_w(&irfn, IR_PARAM, v, -1, -1, next_pidx++,
                                    8, 1, ploc);
                        irfn.insts.data[irfn.insts.len - 1].force_stack = 1;
                        param_mem_vals[mem_nvals++] = v;
                    }
                }
            } else {
                param_nreg[p] = -1;
                int pw = (pty.kind == TY_PTR) ? 8 : (pty.width ? pty.width : 4);
                int pu = (pty.kind == TY_PTR) ? 1 : pty.is_unsigned;
                param_ebs[p][0] = new_value(&irfn);
                emit_inst_w(&irfn, IR_PARAM, param_ebs[p][0], -1, -1,
                            next_pidx++, pw, pu, ploc);
                if (pty.kind == TY_FLOAT)
                    set_value_float(&irfn, param_ebs[p][0], 1);
            }
        }
        for (size_t p = 0; p < fd->params.len; p++) {
            Type pty = fd->params.data[p].type;
            int pw = pty.kind == TY_PTR ? 8 : (pty.width ? pty.width : 4);
            int pu = pty.kind == TY_PTR ? 1 : pty.is_unsigned;
            const char *pname = fd->params.data[p].name;
            SourceLoc ploc = fd->params.data[p].loc;
            int pinned = is_pinned_in_body(fd, pname, pty);
            if (g_ir_pin_locals)
                pinned = 1;
            /* Float params are always pinned: their SSA value lives in the XMM
             * register file (separate from the GP file the scalar regalloc
             * targets), so mem2reg must not promote them.  long double params
             * are pinned for the same reason: they arrive in a 16-byte stack
             * slot and move through x87 st0, and a promoted copy would be
             * renamed into the GP file and reloaded as an integer. */
            if (pty.kind == TY_FLOAT)
                pinned = 1;
            /* Struct formals are always pinned (live in a stack slot). */
            if (pty.kind == TY_STRUCT)
                pinned = 1;

            IRValue slot;
            if (pty.kind == TY_STRUCT) {
                int total = type_size(pty);
                slot = emit_alloca(&irfn, total, 8, 1, ploc);
                IRValue addr = emit_bin_w(&irfn, IR_ADDR, slot, -1, 8, 1, ploc);
                if (param_nreg[p] > 0) {
                    SysVRegClass cls[2];
                    sysv_classify_agg(pty, cls);
                    store_agg_regs(&irfn, addr, total, param_nreg[p], cls,
                                   param_ebs[p], ploc);
                } else {
                    /* MEMORY: store each incoming stack eightbyte into the
                     * local slot. */
                    SysVRegClass cls[2] = { SYSV_CLS_INTEGER, SYSV_CLS_INTEGER };
                    for (int k = 0; k < param_mem_n[p]; k++) {
                        int off = k * 8;
                        int remain = total - off;
                        if (remain <= 0) break;
                        int w = remain >= 8 ? 8 : remain >= 4 ? 4
                              : remain >= 2 ? 2 : 1;
                        IRValue a = emit_add_const(&irfn, addr, off, ploc);
                        IRValue v = param_mem_vals[param_mem_base[p] + k];
                        if (w < 8) {
                            IRValue t = new_value(&irfn);
                            emit_inst_w(&irfn, IR_TRUNC, t, v, -1, 8, w, 1,
                                        ploc);
                            v = t;
                        }
                        emit_inst_w(&irfn, IR_STORE_PTR, -1, a, v, 0, w, 1,
                                    ploc);
                        (void)cls;
                    }
                }
            } else if (pinned) {
                int total = type_size(pty);
                slot = emit_alloca(&irfn, total, pw, pu, ploc);
                IRValue addr = emit_bin_w(&irfn, IR_ADDR, slot, -1, 8, 1, ploc);
                emit_inst_w(&irfn, IR_STORE_PTR, -1, addr, param_ebs[p][0], 0,
                            pw, pu, ploc);
            } else {
                slot = new_value(&irfn);
                emit_inst_w(&irfn, IR_ALLOCA, slot, -1, -1, 0, pw, pu, ploc);
                emit_inst_w(&irfn, IR_STORE, -1, slot, param_ebs[p][0], 0, pw,
                            pu, ploc);
            }
            irsymtable_push(&st, pname, slot, pinned, pty);
            {
                /* Debug: first IR param index for this formal (sret shifts). */
                int pidx = 0;
                if (irfn.ret_is_struct && irfn.ret_reg_n == 0) pidx = 1;
                for (size_t q = 0; q < p; q++) {
                    if (param_nreg[q] > 0) pidx += param_nreg[q];
                    else if (param_nreg[q] == 0) pidx += param_mem_n[q];
                    else pidx += 1;
                }
                ir_add_dbg_var(&irfn, pname, ploc, IR_DBG_PARAM, pty, slot, pidx);
            }
        }

        /* Pre-pass: assign label ids to every label in this function so
         * forward gotos resolve. */
        LabelMap lm;
        labelmap_init(&lm);
        g_ir_label_map = &lm;
        g_ir_cur_fd = fd;
        for (size_t j = 0; j < fd->body.len; j++)
            assign_label_ids(&irfn, &lm, &fd->body.data[j]);

        for (size_t j = 0; j < fd->body.len; j++) {
            lower_stmt(&irfn, &st, &fd->body.data[j], fd);
        }

        /* If function doesn't end with an IR_RETURN, append a default return. */
        int needs_ret = 1;
        if (irfn.insts.len > 0) {
            IROpcode last_op = irfn.insts.data[irfn.insts.len - 1].op;
            if (last_op == IR_RETURN || last_op == IR_BR)
                needs_ret = 0;
        }
        if (needs_ret) {
            if (fd->ret_type.kind == TY_VOID) {
                emit_inst_w(&irfn, IR_RETURN, -1, -1, -1, 0, 0, 0, fd->loc);
            } else {
                IRValue zero = new_value(&irfn);
                int rw = fd->ret_type.width ? fd->ret_type.width : 4;
                int ru = fd->ret_type.is_unsigned;
                emit_inst_w(&irfn, IR_CONST, zero, -1, -1, 0, rw, ru, fd->loc);
                emit_inst_w(&irfn, IR_RETURN, -1, zero, -1, 0, rw, ru, fd->loc);
            }
        }

        g_ir_label_map = NULL;
        labelmap_free(&lm);

        irsymtable_free(&st);
        ir_func_array_push(&ir->functions, irfn);
    }
}
