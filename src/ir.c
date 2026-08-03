#include "fakecc/ir.h"
#include "fakecc/ast.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* IRModule lifetime                                                   */
/* ------------------------------------------------------------------ */

void ir_module_init(IRModule *m) {
    m->functions.data = NULL;
    m->functions.len = 0;
    m->functions.cap = 0;
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
    m->functions.data = NULL;
    m->functions.len = 0;
    m->functions.cap = 0;
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

/* IR symbol table: variable name → slot (an IRValue, i.e. a stack slot). */
typedef struct {
    const char *name;
    IRValue slot;
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

static void irsymtable_push(IRSymTable *st, const char *name, IRValue slot) {
    if (st->len >= st->cap) {
        size_t new_cap = st->cap ? st->cap * 2 : 8;
        st->data = realloc(st->data, new_cap * sizeof(IRSlot));
        if (!st->data) {
            fprintf(stderr, "fakecc: out of memory\n");
            exit(1);
        }
        st->cap = new_cap;
    }
    st->data[st->len].name = name;   /* borrows the AST's name pointer */
    st->data[st->len].slot = slot;
    st->len++;
}

/* Look up a variable's slot. Sema guarantees the name exists. */
static IRValue irsymtable_get(const IRSymTable *st, const char *name) {
    for (size_t i = 0; i < st->len; i++) {
        if (strcmp(st->data[i].name, name) == 0) {
            return st->data[i].slot;
        }
    }
    return -1; /* unreachable: sema ensures declared */
}

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
    case EX_UNARY:
        switch (e->u.un.op) {
        case UOP_NEG: {
            IRValue x = lower_expr(fn, st, e->u.un.operand);
            int sw = get_value_width(fn, x);
            int su = get_value_is_unsigned(fn, x);
            IRValue px = coerce(fn, x, sw, su, e->type.width, e->type.is_unsigned, e->loc);
            return emit_bin_w(fn, IR_NEG, px, -1,
                              e->type.width, e->type.is_unsigned, e->loc);
        }
        case UOP_POS:
            /* no-op */
            return lower_expr(fn, st, e->u.un.operand);
        }
        break; /* unreachable */
    case EX_BINOP: {
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
            op_w = e->type.width;
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
        IRValue slot = irsymtable_get(st, e->u.var.name);
        IRValue v = new_value(fn);
        emit_inst_w(fn, IR_LOAD, v, slot, -1, 0,
                    e->type.width, e->type.is_unsigned, e->loc);
        return v;
    }
    case EX_ASSIGN: {
        IRValue slot = irsymtable_get(st, e->u.assign.lvalue->u.var.name);
        IRValue rv = lower_expr(fn, st, e->u.assign.rvalue);
        int rw = get_value_width(fn, rv), ru = get_value_is_unsigned(fn, rv);
        int lw = e->u.assign.lvalue->type.width;
        int lu = e->u.assign.lvalue->type.is_unsigned;
        IRValue coerced = coerce(fn, rv, rw, ru, lw, lu, e->loc);
        emit_inst_w(fn, IR_STORE, -1, slot, coerced, 0, lw, lu, e->loc);
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
static void lower_stmt(IRFunction *fn, IRSymTable *st, const Stmt *s) {
    switch (s->kind) {
    case ST_DECL: {
        IRValue v = new_value(fn);
        int dw = s->u.decl.type.width, du = s->u.decl.type.is_unsigned;
        emit_inst_w(fn, IR_ALLOCA, v, -1, -1, 0, dw, du, s->loc);
        irsymtable_push(st, s->u.decl.name, v);
        if (s->u.decl.init) {
            IRValue rv = lower_expr(fn, st, s->u.decl.init);
            int rw = get_value_width(fn, rv), ru = get_value_is_unsigned(fn, rv);
            IRValue coerced = coerce(fn, rv, rw, ru, dw, du, s->loc);
            emit_inst_w(fn, IR_STORE, -1, v, coerced, 0, dw, du, s->loc);
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
        lower_stmt(fn, st, s->u.if_s.then_s);
        if (s->u.if_s.else_s) {
            emit_br(fn, L_end, s->loc);
            emit_label(fn, L_else, s->loc);
            lower_stmt(fn, st, s->u.if_s.else_s);
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
        lower_stmt(fn, st, s->u.while_s.body);
        emit_br(fn, L_head, s->loc);
        emit_label(fn, L_exit, s->loc);
        break;
    }
    case ST_BLOCK: {
        size_t mark = st->len;
        for (size_t i = 0; i < s->u.block.len; i++) {
            lower_stmt(fn, st, &s->u.block.data[i]);
        }
        st->len = mark;
        break;
    }
    }
}

void ir_generate(const TranslationUnit *tu, IRModule *ir) {
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

        for (size_t p = 0; p < fd->params.len; p++) {
            int pw = fd->params.data[p].type.width;
            int pu = fd->params.data[p].type.is_unsigned;
            IRValue param_v = new_value(&irfn);
            emit_inst_w(&irfn, IR_PARAM, param_v, -1, -1, (int)p, pw, pu,
                        fd->params.data[p].loc);

            IRValue slot = new_value(&irfn);
            emit_inst_w(&irfn, IR_ALLOCA, slot, -1, -1, 0, pw, pu,
                        fd->params.data[p].loc);
            emit_inst_w(&irfn, IR_STORE, -1, slot, param_v, 0, pw, pu,
                        fd->params.data[p].loc);
            irsymtable_push(&st, fd->params.data[p].name, slot);
        }

        for (size_t j = 0; j < fd->body.len; j++) {
            lower_stmt(&irfn, &st, &fd->body.data[j]);
        }

        irsymtable_free(&st);
        ir_func_array_push(&ir->functions, irfn);
    }
}
