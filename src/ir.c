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

/* Push a binary/2-source instruction with the given opcode. */
static IRValue emit_bin(IRFunction *fn, IROpcode op, IRValue a, IRValue b, SourceLoc loc) {
    IRValue v = new_value(fn);
    IRInst inst;
    inst.op = op;
    inst.dst = v;
    inst.a = a;
    inst.b = b;
    inst.imm = 0;
    inst.loc = loc;
    inst.call_name = NULL;
    inst.call_nargs = 0;
    ir_inst_array_push(&fn->insts, inst);
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

/* Push an instruction with the given fields. */
static void emit_inst(IRFunction *fn, IROpcode op, IRValue dst, IRValue a, IRValue b,
                      int imm, SourceLoc loc) {
    IRInst inst;
    inst.op = op;
    inst.dst = dst;
    inst.a = a;
    inst.b = b;
    inst.imm = imm;
    inst.loc = loc;
    inst.call_name = NULL;
    inst.call_nargs = 0;
    ir_inst_array_push(&fn->insts, inst);
}

/* Lower an expression to a value id, emitting instructions as needed. */
static IRValue lower_expr(IRFunction *fn, IRSymTable *st, const Expr *e) {
    switch (e->kind) {
    case EX_INT_LIT: {
        IRValue v = new_value(fn);
        emit_inst(fn, IR_CONST, v, -1, -1, e->u.int_val, e->loc);
        return v;
    }
    case EX_UNARY:
        switch (e->u.un.op) {
        case UOP_NEG: {
            IRValue x = lower_expr(fn, st, e->u.un.operand);
            return emit_bin(fn, IR_NEG, x, -1, e->loc);
        }
        case UOP_POS:
            /* no-op */
            return lower_expr(fn, st, e->u.un.operand);
        }
        break; /* unreachable */
    case EX_BINOP: {
        IRValue l = lower_expr(fn, st, e->u.bin.l);
        IRValue r = lower_expr(fn, st, e->u.bin.r);
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
        default: op = IR_ADD; break; /* unreachable */
        }
        return emit_bin(fn, op, l, r, e->loc);
    }
    case EX_VAR: {
        IRValue slot = irsymtable_get(st, e->u.var.name);
        IRValue v = new_value(fn);
        emit_inst(fn, IR_LOAD, v, slot, -1, 0, e->loc);
        return v;
    }
    case EX_ASSIGN: {
        /* lvalue is EX_VAR (sema guarantees); store rvalue into its slot. */
        IRValue slot = irsymtable_get(st, e->u.assign.lvalue->u.var.name);
        IRValue rv = lower_expr(fn, st, e->u.assign.rvalue);
        emit_inst(fn, IR_STORE, -1, slot, rv, 0, e->loc);
        return rv;   /* assignment yields the assigned value, no extra load */
    }
    case EX_CALL: {
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
        ir_inst_array_push(&fn->insts, inst);
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
        emit_inst(fn, IR_ALLOCA, v, -1, -1, 0, s->loc);
        irsymtable_push(st, s->u.decl.name, v);
        if (s->u.decl.init) {
            IRValue rv = lower_expr(fn, st, s->u.decl.init);
            emit_inst(fn, IR_STORE, -1, v, rv, 0, s->loc);
        }
        break;
    }
    case ST_EXPR:
        lower_expr(fn, st, s->u.expr);
        break;
    case ST_RETURN: {
        IRValue v = lower_expr(fn, st, s->u.value);
        emit_inst(fn, IR_RETURN, -1, v, -1, 0, s->loc);
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
        st->len = mark;  /* pop block-local names; slots keep their IR ids */
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

        IRSymTable st;
        irsymtable_init(&st);

        /* Materialize incoming parameters. Each parameter gets:
         *   1. An IR_PARAM defining its incoming SSA value (imm = position).
         *   2. An IR_ALLOCA + IR_STORE so the body can treat it as a normal
         *      writable local; mem2reg will fold the store when possible. */
        for (size_t p = 0; p < fd->params.len; p++) {
            IRValue param_v = new_value(&irfn);
            emit_inst(&irfn, IR_PARAM, param_v, -1, -1, (int)p, fd->params.data[p].loc);

            IRValue slot = new_value(&irfn);
            emit_inst(&irfn, IR_ALLOCA, slot, -1, -1, 0, fd->params.data[p].loc);
            emit_inst(&irfn, IR_STORE, -1, slot, param_v, 0, fd->params.data[p].loc);
            irsymtable_push(&st, fd->params.data[p].name, slot);
        }

        /* Lower each statement in the body in order. */
        for (size_t j = 0; j < fd->body.len; j++) {
            lower_stmt(&irfn, &st, &fd->body.data[j]);
        }

        irsymtable_free(&st);
        ir_func_array_push(&ir->functions, irfn);
    }
}
