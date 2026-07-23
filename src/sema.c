#include "fakecc/sema.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Symbol table — flat, function-level scope (Slice 3)                 */
/* ------------------------------------------------------------------ */

typedef struct {
    char *name;      /* xstrdup'd */
    SourceLoc loc;   /* declaration site, for error reporting */
} Sym;

typedef struct {
    Sym *data;
    size_t len;
    size_t cap;
} SymTable;

static void symtable_init(SymTable *st) {
    st->data = NULL;
    st->len = 0;
    st->cap = 0;
}

static void symtable_free(SymTable *st) {
    for (size_t i = 0; i < st->len; i++) {
        free(st->data[i].name);
    }
    free(st->data);
    st->data = NULL;
    st->len = 0;
    st->cap = 0;
}

static void symtable_push(SymTable *st, const char *name, SourceLoc loc) {
    if (st->len >= st->cap) {
        size_t new_cap = st->cap ? st->cap * 2 : 8;
        st->data = realloc(st->data, new_cap * sizeof(Sym));
        if (!st->data) {
            fprintf(stderr, "fakecc: out of memory\n");
            exit(1);
        }
        st->cap = new_cap;
    }
    st->data[st->len].name = xstrdup(name);
    st->data[st->len].loc = loc;
    st->len++;
}

/* Linear lookup; variables are few. Returns 1 if found. */
static int symtable_has(const SymTable *st, const char *name) {
    for (size_t i = 0; i < st->len; i++) {
        if (strcmp(st->data[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Expression checker — uses the symbol table                         */
/* ------------------------------------------------------------------ */

static void check_expr(const Expr *e, const SymTable *st) {
    if (!e) return;
    switch (e->kind) {
    case EX_INT_LIT:
        break;
    case EX_BINOP:
        check_expr(e->u.bin.l, st);
        check_expr(e->u.bin.r, st);
        break;
    case EX_UNARY:
        check_expr(e->u.un.operand, st);
        break;
    case EX_VAR:
        if (!symtable_has(st, e->u.var.name)) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "use of undeclared variable '%s'", e->u.var.name);
        }
        break;
    case EX_ASSIGN:
        /* Only variables are assignable in Slice 3. */
        if (e->u.assign.lvalue->kind != EX_VAR) {
            die_at(e->u.assign.lvalue->loc.file,
                   e->u.assign.lvalue->loc.line,
                   e->u.assign.lvalue->loc.col,
                   "expression is not assignable");
        }
        if (!symtable_has(st, e->u.assign.lvalue->u.var.name)) {
            die_at(e->u.assign.lvalue->loc.file,
                   e->u.assign.lvalue->loc.line,
                   e->u.assign.lvalue->loc.col,
                   "use of undeclared variable '%s'",
                   e->u.assign.lvalue->u.var.name);
        }
        check_expr(e->u.assign.rvalue, st);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Semantic checks                                                      */
/* ------------------------------------------------------------------ */

void sema_check(const TranslationUnit *tu) {
    /* package must be "main" */
    if (tu->package.name == NULL || strcmp(tu->package.name, "main") != 0) {
        die_at(tu->package.loc.file, tu->package.loc.line, tu->package.loc.col,
               "package must be 'main'");
    }

    /* must have exactly one function */
    if (tu->functions.len != 1) {
        die_at(tu->package.loc.file, tu->package.loc.line, tu->package.loc.col,
               "expected exactly one function");
    }

    /* function must be named "main" */
    const FunctionDecl *fn = &tu->functions.data[0];
    if (strcmp(fn->name, "main") != 0) {
        die_at(fn->loc.file, fn->loc.line, fn->loc.col,
               "function must be 'main'");
    }

    /* walk the statement list with a flat symbol table */
    SymTable st;
    symtable_init(&st);

    int has_return = 0;
    for (size_t i = 0; i < fn->body.len; i++) {
        const Stmt *s = &fn->body.data[i];
        switch (s->kind) {
        case ST_DECL:
            if (symtable_has(&st, s->u.decl.name)) {
                die_at(s->loc.file, s->loc.line, s->loc.col,
                       "redeclaration of '%s'", s->u.decl.name);
            }
            symtable_push(&st, s->u.decl.name, s->loc);
            if (s->u.decl.init) {
                check_expr(s->u.decl.init, &st);
            }
            break;
        case ST_EXPR:
            check_expr(s->u.expr, &st);
            break;
        case ST_RETURN:
            check_expr(s->u.value, &st);
            has_return = 1;
            break;
        }
    }

    symtable_free(&st);

    /* function must have at least one return statement */
    if (!has_return) {
        die_at(fn->loc.file, fn->loc.line, fn->loc.col,
               "function must have a return statement");
    }
}
