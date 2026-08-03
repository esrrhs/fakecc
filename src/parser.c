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
    TranslationUnit *tu;   /* backing TU — parser writes structs directly */
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

/* Recognize an integer type at the current position.
 *   [signed|unsigned] (char|short|int|long)
 * Also accepts bare "signed" / "unsigned" (= int).
 * Returns Type; emits a diagnostic if no type keyword is present. */
static int is_type_start(TokenKind k) {
    return k == TK_KW_INT || k == TK_KW_CHAR || k == TK_KW_SHORT
        || k == TK_KW_LONG || k == TK_KW_SIGNED || k == TK_KW_UNSIGNED
        || k == TK_KW_STRUCT;
}

static Type parse_type(Parser *p) {
    /* struct Name — must be already defined by a `struct` definition earlier
     * OR later; we defer size lookup to sema/IR-gen time by consulting the
     * registry now.  Since ordering matters for parsing (need to know the
     * size for arrays of struct, etc.), we require struct defs precede
     * uses — enforce by looking up in the registry now. */
    if (peek(p)->kind == TK_KW_STRUCT) {
        advance(p);
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected struct tag but got '%s'", tag->text);
        }
        advance(p);
        StructDef *sd = struct_registry_find(&p->tu->structs, tag->text);
        int size = sd ? sd->size : 0;
        if (!sd) {
            /* forward declaration — legal only when we're followed by `*`
             * (pointer to incomplete type).  Otherwise error. */
            /* Note: sema will re-check size once all types settle. */
        }
        Type t = type_make_struct(tag->text, size);
        while (peek(p)->kind == TK_STAR) {
            advance(p);
            Type w = type_make_ptr(t);
            type_free(&t);
            t = w;
        }
        return t;
    }
    int is_unsigned = 0;
    int saw_sign = 0;
    int width = -1;

    TokenKind k = peek(p)->kind;
    if (k == TK_KW_SIGNED)    { is_unsigned = 0; saw_sign = 1; advance(p); }
    else if (k == TK_KW_UNSIGNED) { is_unsigned = 1; saw_sign = 1; advance(p); }

    k = peek(p)->kind;
    switch (k) {
    case TK_KW_CHAR:  advance(p); width = 1; break;
    case TK_KW_SHORT: advance(p); width = 2; break;
    case TK_KW_INT:   advance(p); width = 4; break;
    case TK_KW_LONG:  advance(p); width = 8; break;
    default:
        if (saw_sign) { width = 4; break; }
        {
            const Token *t = peek(p);
            die_at(t->loc.file, t->loc.line, t->loc.col,
                   "expected type but got '%s'", t->text);
        }
    }
    Type t = type_make_int(width, is_unsigned);
    /* Postfix: zero-or-more `*` → wrap in TY_PTR. */
    while (peek(p)->kind == TK_STAR) {
        advance(p);
        Type wrapped = type_make_ptr(t);
        type_free(&t);
        t = wrapped;
    }
    return t;
}

/* ------------------------------------------------------------------ */
/* Grammar                                                             */
/* ------------------------------------------------------------------ */

/*
 * translation-unit  = package-decl function-decl EOF
 * package-decl      = "package" IDENT ";"
 * function-decl     = "int" IDENT "(" ")" "{" stmt-list "}"
 * stmt-list         = { stmt }
 * stmt              = decl-stmt
 *                   | return-stmt
 *                   | expr-stmt
 * decl-stmt         = "int" IDENT [ "=" expr ] ";"
 * return-stmt       = "return" expr ";"
 * expr-stmt         = expr ";"
 *
 * expr              = assign-expr
 * assign-expr       = add-expr [ "=" assign-expr ]        (* right-assoc *)
 * add-expr          = mul-expr { ("+" | "-") mul-expr }
 * mul-expr          = unary-expr { ("*" | "/" | "%") unary-expr }
 * unary-expr        = ("+" | "-") unary-expr
 *                   | primary-expr
 * primary-expr      = INT_LITERAL
 *                   | IDENT
 *                   | "(" expr ")"
 */

