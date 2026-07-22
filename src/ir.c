#include "fakecc/ir.h"
#include "fakecc/ast.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>

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

/* Lower an expression to a value id, emitting instructions as needed. */
static IRValue lower_expr(IRFunction *fn, const Expr *e) {
    switch (e->kind) {
    case EX_INT_LIT: {
        IRValue v = new_value(fn);
        IRInst inst;
        inst.op = IR_CONST;
        inst.dst = v;
        inst.a = -1;
        inst.b = -1;
        inst.imm = e->u.int_val;
        inst.loc = e->loc;
        ir_inst_array_push(&fn->insts, inst);
        return v;
    }
    case EX_UNARY:
        switch (e->u.un.op) {
        case UOP_NEG: {
            IRValue x = lower_expr(fn, e->u.un.operand);
            return emit_bin(fn, IR_NEG, x, -1, e->loc);
        }
        case UOP_POS:
            /* no-op */
            return lower_expr(fn, e->u.un.operand);
        }
        break; /* unreachable */
    case EX_BINOP: {
        IRValue l = lower_expr(fn, e->u.bin.l);
        IRValue r = lower_expr(fn, e->u.bin.r);
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
    }
    /* unreachable */
    return -1;
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

        /* Lower the return expression to a value, then emit IR_RETURN. */
        IRValue v = lower_expr(&irfn, fd->body.value);
        IRInst inst;
        inst.op = IR_RETURN;
        inst.dst = -1;
        inst.a = v;
        inst.b = -1;
        inst.imm = 0;
        inst.loc = fd->body.loc;
        ir_inst_array_push(&irfn.insts, inst);

        ir_func_array_push(&ir->functions, irfn);
    }
}
