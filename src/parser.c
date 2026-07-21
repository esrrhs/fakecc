#include "fakecc/parser.h"
#include "fakecc/common.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Parser state                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    const TokenArray *tokens;
    size_t pos;
} Parser;

static const Token *peek(const Parser *p) {
    return &p->tokens->data[p->pos];
}

static const Token *advance(Parser *p) {
    return &p->tokens->data[p->pos++];
}

static void expect_kind(Parser *p, TokenKind kind, const char *msg) {
    const Token *t = peek(p);
    if (t->kind != kind) {
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "expected %s but got '%s'", msg, t->text);
    }
    advance(p);
}

/* ------------------------------------------------------------------ */
/* Grammar                                                             */
/* ------------------------------------------------------------------ */

/*
 * translation-unit  = package-decl function-decl EOF
 * package-decl      = "package" IDENT ";"
 * function-decl     = "int" IDENT "(" ")" "{" return-stmt "}"
 * return-stmt       = "return" INT_LITERAL ";"
 */

static ReturnStmt parse_return_stmt(Parser *p) {
    const Token *kw = peek(p);
    expect_kind(p, TK_KW_RETURN, "'return'");

    const Token *val = peek(p);
    if (val->kind != TK_INT_LITERAL) {
        die_at(val->loc.file, val->loc.line, val->loc.col,
               "expected integer literal but got '%s'", val->text);
    }
    advance(p);

    expect_kind(p, TK_SEMICOLON, "';'");

    ReturnStmt rs;
    rs.value = atoi(val->text);
    rs.loc = kw->loc;
    return rs;
}

static FunctionDecl parse_function_decl(Parser *p) {
    const Token *int_kw = peek(p);
    expect_kind(p, TK_KW_INT, "'int'");

    const Token *name = peek(p);
    if (name->kind != TK_IDENT) {
        die_at(name->loc.file, name->loc.line, name->loc.col,
               "expected function name but got '%s'", name->text);
    }
    advance(p);

    expect_kind(p, TK_LPAREN, "'('");
    expect_kind(p, TK_RPAREN, "')'");
    expect_kind(p, TK_LBRACE, "'{'");

    ReturnStmt body = parse_return_stmt(p);

    expect_kind(p, TK_RBRACE, "'}'");

    FunctionDecl fn;
    fn.name = xstrdup(name->text);
    fn.body = body;
    fn.loc = int_kw->loc;
    return fn;
}

static PackageDecl parse_package_decl(Parser *p) {
    const Token *kw = peek(p);
    expect_kind(p, TK_KW_PACKAGE, "'package'");

    const Token *ident = peek(p);
    if (ident->kind != TK_IDENT) {
        die_at(ident->loc.file, ident->loc.line, ident->loc.col,
               "expected package name but got '%s'", ident->text);
    }
    advance(p);

    expect_kind(p, TK_SEMICOLON, "';'");

    PackageDecl pd;
    pd.name = xstrdup(ident->text);
    pd.loc = kw->loc;
    return pd;
}

void parse(const TokenArray *tokens, TranslationUnit *tu) {
    Parser p;
    p.tokens = tokens;
    p.pos = 0;

    /* must start with package declaration */
    if (peek(&p)->kind != TK_KW_PACKAGE) {
        const Token *t = peek(&p);
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "expected 'package' declaration at start of file");
    }

    tu->package = parse_package_decl(&p);

    /* reject import in Slice 1 */
    if (peek(&p)->kind == TK_KW_IMPORT) {
        const Token *t = peek(&p);
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "'import' is not supported in Slice 1");
    }

    FunctionDecl fn = parse_function_decl(&p);

    /* grow function array */
    if (tu->functions.len >= tu->functions.cap) {
        size_t new_cap = tu->functions.cap ? tu->functions.cap * 2 : 4;
        tu->functions.data = realloc(tu->functions.data, new_cap * sizeof(FunctionDecl));
        tu->functions.cap = new_cap;
    }
    tu->functions.data[tu->functions.len++] = fn;

    /* expect EOF */
    const Token *eof = peek(&p);
    if (eof->kind != TK_EOF) {
        die_at(eof->loc.file, eof->loc.line, eof->loc.col,
               "expected end of file but got '%s'", eof->text);
    }
}