/* ---- expression parsing (forward declarations) ---- */

static Expr *parse_expr(Parser *p);
static Expr *parse_assign(Parser *p);
static Expr *parse_ternary(Parser *p);
static Expr *parse_or(Parser *p);
static Expr *parse_and(Parser *p);
static Expr *parse_equality(Parser *p);
static Expr *parse_relational(Parser *p);
static Expr *parse_add(Parser *p);
static Expr *parse_mul(Parser *p);
static Expr *parse_unary(Parser *p);
static Expr *parse_postfix(Parser *p, Expr *lhs);
static Expr *parse_primary(Parser *p);

static Expr *parse_expr(Parser *p) {
    return parse_assign(p);
}

/* assign-expr = ternary-expr [ "=" assign-expr ]  -- right associative */
static Expr *parse_assign(Parser *p) {
    Expr *lhs = parse_ternary(p);
    if (peek(p)->kind == TK_ASSIGN) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_assign(p);   /* recursive → right associative */
        return expr_new_assign(lhs, rhs, loc);
    }
    return lhs;
}

/* ternary-expr = or-expr [ "?" expr ":" ternary-expr ]  -- right associative.
 * The middle operand is a full expr (allows e.g. `c ? a = b : d`);
 * the else branch is ternary-expr so right-associativity chains naturally. */
static Expr *parse_ternary(Parser *p) {
    Expr *cond = parse_or(p);
    if (peek(p)->kind != TK_QUESTION) return cond;
    SourceLoc loc = peek(p)->loc;
    advance(p);  /* consume '?' */
    Expr *then = parse_expr(p);
    expect_kind(p, TK_COLON, "':'");
    Expr *else_ = parse_ternary(p);
    return expr_new_ternary(cond, then, else_, loc);
}

/* or-expr = and-expr { "||" and-expr }  -- left associative, lower than && */
static Expr *parse_or(Parser *p) {
    Expr *lhs = parse_and(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_OROR) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_and(p);
        lhs = expr_new_binop(BOP_OR, lhs, rhs, loc);
    }
    return lhs;
}

/* and-expr = equality-expr { "&&" equality-expr }  -- left associative */
static Expr *parse_and(Parser *p) {
    Expr *lhs = parse_equality(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_ANDAND) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_equality(p);
        lhs = expr_new_binop(BOP_AND, lhs, rhs, loc);
    }
    return lhs;
}

/* equality-expr = relational-expr { ("==" | "!=") relational-expr } */
static Expr *parse_equality(Parser *p) {
    Expr *lhs = parse_relational(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_EQ && k != TK_NE) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_relational(p);
        BinOp op = (k == TK_EQ) ? BOP_EQ : BOP_NE;
        lhs = expr_new_binop(op, lhs, rhs, loc);
    }
    return lhs;
}

/* relational-expr = add-expr { ("<" | "<=" | ">" | ">=") add-expr } */
static Expr *parse_relational(Parser *p) {
    Expr *lhs = parse_add(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_LT && k != TK_LE && k != TK_GT && k != TK_GE) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_add(p);
        BinOp op;
        switch (k) {
        case TK_LT: op = BOP_LT; break;
        case TK_LE: op = BOP_LE; break;
        case TK_GT: op = BOP_GT; break;
        default:    op = BOP_GE; break;
        }
        lhs = expr_new_binop(op, lhs, rhs, loc);
    }
    return lhs;
}

/* add-expr = mul-expr { ("+" | "-") mul-expr }  -- left associative */
static Expr *parse_add(Parser *p) {
    Expr *lhs = parse_mul(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_PLUS && k != TK_MINUS) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_mul(p);
        BinOp op = (k == TK_PLUS) ? BOP_ADD : BOP_SUB;
        lhs = expr_new_binop(op, lhs, rhs, loc);
    }
    return lhs;
}

