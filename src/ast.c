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

Expr *expr_new_var(const char *name, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_VAR;
    e->loc = loc;
    e->u.var.name = xstrdup(name);
    return e;
}

Expr *expr_new_assign(Expr *lvalue, Expr *rvalue, SourceLoc loc) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    e->kind = EX_ASSIGN;
    e->loc = loc;
    e->u.assign.lvalue = lvalue;
    e->u.assign.rvalue = rvalue;
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
    case EX_VAR:
        free(e->u.var.name);
        break;
    case EX_ASSIGN:
        expr_free(e->u.assign.lvalue);
        expr_free(e->u.assign.rvalue);
        break;
    }
    free(e);
}

/* ------------------------------------------------------------------ */
/* Stmt lifetime                                                       */
/* ------------------------------------------------------------------ */

void stmt_free(Stmt *s) {
    if (!s) return;
    switch (s->kind) {
    case ST_DECL:
        free(s->u.decl.name);
        expr_free(s->u.decl.init);
        break;
    case ST_EXPR:
        expr_free(s->u.expr);
        break;
    case ST_RETURN:
        expr_free(s->u.value);
        break;
    }
}

void stmt_array_init(StmtArray *a) {
    a->data = NULL;
    a->len = 0;
    a->cap = 0;
}

void stmt_array_push(StmtArray *a, Stmt s) {
    if (a->len >= a->cap) {
        size_t new_cap = a->cap ? a->cap * 2 : 8;
        a->data = realloc(a->data, new_cap * sizeof(Stmt));
        if (!a->data) {
            fprintf(stderr, "fakecc: out of memory\n");
            exit(1);
        }
        a->cap = new_cap;
    }
    a->data[a->len++] = s;
}

void stmt_array_free(StmtArray *a) {
    for (size_t i = 0; i < a->len; i++) {
        stmt_free(&a->data[i]);
    }
    free(a->data);
    a->data = NULL;
    a->len = 0;
    a->cap = 0;
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
        stmt_array_free(&tu->functions.data[i].body);
    }
    free(tu->functions.data);
}
