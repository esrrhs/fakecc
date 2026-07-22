#include "fakecc/sema.h"
#include "fakecc/common.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* Expression walker — trivial non-null check, preheat for Slice 3     */
/* ------------------------------------------------------------------ */

static void check_expr(const Expr *e) {
    if (!e) return;
    switch (e->kind) {
    case EX_INT_LIT:
        break;
    case EX_BINOP:
        check_expr(e->u.bin.l);
        check_expr(e->u.bin.r);
        break;
    case EX_UNARY:
        check_expr(e->u.un.operand);
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

    /* walk the return expression — just non-null check for now */
    check_expr(fn->body.value);
}