/* mul-expr = unary-expr { ("*" | "/" | "%") unary-expr }  -- left associative */
static Expr *parse_mul(Parser *p) {
    Expr *lhs = parse_unary(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_STAR && k != TK_SLASH && k != TK_PERCENT) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_unary(p);
        BinOp op;
        switch (k) {
        case TK_STAR:    op = BOP_MUL; break;
        case TK_SLASH:   op = BOP_DIV; break;
        default:         op = BOP_MOD; break; /* TK_PERCENT */
        }
        lhs = expr_new_binop(op, lhs, rhs, loc);
    }
    return lhs;
}

/* unary-expr = ("+"|"-"|"&"|"*") unary-expr | sizeof unary-or-type | primary { postfix } */
static Expr *parse_unary(Parser *p) {
    TokenKind k = peek(p)->kind;
    if (k == TK_PLUS || k == TK_MINUS) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *operand = parse_unary(p);
        UnaryOp op = (k == TK_MINUS) ? UOP_NEG : UOP_POS;
        return expr_new_unary(op, operand, loc);
    }
    if (k == TK_AMP) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        return expr_new_addr(parse_unary(p), loc);
    }
    if (k == TK_STAR) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        return expr_new_deref(parse_unary(p), loc);
    }
    if (k == TK_KW_SIZEOF) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        /* sizeof(T) or sizeof(expr) or sizeof expr. */
        if (peek(p)->kind == TK_LPAREN && is_type_start(p->tokens->data[p->pos + 1].kind)) {
            advance(p);
            Type t = parse_type(p);
            expect_kind(p, TK_RPAREN, "')'");
            Expr *e = expr_new_sizeof_type(t, loc);
            type_free(&t);
            return e;
        }
        return expr_new_sizeof_expr(parse_unary(p), loc);
    }
    return parse_primary(p);
}

/* Attach postfix operators (call `[i]` etc.) after any primary that
 * returned a valid expr.  Called from parse_primary. */
static Expr *parse_postfix(Parser *p, Expr *lhs) {
    for (;;) {
        if (peek(p)->kind == TK_LBRACKET) {
            SourceLoc loc = peek(p)->loc;
            advance(p);
            Expr *idx = parse_expr(p);
            expect_kind(p, TK_RBRACKET, "']'");
            lhs = expr_new_index(lhs, idx, loc);
            continue;
        }
        if (peek(p)->kind == TK_DOT) {
            SourceLoc loc = peek(p)->loc;
            advance(p);
            const Token *mn = peek(p);
            if (mn->kind != TK_IDENT) {
                die_at(mn->loc.file, mn->loc.line, mn->loc.col,
                       "expected member name after '.'");
            }
            advance(p);
            lhs = expr_new_member(lhs, mn->text, loc);
            continue;
        }
        if (peek(p)->kind == TK_ARROW) {
            SourceLoc loc = peek(p)->loc;
            advance(p);
            const Token *mn = peek(p);
            if (mn->kind != TK_IDENT) {
                die_at(mn->loc.file, mn->loc.line, mn->loc.col,
                       "expected member name after '->'");
            }
            advance(p);
            /* Desugar `p->x` to `(*p).x`. */
            Expr *deref = expr_new_deref(lhs, loc);
            lhs = expr_new_member(deref, mn->text, loc);
            continue;
        }
        break;
    }
    return lhs;
}

/* Decode a char-literal token's text (e.g. "'A'" or "'\\n'") to its int value.
 * Caller guarantees text starts and ends with a single quote. */
static int char_literal_value(const char *text) {
    /* text[0] == '\'' */
    if (text[1] == '\\') {
        switch (text[2]) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '\\': return '\\';
        case '\'': return '\'';
        case '0': return '\0';
        default:  return text[2];  /* unknown escape: use the char as-is */
        }
    }
    return (unsigned char)text[1];
}

