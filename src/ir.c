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
        free(m->functions.data[i].insts.data);
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
    }
    /* unreachable */
    return -1;
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
        /* evaluate for side effects (e.g. assignment store), discard result */
        lower_expr(fn, st, s->u.expr);
        break;
    case ST_RETURN: {
        IRValue v = lower_expr(fn, st, s->u.value);
        emit_inst(fn, IR_RETURN, -1, v, -1, 0, s->loc);
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

        IRSymTable st;
        irsymtable_init(&st);

        /* Lower each statement in the body in order. */
        for (size_t j = 0; j < fd->body.len; j++) {
            lower_stmt(&irfn, &st, &fd->body.data[j]);
        }

        irsymtable_free(&st);
        ir_func_array_push(&ir->functions, irfn);
    }
}
