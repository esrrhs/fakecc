#include "fakecc/sema.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Function table (module-level)                                       */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;   /* borrows from FunctionDecl */
    int arity;
    SourceLoc loc;
} FunSig;

typedef struct {
    FunSig *data;
    size_t len;
    size_t cap;
} FunTable;

static void ftab_init(FunTable *t) { t->data = NULL; t->len = 0; t->cap = 0; }
static void ftab_free(FunTable *t) { free(t->data); t->data = NULL; t->len = 0; t->cap = 0; }

static void ftab_push(FunTable *t, const char *name, int arity, SourceLoc loc) {
    if (t->len >= t->cap) {
        t->cap = t->cap ? t->cap * 2 : 8;
        t->data = realloc(t->data, t->cap * sizeof(FunSig));
        if (!t->data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    }
    t->data[t->len].name = name;
    t->data[t->len].arity = arity;
    t->data[t->len].loc = loc;
    t->len++;
}

static const FunSig *ftab_find(const FunTable *t, const char *name) {
    for (size_t i = 0; i < t->len; i++)
        if (strcmp(t->data[i].name, name) == 0) return &t->data[i];
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Symbol table — supports nested scopes via a scope-mark stack       */
/* ------------------------------------------------------------------ */

typedef struct {
    char *name;      /* xstrdup'd; NULL indicates a scope-boundary marker */
    SourceLoc loc;
} Sym;

typedef struct {
    Sym *data;
    size_t len;
    size_t cap;
} SymTable;

static void symtable_init(SymTable *st) { st->data = NULL; st->len = 0; st->cap = 0; }

static void symtable_free(SymTable *st) {
    for (size_t i = 0; i < st->len; i++) free(st->data[i].name);
    free(st->data);
    st->data = NULL; st->len = 0; st->cap = 0;
}

static void symtable_push(SymTable *st, const char *name, SourceLoc loc) {
    if (st->len >= st->cap) {
        st->cap = st->cap ? st->cap * 2 : 8;
        st->data = realloc(st->data, st->cap * sizeof(Sym));
        if (!st->data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    }
    st->data[st->len].name = name ? xstrdup(name) : NULL;
    st->data[st->len].loc = loc;
    st->len++;
}

static size_t symtable_enter_scope(SymTable *st) { return st->len; }

static void symtable_leave_scope(SymTable *st, size_t mark) {
    while (st->len > mark) {
        st->len--;
        free(st->data[st->len].name);
    }
}

static int symtable_has(const SymTable *st, const char *name) {
    for (size_t i = 0; i < st->len; i++)
        if (st->data[i].name && strcmp(st->data[i].name, name) == 0) return 1;
    return 0;
}

static int symtable_has_since(const SymTable *st, const char *name, size_t mark) {
    for (size_t i = mark; i < st->len; i++)
        if (st->data[i].name && strcmp(st->data[i].name, name) == 0) return 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Expression checker                                                  */
/* ------------------------------------------------------------------ */

static void check_expr(const Expr *e, const SymTable *st, const FunTable *ft) {
    if (!e) return;
    switch (e->kind) {
    case EX_INT_LIT:
        break;
    case EX_BINOP:
        check_expr(e->u.bin.l, st, ft);
        check_expr(e->u.bin.r, st, ft);
        break;
    case EX_UNARY:
        check_expr(e->u.un.operand, st, ft);
        break;
    case EX_VAR:
        if (!symtable_has(st, e->u.var.name)) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "use of undeclared variable '%s'", e->u.var.name);
        }
        break;
    case EX_ASSIGN:
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
        check_expr(e->u.assign.rvalue, st, ft);
        break;
    case EX_CALL: {
        const FunSig *sig = ftab_find(ft, e->u.call.callee);
        if (!sig) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "call to undeclared function '%s'", e->u.call.callee);
        }
        if ((int)e->u.call.args.len != sig->arity) {
            die_at(e->loc.file, e->loc.line, e->loc.col,
                   "function '%s' takes %d argument%s but %zu given",
                   e->u.call.callee, sig->arity,
                   sig->arity == 1 ? "" : "s", e->u.call.args.len);
        }
        for (size_t i = 0; i < e->u.call.args.len; i++)
            check_expr(e->u.call.args.data[i], st, ft);
        break;
    }
    }
}

/* ------------------------------------------------------------------ */
/* Statement checker                                                   */
/* ------------------------------------------------------------------ */