/* primary-expr = INT_LITERAL | CHAR_LITERAL | IDENT [ "(" arg-list? ")" ]  | "(" (type ")" unary | expr ")" ) | ... */
static Expr *parse_primary(Parser *p) {
    const Token *t = peek(p);
    if (t->kind == TK_INT_LITERAL) {
        Expr *e = expr_new_int(atoi(t->text), t->loc);
        advance(p);
        return parse_postfix(p, e);
    }
    if (t->kind == TK_STRING_LITERAL) {
        /* Token text includes surrounding quotes; strip and process escapes. */
        const char *src = t->text;
        size_t slen = strlen(src);
        /* strip leading `"` and trailing `"` (lexer guarantees a leading `"`
         * but the trailing one might be missing on EOF — tolerate that). */
        if (slen >= 1 && src[0] == '"') { src++; slen--; }
        if (slen >= 1 && src[slen-1] == '"') slen--;
        /* Decode escape sequences into a temporary buffer. */
        char *buf = malloc(slen + 1);
        int blen = 0;
        for (size_t i = 0; i < slen; i++) {
            char c = src[i];
            if (c == '\\' && i + 1 < slen) {
                i++;
                switch (src[i]) {
                case 'n':  buf[blen++] = '\n'; break;
                case 't':  buf[blen++] = '\t'; break;
                case 'r':  buf[blen++] = '\r'; break;
                case '0':  buf[blen++] = '\0'; break;
                case '\\': buf[blen++] = '\\'; break;
                case '"':  buf[blen++] = '"'; break;
                case '\'': buf[blen++] = '\''; break;
                default:   buf[blen++] = src[i]; break;
                }
            } else {
                buf[blen++] = c;
            }
        }
        Expr *e = expr_new_str(buf, blen, t->loc);
        free(buf);
        advance(p);
        return parse_postfix(p, e);
    }
    if (t->kind == TK_CHAR_LITERAL) {
        Expr *e = expr_new_int(char_literal_value(t->text), t->loc);
        advance(p);
        return e;
    }
    if (t->kind == TK_IDENT) {
        const Token *ident = t;
        advance(p);
        if (peek(p)->kind == TK_LPAREN) {
            advance(p);
            Expr *call = expr_new_call(ident->text, ident->loc);
            if (peek(p)->kind != TK_RPAREN) {
                for (;;) {
                    Expr *arg = parse_expr(p);
                    expr_call_push_arg(call, arg);
                    if (peek(p)->kind == TK_COMMA) { advance(p); continue; }
                    break;
                }
            }
            expect_kind(p, TK_RPAREN, "')'");
            return parse_postfix(p, call);
        }
        return parse_postfix(p, expr_new_var(ident->text, ident->loc));
    }
    if (t->kind == TK_LPAREN) {
        /* Cast?  (Type) unary   vs.   (expr) */
        if (is_type_start(p->tokens->data[p->pos + 1].kind)) {
            SourceLoc loc = peek(p)->loc;
            advance(p);
            Type ty = parse_type(p);
            expect_kind(p, TK_RPAREN, "')'");
            Expr *e = expr_new_cast(ty, parse_unary(p), loc);
            type_free(&ty);
            return parse_postfix(p, e);
        }
        advance(p);
        Expr *e = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        return parse_postfix(p, e);
    }
    die_at(t->loc.file, t->loc.line, t->loc.col,
           "expected expression but got '%s'", t->text);
    return NULL; /* unreachable */
}

/* ---- statement parsing (forward declarations) ---- */

static void parse_stmt_list(Parser *p, StmtArray *out);
static Stmt parse_stmt(Parser *p);

/* stmt-list = { stmt } until '}' */
static void parse_stmt_list(Parser *p, StmtArray *out) {
    while (peek(p)->kind != TK_RBRACE) {
        const Token *t = peek(p);
        if (t->kind == TK_EOF) {
            die_at(t->loc.file, t->loc.line, t->loc.col,
                   "expected '}' but got end of file");
        }
        stmt_array_push(out, parse_stmt(p));
    }
}

