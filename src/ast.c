#include "fakecc/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Expr constructors & destructor                                       */
/* ------------------------------------------------------------------ */

Expr *expr_new_int(int v, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_INT_LIT;
    e->loc = loc;
    e->u.int_val = v;
    return e;
}

Expr *expr_new_binop(BinOp op, Expr *l, Expr *r, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_BINOP;
    e->loc = loc;
    e->u.bin.op = op;
    e->u.bin.l = l;
    e->u.bin.r = r;
    return e;
}

Expr *expr_new_unary(UnaryOp op, Expr *operand, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_UNARY;
    e->loc = loc;
    e->u.un.op = op;
    e->u.un.operand = operand;
    return e;
}

void expr_free(Expr *e) {
    if (!e) return;
    switch (e->kind) {
    case EX_INT_LIT:
        break;
    case EX_BINOP:
        expr_free(e->u.bin.l);
        expr_free(e->u.bin.r);
        break;
    case EX_UNARY:
        expr_free(e->u.un.operand);
        break;
    }
    free(e);
}

/* ------------------------------------------------------------------ */
/* TranslationUnit lifetime                                            */
/* ------------------------------------------------------------------ */

void tu_init(TranslationUnit *tu) {
    tu->package.name = NULL;
    tu->package.loc.file = NULL;
    tu->package.loc.line = 0;
    tu->package.loc.col = 0;
    tu->functions.data = NULL;
    tu->functions.len = 0;
    tu->functions.cap = 0;
}

void tu_free(TranslationUnit *tu) {
    free(tu->package.name);
    for (size_t i = 0; i < tu->functions.len; i++) {
        free(tu->functions.data[i].name);
        expr_free(tu->functions.data[i].body.value);
    }
    free(tu->functions.data);
}