static void check_stmt(const Stmt *s, SymTable *st, const FunTable *ft,
                       size_t scope_mark, int *has_return);

static void check_stmt_list(const StmtArray *body, SymTable *st,
                            const FunTable *ft, int *has_return) {
    size_t mark = symtable_enter_scope(st);
    for (size_t i = 0; i < body->len; i++)
        check_stmt(&body->data[i], st, ft, mark, has_return);
    symtable_leave_scope(st, mark);
}

static void check_stmt(const Stmt *s, SymTable *st, const FunTable *ft,
                       size_t scope_mark, int *has_return) {
    switch (s->kind) {
    case ST_DECL:
        if (symtable_has_since(st, s->u.decl.name, scope_mark)) {
            die_at(s->loc.file, s->loc.line, s->loc.col,
                   "redeclaration of '%s'", s->u.decl.name);
        }
        symtable_push(st, s->u.decl.name, s->loc);
        if (s->u.decl.init) check_expr(s->u.decl.init, st, ft);
        break;
    case ST_EXPR:
        check_expr(s->u.expr, st, ft);
        break;
    case ST_RETURN:
        check_expr(s->u.value, st, ft);
        *has_return = 1;
        break;
    case ST_IF:
        check_expr(s->u.if_s.cond, st, ft);
        check_stmt(s->u.if_s.then_s, st, ft, scope_mark, has_return);
        if (s->u.if_s.else_s) check_stmt(s->u.if_s.else_s, st, ft, scope_mark, has_return);
        break;
    case ST_WHILE:
        check_expr(s->u.while_s.cond, st, ft);
        check_stmt(s->u.while_s.body, st, ft, scope_mark, has_return);
        break;
    case ST_BLOCK:
        check_stmt_list(&s->u.block, st, ft, has_return);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Whole-TU check                                                       */
/* ------------------------------------------------------------------ */

void sema_check(const TranslationUnit *tu) {
    if (tu->package.name == NULL || strcmp(tu->package.name, "main") != 0) {
        die_at(tu->package.loc.file, tu->package.loc.line, tu->package.loc.col,
               "package must be 'main'");
    }

    /* Build function table; enforce no duplicates. */
    FunTable ft;
    ftab_init(&ft);
    int has_main = 0;
    for (size_t i = 0; i < tu->functions.len; i++) {
        const FunctionDecl *fn = &tu->functions.data[i];
        if (ftab_find(&ft, fn->name)) {
            die_at(fn->loc.file, fn->loc.line, fn->loc.col,
                   "redefinition of function '%s'", fn->name);
        }
        ftab_push(&ft, fn->name, (int)fn->params.len, fn->loc);
        if (strcmp(fn->name, "main") == 0) has_main = 1;
    }
    if (!has_main) {
        die_at(tu->package.loc.file, tu->package.loc.line, tu->package.loc.col,
               "no 'main' function defined");
    }

    /* Check each function body. */
    for (size_t i = 0; i < tu->functions.len; i++) {
        const FunctionDecl *fn = &tu->functions.data[i];

        /* main must be nullary (Slice 6 restriction). */
        if (strcmp(fn->name, "main") == 0 && fn->params.len != 0) {
            die_at(fn->loc.file, fn->loc.line, fn->loc.col,
                   "'main' must take no parameters");
        }

        SymTable st;
        symtable_init(&st);

        /* Push params as innermost scope. */
        size_t mark = symtable_enter_scope(&st);
        for (size_t j = 0; j < fn->params.len; j++) {
            /* Detect duplicate parameter names within one signature. */
            if (symtable_has_since(&st, fn->params.data[j].name, mark)) {
                die_at(fn->params.data[j].loc.file,
                       fn->params.data[j].loc.line,
                       fn->params.data[j].loc.col,
                       "duplicate parameter name '%s'",
                       fn->params.data[j].name);
            }
            symtable_push(&st, fn->params.data[j].name, fn->params.data[j].loc);
        }

        int has_return = 0;
        check_stmt_list(&fn->body, &st, &ft, &has_return);
        symtable_leave_scope(&st, mark);
        symtable_free(&st);

        if (!has_return) {
            die_at(fn->loc.file, fn->loc.line, fn->loc.col,
                   "function '%s' must have a return statement", fn->name);
        }
    }

    ftab_free(&ft);
}