/* stmt = decl-stmt | return-stmt | if-stmt | while-stmt | block | expr-stmt */
static Stmt parse_stmt(Parser *p) {
    TokenKind k = peek(p)->kind;
    if (is_type_start(k)) {
        /* decl-stmt: type IDENT [ "[" N "]" ]* ["=" expr] ";" */
        SourceLoc decl_loc = peek(p)->loc;
        Type ty = parse_type(p);
        const Token *name = peek(p);
        if (name->kind != TK_IDENT) {
            die_at(name->loc.file, name->loc.line, name->loc.col,
                   "expected variable name but got '%s'", name->text);
        }
        advance(p);
        /* Array declarator postfix: collect all "[N]" then wrap right-to-left
         * so `int a[3][2]` becomes array-of-3-of-array-of-2-of-int. */
        int dims[8];
        int ndims = 0;
        while (peek(p)->kind == TK_LBRACKET) {
            advance(p);
            const Token *nt = peek(p);
            if (nt->kind != TK_INT_LITERAL) {
                die_at(nt->loc.file, nt->loc.line, nt->loc.col,
                       "expected integer array length but got '%s'", nt->text);
            }
            int len = atoi(nt->text);
            if (len <= 0) {
                die_at(nt->loc.file, nt->loc.line, nt->loc.col,
                       "array length must be positive");
            }
            advance(p);
            expect_kind(p, TK_RBRACKET, "']'");
            if (ndims >= 8) die_at(nt->loc.file, nt->loc.line, nt->loc.col,
                                   "too many array dimensions");
            dims[ndims++] = len;
        }
        /* Wrap right-to-left. */
        for (int i = ndims - 1; i >= 0; i--) {
            Type wrapped = type_make_array(ty, dims[i]);
            type_free(&ty);
            ty = wrapped;
        }
        Stmt s;
        s.kind = ST_DECL;
        s.loc = decl_loc;
        s.u.decl.name = xstrdup(name->text);
        s.u.decl.type = ty;
        s.u.decl.init = NULL;
        if (peek(p)->kind == TK_ASSIGN) {
            advance(p);
            s.u.decl.init = parse_expr(p);
        }
        expect_kind(p, TK_SEMICOLON, "';'");
        return s;
    }
    if (k == TK_KW_RETURN) {
        /* return-stmt */
        const Token *kw = peek(p);
        advance(p);  /* consume "return" */
        Stmt s;
        s.kind = ST_RETURN;
        s.loc = kw->loc;
        s.u.value = parse_expr(p);
        expect_kind(p, TK_SEMICOLON, "';'");
        return s;
    }
    if (k == TK_KW_IF) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        Expr *cond = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        Stmt then_s = parse_stmt(p);
        Stmt *then_ptr = stmt_alloc();
        *then_ptr = then_s;
        Stmt *else_ptr = NULL;
        if (peek(p)->kind == TK_KW_ELSE) {
            advance(p);
            Stmt else_s = parse_stmt(p);
            else_ptr = stmt_alloc();
            *else_ptr = else_s;
        }
        Stmt s;
        s.kind = ST_IF;
        s.loc = kw->loc;
        s.u.if_s.cond = cond;
        s.u.if_s.then_s = then_ptr;
        s.u.if_s.else_s = else_ptr;
        return s;
    }
    if (k == TK_KW_WHILE) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        Expr *cond = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        Stmt body = parse_stmt(p);
        Stmt *body_ptr = stmt_alloc();
        *body_ptr = body;
        Stmt s;
        s.kind = ST_WHILE;
        s.loc = kw->loc;
        s.u.while_s.cond = cond;
        s.u.while_s.body = body_ptr;
        return s;
    }
    if (k == TK_KW_FOR) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        /* init: either a decl-stmt, an expr-stmt, or empty (just `;`). */
        Stmt *init_ptr = NULL;
        if (peek(p)->kind != TK_SEMICOLON) {
            Stmt is = parse_stmt(p);   /* consumes trailing `;` */
            init_ptr = stmt_alloc();
            *init_ptr = is;
        } else {
            advance(p);  /* consume `;` */
        }
        Expr *cond = NULL;
        if (peek(p)->kind != TK_SEMICOLON) cond = parse_expr(p);
        expect_kind(p, TK_SEMICOLON, "';'");
        Expr *step = NULL;
        if (peek(p)->kind != TK_RPAREN) step = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        Stmt body = parse_stmt(p);
        Stmt *body_ptr = stmt_alloc();
        *body_ptr = body;
        Stmt s;
        s.kind = ST_FOR;
        s.loc = kw->loc;
        s.u.for_s.init = init_ptr;
        s.u.for_s.cond = cond;
        s.u.for_s.step = step;
        s.u.for_s.body = body_ptr;
        return s;
    }
    if (k == TK_KW_BREAK) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s; s.kind = ST_BREAK; s.loc = kw->loc; return s;
    }
    if (k == TK_KW_CONTINUE) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s; s.kind = ST_CONTINUE; s.loc = kw->loc; return s;
    }
    if (k == TK_LBRACE) {
        const Token *lb = peek(p);
        advance(p);
        Stmt s;
        s.kind = ST_BLOCK;
        s.loc = lb->loc;
        stmt_array_init(&s.u.block);
        parse_stmt_list(p, &s.u.block);
        expect_kind(p, TK_RBRACE, "'}'");
        return s;
    }
    /* expr-stmt */
    const Token *t = peek(p);
    Stmt s;
    s.kind = ST_EXPR;
    s.loc = t->loc;
    s.u.expr = parse_expr(p);
    expect_kind(p, TK_SEMICOLON, "';'");
    return s;
}

