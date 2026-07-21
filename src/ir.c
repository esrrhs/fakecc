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

void ir_generate(const TranslationUnit *tu, IRModule *ir) {
    for (size_t i = 0; i < tu->functions.len; i++) {
        const FunctionDecl *fd = &tu->functions.data[i];

        IRFunction irfn;
        irfn.name = xstrdup(fd->name);
        irfn.loc = fd->loc;
        irfn.insts.data = NULL;
        irfn.insts.len = 0;
        irfn.insts.cap = 0;

        /* Translate ReturnStmt → IR_RETURN instruction */
        IRInst inst;
        inst.op = IR_RETURN;
        inst.value = fd->body.value;
        inst.loc = fd->body.loc;
        ir_inst_array_push(&irfn.insts, inst);

        ir_func_array_push(&ir->functions, irfn);
    }
}