static FunctionDecl parse_function_decl(Parser *p) {
    SourceLoc fn_loc = peek(p)->loc;
    Type ret_ty = parse_type(p);

    const Token *name = peek(p);
    if (name->kind != TK_IDENT) {
        die_at(name->loc.file, name->loc.line, name->loc.col,
               "expected function name but got '%s'", name->text);
    }
    advance(p);

    expect_kind(p, TK_LPAREN, "'('");

    FunctionDecl fn;
    fn.name = xstrdup(name->text);
    fn.ret_type = ret_ty;
    param_array_init(&fn.params);
    stmt_array_init(&fn.body);
    fn.loc = fn_loc;

    /* Parameter list: type IDENT ("," type IDENT)* — or empty. */
    if (peek(p)->kind != TK_RPAREN) {
        for (;;) {
            if (!is_type_start(peek(p)->kind)) {
                const Token *t = peek(p);
                die_at(t->loc.file, t->loc.line, t->loc.col,
                       "expected type for parameter but got '%s'", t->text);
            }
            Type pty = parse_type(p);
            const Token *pname = peek(p);
            if (pname->kind != TK_IDENT) {
                die_at(pname->loc.file, pname->loc.line, pname->loc.col,
                       "expected parameter name but got '%s'", pname->text);
            }
            advance(p);
            /* Array parameter postfix `[N]` or `[]` — normalized to pointer. */
            if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                /* accept optional length, ignore it: array param decays to ptr */
                if (peek(p)->kind == TK_INT_LITERAL) advance(p);
                expect_kind(p, TK_RBRACKET, "']'");
                Type wrapped = type_make_ptr(pty);
                type_free(&pty);
                pty = wrapped;
            }
            param_array_push(&fn.params, pname->text, pty, pname->loc);
            if (peek(p)->kind == TK_COMMA) { advance(p); continue; }
            break;
        }
        if (fn.params.len > 16) {
            die_at(fn.loc.file, fn.loc.line, fn.loc.col,
                   "more than 16 parameters not supported");
        }
    }

    expect_kind(p, TK_RPAREN, "')'");
    expect_kind(p, TK_LBRACE, "'{'");

    parse_stmt_list(p, &fn.body);

    expect_kind(p, TK_RBRACE, "'}'");

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
    p.tu = tu;

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
               "'import' is not supported yet");
    }

    /* At file scope: each top-level declaration starts with a type OR
     * with `struct TAG { ... };` — a struct definition (no variable). */
    while (peek(&p)->kind != TK_EOF) {
        /* Struct definition: `struct TAG { type NAME [ [N] ]* ; ... };`
         * Distinguish from `struct TAG x;` global by lookahead — after
         * `struct TAG` we expect `{` for a definition. */
        if (peek(&p)->kind == TK_KW_STRUCT) {
            size_t save = p.pos;
            advance(&p);
            const Token *tag = peek(&p);
            if (tag->kind == TK_IDENT) advance(&p);
            if (peek(&p)->kind == TK_LBRACE) {
                /* Struct definition. */
                if (struct_registry_find(&tu->structs, tag->text)) {
                    die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                           "redefinition of struct '%s'", tag->text);
                }
                StructDef *sd = struct_registry_add(&tu->structs, tag->text, tag->loc);
                advance(&p);  /* consume `{` */
                while (peek(&p)->kind != TK_RBRACE) {
                    Type mty = parse_type(&p);
                    const Token *mn = peek(&p);
                    if (mn->kind != TK_IDENT) {
                        die_at(mn->loc.file, mn->loc.line, mn->loc.col,
                               "expected member name");
                    }
                    advance(&p);
                    /* Array dims. */
                    int dims[8]; int ndims = 0;
                    while (peek(&p)->kind == TK_LBRACKET) {
                        advance(&p);
                        const Token *nt = peek(&p);
                        if (nt->kind != TK_INT_LITERAL) {
                            die_at(nt->loc.file, nt->loc.line, nt->loc.col,
                                   "expected integer array length");
                        }
                        int len = atoi(nt->text);
                        advance(&p);
                        expect_kind(&p, TK_RBRACKET, "']'");
                        dims[ndims++] = len;
                    }
                    for (int i = ndims - 1; i >= 0; i--) {
                        Type w = type_make_array(mty, dims[i]);
                        type_free(&mty); mty = w;
                    }
                    expect_kind(&p, TK_SEMICOLON, "';'");
                    struct_def_push_member(sd, mn->text, mty);
                    type_free(&mty);
                }
                expect_kind(&p, TK_RBRACE, "'}'");
                expect_kind(&p, TK_SEMICOLON, "';'");
                continue;
            } else {
                /* Not a definition — reset and fall through to declaration. */
                p.pos = save;
            }
        }

        /* Save position, look ahead to find kind. */
        size_t save = p.pos;
        /* Skip signed/unsigned prefix */
        if (peek(&p)->kind == TK_KW_SIGNED || peek(&p)->kind == TK_KW_UNSIGNED)
            advance(&p);
        /* Skip type keyword */
        if (peek(&p)->kind == TK_KW_INT || peek(&p)->kind == TK_KW_CHAR
            || peek(&p)->kind == TK_KW_SHORT || peek(&p)->kind == TK_KW_LONG) {
            advance(&p);
        } else if (peek(&p)->kind == TK_KW_STRUCT) {
            advance(&p);
            if (peek(&p)->kind == TK_IDENT) advance(&p);
        }
        /* Skip pointer stars */
        while (peek(&p)->kind == TK_STAR) advance(&p);
        /* Skip identifier */
        if (peek(&p)->kind == TK_IDENT) advance(&p);
        /* Skip array dims [N] */
        while (peek(&p)->kind == TK_LBRACKET) {
            advance(&p);
            while (peek(&p)->kind != TK_RBRACKET && peek(&p)->kind != TK_EOF)
                advance(&p);
            if (peek(&p)->kind == TK_RBRACKET) advance(&p);
        }
        int is_func = (peek(&p)->kind == TK_LPAREN);
        p.pos = save;

        if (is_func) {
            FunctionDecl fn = parse_function_decl(&p);
            if (tu->functions.len >= tu->functions.cap) {
                size_t new_cap = tu->functions.cap ? tu->functions.cap * 2 : 4;
                tu->functions.data = realloc(tu->functions.data,
                                             new_cap * sizeof(FunctionDecl));
                tu->functions.cap = new_cap;
            }
            tu->functions.data[tu->functions.len++] = fn;
        } else {
            /* Global variable: reuse the decl-stmt parser via parse_stmt. */
            Stmt s = parse_stmt(&p);
            if (s.kind != ST_DECL) {
                die_at(s.loc.file, s.loc.line, s.loc.col,
                       "only variable declarations allowed at file scope");
            }
            stmt_array_push(&tu->globals, s);
        }
    }
}
