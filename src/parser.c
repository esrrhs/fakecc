#include "fakecc/parser.h"
#include "fakecc/pkg.h"
#include "fakecc/common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TranslationUnit *g_parser_tu = NULL;
const StructRegistry *get_parser_structs(void) {
    return g_parser_tu ? &g_parser_tu->structs : NULL;
}

typedef struct {
    const TokenArray *tokens;
    size_t pos;
    TranslationUnit *tu;   /* backing TU — parser writes structs directly */
    PkgContext *pkg_ctx;   /* NULL → imports are rejected */
    /* Prepended statements: multi-declarators (`int a, b;`) parse as several
     * ST_DECLs but a statement slot holds one — the trailing declarators are
     * queued here and flushed into the enclosing stmt-list before the next
     * real statement, so all names land in the same (current) scope. */
    StmtArray prepend;
    /* Counter for generating unique tags for anonymous structs/unions/enums
     * (`__anon_0`, `__anon_1`, ...).  Each anonymous definition gets a fresh
     * tag so its type is distinct (C semantics). */
    int anon_counter;
    /* Innermost function currently being parsed; used for __FUNCTION__. */
    const char *cur_fn_name;
    /* tu->typedefs.len at function-body entry; locals are truncated back. */
    size_t typedef_mark;
} Parser;

static const Token *peek(const Parser *p) {
    return &p->tokens->data[p->pos];
}

static const Token *advance(Parser *p) {
    return &p->tokens->data[p->pos++];
}

/* A token usable as a member/variable name.  Normally TK_IDENT, but fakecc
 * uses `package` as a keyword while the source also uses it as a struct member
 * (`TranslationUnit.package`); accept it as a name in member positions. */
static int is_name_token(TokenKind k) {
    return k == TK_IDENT || k == TK_KW_PACKAGE;
}

/* Forward declarations for struct/enum body parsers (defined after
 * parse_specifiers, which calls them for anonymous types). */
static void parse_struct_body(Parser *p, StructDef *sd);
static void parse_enum_body(Parser *p, EnumDef *ed);
static int int_literal_value(const char *text);
static Type parse_declarator(Parser *p, Type base, char **name_out);
static FunctionDecl parse_function_decl(Parser *p);
static void parse_stmt_list(Parser *p, StmtArray *out);
static Stmt parse_stmt(Parser *p);
static Expr *parse_expr(Parser *p);
static Expr *parse_ternary(Parser *p);
static Type parse_type_abstract(Parser *p);

/* Skip a GCC-style `__attribute__((...))` annotation if present at the current
 * position.  Tokenizes as IDENT `__attribute__`, then `(`, then a balanced
 * parenthesized group.  Returns 1 if an attribute was consumed, else 0. */
static int skip_attribute(Parser *p);

static void expect_kind(Parser *p, TokenKind kind, const char *msg) {
    const Token *t = peek(p);
    if (t->kind != kind) {
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "expected %s but got '%s'", msg, t->text);
    }
    advance(p);
}

static char *g_parsed_alias = NULL;
static int g_parsed_mode_size = 0;
static int g_parsed_no_instrument = 0;

static int parse_attribute(Parser *p, int *align, int *packed, int *sso, int *vec_size, char **alias_out) {
    if (peek(p)->kind == TK_LBRACKET && p->pos + 1 < p->tokens->len && p->tokens->data[p->pos + 1].kind == TK_LBRACKET) {
        advance(p);
        advance(p);
        int depth = 1;
        while (depth > 0 && peek(p)->kind != TK_EOF) {
            if (peek(p)->kind == TK_LBRACKET && p->pos + 1 < p->tokens->len && p->tokens->data[p->pos + 1].kind == TK_LBRACKET) {
                depth++;
                advance(p);
                advance(p);
            } else if (peek(p)->kind == TK_RBRACKET && p->pos + 1 < p->tokens->len && p->tokens->data[p->pos + 1].kind == TK_RBRACKET) {
                depth--;
                advance(p);
                advance(p);
            } else {
                advance(p);
            }
        }
        return 1;
    }
    if (peek(p)->kind != TK_IDENT) return 0;
    if (strcmp(peek(p)->text, "__asm__") == 0 || strcmp(peek(p)->text, "asm") == 0 || strcmp(peek(p)->text, "__asm") == 0) {
        if (p->pos + 1 < p->tokens->len && p->tokens->data[p->pos + 1].kind == TK_LPAREN) {
            size_t k = p->pos + 2;
            int has_str = 0;
            while (k < p->tokens->len && p->tokens->data[k].kind == TK_STRING_LITERAL) {
                has_str = 1;
                k++;
            }
            if (has_str && k < p->tokens->len && p->tokens->data[k].kind == TK_RPAREN) {
                advance(p); /* consume asm */
                advance(p); /* consume ( */
                char buf[512] = {0};
                int blen = 0;
                while (peek(p)->kind == TK_STRING_LITERAL) {
                    const char *s = peek(p)->text;
                    size_t slen = strlen(s);
                    size_t start = (slen >= 2 && s[0] == '"') ? 1 : 0;
                    size_t end = (slen >= 2 && s[slen-1] == '"') ? slen - 1 : slen;
                    for (size_t i = start; i < end && blen < 510; i++) {
                        buf[blen++] = s[i];
                    }
                    buf[blen] = '\0';
                    advance(p);
                }
                if (peek(p)->kind == TK_RPAREN) advance(p);
                if (buf[0]) {
                    if (alias_out) {
                        free(*alias_out);
                        *alias_out = xstrdup(buf);
                    }
                    free(g_parsed_alias);
                    g_parsed_alias = xstrdup(buf);
                }
                return 1;
            }
        }
        return 0;
    }
    if (strcmp(peek(p)->text, "__attribute__") != 0
        && strcmp(peek(p)->text, "__attribute") != 0)
        return 0;
    advance(p);  /* consume `__attribute__` */
    if (peek(p)->kind != TK_LPAREN) return 0;
    advance(p);  /* consume outer `(` */
    if (peek(p)->kind != TK_LPAREN) {
        return 0;
    }
    advance(p);  /* consume inner `(` */
    int depth = 2;
    while (depth > 1 && peek(p)->kind != TK_EOF) {
        if (peek(p)->kind == TK_IDENT) {
            const char *name = peek(p)->text;
            if (strcmp(name, "aligned") == 0 || strcmp(name, "__aligned__") == 0) {
                advance(p);
                if (peek(p)->kind == TK_LPAREN) {
                    advance(p);
                    depth++;
                    Expr *e = parse_ternary(p);
                    long long val = 0;
                    if (fold_const_int(e, &val)) {
                        if (align && val > *align) *align = (int)val;
                    }
                    expr_free(e);
                    if (peek(p)->kind == TK_RPAREN) {
                        advance(p);
                        depth--;
                    }
                } else {
                    if (align && 16 > *align) *align = 16;
                }
                continue;
            } else if (strcmp(name, "packed") == 0 || strcmp(name, "__packed__") == 0) {
                if (packed) *packed = 1;
                advance(p);
                continue;
            } else if (strcmp(name, "vector_size") == 0 || strcmp(name, "__vector_size__") == 0) {
                advance(p);
                if (peek(p)->kind == TK_LPAREN) {
                    advance(p);
                    depth++;
                    Expr *e = parse_ternary(p);
                    long long val = 0;
                    if (fold_const_int(e, &val)) {
                        if (vec_size) *vec_size = (int)val;
                    }
                    expr_free(e);
                    if (peek(p)->kind == TK_RPAREN) {
                        advance(p);
                        depth--;
                    }
                }
                continue;
            } else if (strcmp(name, "scalar_storage_order") == 0 || strcmp(name, "__scalar_storage_order__") == 0) {
                advance(p);
                if (peek(p)->kind == TK_LPAREN) {
                    advance(p);
                    depth++;
                    if (peek(p)->kind == TK_STRING_LITERAL) {
                        const char *s = peek(p)->text;
                        if (sso) {
                            if (strstr(s, "big-endian")) *sso = 1;
                            else if (strstr(s, "little-endian")) *sso = 2;
                        }
                        advance(p);
                    }
                    if (peek(p)->kind == TK_RPAREN) {
                        advance(p);
                        depth--;
                    }
                }
                continue;
            } else if (strcmp(name, "alias") == 0 || strcmp(name, "__alias__") == 0) {
                advance(p);
                if (peek(p)->kind == TK_LPAREN) {
                    advance(p);
                    depth++;
                    if (peek(p)->kind == TK_STRING_LITERAL) {
                        const char *src = peek(p)->text;
                        size_t slen = strlen(src);
                        if (slen >= 1 && src[0] == '"') { src++; slen--; }
                        if (slen >= 1 && src[slen-1] == '"') slen--;
                        if (g_parsed_alias) free(g_parsed_alias);
                        g_parsed_alias = malloc(slen + 1);
                        if (!g_parsed_alias) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
                        memcpy(g_parsed_alias, src, slen);
                        g_parsed_alias[slen] = '\0';
                        if (alias_out) {
                            if (*alias_out) free(*alias_out);
                            *alias_out = xstrdup(g_parsed_alias);
                        }
                        advance(p);
                    }
                    if (peek(p)->kind == TK_RPAREN) {
                        advance(p);
                        depth--;
                    }
                }
                continue;
            } else if (strcmp(name, "mode") == 0 || strcmp(name, "__mode__") == 0) {
                advance(p);
                if (peek(p)->kind == TK_LPAREN) {
                    advance(p);
                    depth++;
                    if (peek(p)->kind == TK_IDENT) {
                        const char *mname = peek(p)->text;
                        int msize = 0;
                        if (strcmp(mname, "QI") == 0 || strcmp(mname, "__QI__") == 0 ||
                            strcmp(mname, "byte") == 0 || strcmp(mname, "__byte__") == 0)
                            msize = 1;
                        else if (strcmp(mname, "HI") == 0 || strcmp(mname, "__HI__") == 0)
                            msize = 2;
                        else if (strcmp(mname, "SI") == 0 || strcmp(mname, "__SI__") == 0 ||
                                 strcmp(mname, "word") == 0 || strcmp(mname, "__word__") == 0)
                            msize = 4;
                        else if (strcmp(mname, "DI") == 0 || strcmp(mname, "__DI__") == 0)
                            msize = 8;
                        else if (strcmp(mname, "TI") == 0 || strcmp(mname, "__TI__") == 0)
                            msize = 16;
                        if (msize > 0) g_parsed_mode_size = msize;
                        advance(p);
                    }
                    if (peek(p)->kind == TK_RPAREN) {
                        advance(p);
                        depth--;
                    }
                }
                continue;
            } else if (strcmp(name, "no_instrument_function") == 0 || strcmp(name, "__no_instrument_function__") == 0) {
                g_parsed_no_instrument = 1;
                advance(p);
                continue;
            }
        }
        if (peek(p)->kind == TK_LPAREN) depth++;
        else if (peek(p)->kind == TK_RPAREN) {
            depth--;
            if (depth == 1) {
                advance(p);
                if (peek(p)->kind == TK_RPAREN) {
                    advance(p);
                    depth--;
                }
                break;
            }
        }
        advance(p);
    }
    while (depth > 0 && peek(p)->kind != TK_EOF) {
        if (peek(p)->kind == TK_LPAREN) depth++;
        else if (peek(p)->kind == TK_RPAREN) depth--;
        advance(p);
    }
    return 1;
}

static int skip_attribute(Parser *p) {
    return parse_attribute(p, NULL, NULL, NULL, NULL, NULL);
}

/* True if `name` appears in this TU's import list. */
static int tu_has_import(const TranslationUnit *tu, const char *name) {
    for (size_t i = 0; i < tu->imports.len; i++)
        if (strcmp(tu->imports.data[i].name, name) == 0) return 1;
    return 0;
}

/* Resolve a typedef from an imported package or (for unqualified names) from
 * the current package's already-parsed sibling files / export table.
 * Ensures any referenced StructDef is cloned into the local TU; does NOT
 * inject a local typedef alias (so `typedef io.FILE FILE;` can bind the name). */
static const Type *resolve_pkg_typedef(Parser *p, const char *pkg_name,
                                       const char *type_name) {
    if (!p->pkg_ctx) return NULL;
    Package *pkg = pkg_find(p->pkg_ctx, pkg_name);
    if (!pkg) return NULL;
    const Type *t = pkg_find_typedef(pkg, type_name);
    if (!t) {
        /* Same-package siblings may not have exports built yet — search
         * already-parsed files directly. */
        for (size_t f = 0; f < pkg->nfiles; f++) {
            TranslationUnit *sib = &pkg->files[f];
            if (sib == p->tu) continue;
            t = typedef_registry_find(&sib->typedefs, type_name);
            if (t) break;
        }
    }
    if (!t) return NULL;
    /* Clone referenced structs into the local registry for member layout. */
    if (t->kind == TY_STRUCT && t->tag) {
        const StructDef *sd = pkg_find_struct(pkg, t->tag);
        if (!sd) {
            for (size_t f = 0; f < pkg->nfiles && !sd; f++)
                sd = struct_registry_find_c(&pkg->files[f].structs, t->tag);
        }
        if (sd) pkg_clone_struct_into(&p->tu->structs, sd);
    } else if (t->kind == TY_PTR && t->pointee && t->pointee->kind == TY_STRUCT
               && t->pointee->tag) {
        const StructDef *sd = pkg_find_struct(pkg, t->pointee->tag);
        if (!sd) {
            for (size_t f = 0; f < pkg->nfiles && !sd; f++)
                sd = struct_registry_find_c(&pkg->files[f].structs,
                                           t->pointee->tag);
        }
        if (sd) pkg_clone_struct_into(&p->tu->structs, sd);
    }
    return t;
}

/* Look up an unqualified typedef: local first, then current package. */
static const Type *find_typedef_with_fallback(Parser *p, const char *name) {
    const Type *t = typedef_registry_find(&p->tu->typedefs, name);
    if (t) return t;
    if (!p->pkg_ctx || !p->tu->package.name) return NULL;
    return resolve_pkg_typedef(p, p->tu->package.name, name);
}

/* Recognize a type at position `pos` (keywords, typedefs, pkg.Type). */
static int is_type_start(const Parser *p, size_t pos) {
    TokenKind k = p->tokens->data[pos].kind;
    if (k == TK_LBRACKET && pos + 1 < p->tokens->len && p->tokens->data[pos + 1].kind == TK_LBRACKET) {
        size_t k = pos + 2;
        int depth = 1;
        while (k < p->tokens->len && depth > 0) {
            if (p->tokens->data[k].kind == TK_LBRACKET && k + 1 < p->tokens->len && p->tokens->data[k + 1].kind == TK_LBRACKET) {
                depth++;
                k += 2;
            } else if (p->tokens->data[k].kind == TK_RBRACKET && k + 1 < p->tokens->len && p->tokens->data[k + 1].kind == TK_RBRACKET) {
                depth--;
                k += 2;
            } else {
                k++;
            }
        }
        if (k < p->tokens->len) return is_type_start(p, k);
        return 0;
    }
    if (k == TK_KW_VOID || k == TK_KW_INT || k == TK_KW_CHAR || k == TK_KW_SHORT
        || k == TK_KW_LONG || k == TK_KW_SIGNED || k == TK_KW_UNSIGNED
        || k == TK_KW_FLOAT || k == TK_KW_DOUBLE || k == TK_KW_BOOL
        || k == TK_KW_STRUCT || k == TK_KW_ENUM || k == TK_KW_UNION
        || k == TK_KW_CONST || k == TK_KW_STATIC || k == TK_KW_EXTERN
        || k == TK_KW_VOLATILE || k == TK_KW_RESTRICT || k == TK_KW_INLINE
        || k == TK_KW_COMPLEX)
        return 1;
    if (k == TK_IDENT) {
        const char *text = p->tokens->data[pos].text;
        if (strcmp(text, "register") == 0 || strcmp(text, "auto") == 0)
            return is_type_start(p, pos + 1);
        if (strcmp(text, "__attribute__") == 0 || strcmp(text, "__attribute") == 0) {
            size_t k = pos + 1;
            if (k < p->tokens->len && p->tokens->data[k].kind == TK_LPAREN) {
                int depth = 0;
                while (k < p->tokens->len) {
                    if (p->tokens->data[k].kind == TK_LPAREN) depth++;
                    else if (p->tokens->data[k].kind == TK_RPAREN) {
                        depth--;
                        if (depth == 0) { k++; break; }
                    }
                    k++;
                }
                if (k < p->tokens->len) return is_type_start(p, k);
            }
            return 0;
        }
        if (strcmp(text, "__int128") == 0 || strcmp(text, "__int128_t") == 0 || strcmp(text, "__uint128_t") == 0)
            return 1;
        if (strcmp(text, "typeof") == 0 || strcmp(text, "__typeof__") == 0 || strcmp(text, "__typeof") == 0)
            return 1;
        if (typedef_registry_find(&p->tu->typedefs, text))
            return 1;
        /* pkg.Type — IDENT '.' IDENT where IDENT is an imported package. */
        if (pos + 2 < p->tokens->len
            && p->tokens->data[pos + 1].kind == TK_DOT
            && p->tokens->data[pos + 2].kind == TK_IDENT
            && tu_has_import(p->tu, text)
            && p->pkg_ctx) {
            Package *pkg = pkg_find(p->pkg_ctx, text);
            if (pkg && pkg_find_typedef(pkg, p->tokens->data[pos + 2].text))
                return 1;
        }
        /* Same-package unqualified typedef or struct (siblings / export table). */
        if (p->pkg_ctx && p->tu->package.name) {
            Package *cur = pkg_find(p->pkg_ctx, p->tu->package.name);
            if (cur && pkg_find_typedef(cur, text))
                return 1;
            if (cur && pkg_find_struct(cur, text))
                return 1;
            if (cur) {
                for (size_t f = 0; f < cur->nfiles; f++) {
                    if (&cur->files[f] == p->tu) continue;
                    if (typedef_registry_find(&cur->files[f].typedefs, text))
                        return 1;
                    if (struct_registry_find(&cur->files[f].structs, text))
                        return 1;
                }
            }
        }
    }
    return 0;
}

static Type get_or_create_complex_type(Parser *p, Type base) {
    char tag[64];
    int bw = base.width ? base.width : 4;
    const char *bname = (base.kind == TY_FLOAT)
        ? (bw == 4 ? "float" : (bw == 8 ? "double" : "ldouble"))
        : (bw == 1 ? "char" : (bw == 2 ? "short" : (bw == 8 ? "long" : "int")));
    snprintf(tag, sizeof(tag), "__complex_%s", bname);
    StructDef *sd = struct_registry_find(&p->tu->structs, tag);
    if (!sd) {
        SourceLoc loc; memset(&loc, 0, sizeof(loc));
        sd = struct_registry_add(&p->tu->structs, tag, loc);
        struct_def_push_member(sd, "__real", type_clone(base), -1);
        struct_def_push_member(sd, "__imag", type_clone(base), -1);
        struct_def_finish(sd);
    }
    return type_make_struct(tag, sd->size);
}

/* Parse specifiers: const + base type (void/struct/union/enum/typedef/int).
 * This is the old `parse_type` minus the trailing `*` chain — pointers and
 * other declarator suffixes are handled separately by `parse_declarator`. */
static void parse_trailing_qualifiers(Parser *p, int *is_const, int *is_volatile, int *is_restrict, int *is_complex, int *storage_class, int *vec_size) {
    for (;;) {
        int attr_align = 0, attr_packed = 0, attr_sso = 0, attr_vec = 0;
        if (parse_attribute(p, &attr_align, &attr_packed, &attr_sso, &attr_vec, NULL)) {
            if (attr_vec > 0 && vec_size) *vec_size = attr_vec;
            continue;
        }
        if (peek(p)->kind == TK_KW_CONST) { *is_const = 1; advance(p); }
        else if (peek(p)->kind == TK_KW_VOLATILE) { *is_volatile = 1; advance(p); }
        else if (peek(p)->kind == TK_KW_RESTRICT) {
            const Token *t = peek(p);
            die_at(t->loc.file, t->loc.line, t->loc.col, "invalid use of 'restrict'");
        }
        else if (peek(p)->kind == TK_KW_INLINE) { advance(p); }
        else if (is_complex && peek(p)->kind == TK_KW_COMPLEX) { *is_complex = 1; advance(p); }
        else if (peek(p)->kind == TK_KW_STATIC) {
            if (storage_class) *storage_class = 1;
            advance(p);
        }
        else if (peek(p)->kind == TK_KW_EXTERN) {
            if (storage_class) *storage_class = 2;
            advance(p);
        }
        else if (peek(p)->kind == TK_IDENT
                 && (strcmp(peek(p)->text, "register") == 0
                     || strcmp(peek(p)->text, "auto") == 0)) {
            advance(p);
        }
        else break;
    }
}

static Type finish_specifiers(Type t, int is_const, int is_volatile, int is_restrict, int is_complex, int vec_size, Parser *p) {
    t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
    if (is_complex) t = get_or_create_complex_type(p, t);
    if (g_parsed_mode_size > 0) {
        if (t.kind == TY_INT || t.kind == TY_FLOAT) t.width = g_parsed_mode_size;
        g_parsed_mode_size = 0;
    }
    if (vec_size > 0 && !t.is_vector) t = type_make_vector(t, vec_size);
    return t;
}

static Type parse_specifiers_full(Parser *p, int *storage_class) {
    /* Type qualifiers — flag the resulting type.  `const` gates assignment in
     * sema; `volatile`/`restrict` are no-ops without an optimizer (stored for
     * completeness).  All three may appear in any order (C permits mixing). */
    int is_const = 0, is_volatile = 0, is_restrict = 0, is_complex = 0, attr_vec = 0;
    parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);

    /* typeof / __typeof__ / __typeof */
    if (peek(p)->kind == TK_IDENT && (strcmp(peek(p)->text, "typeof") == 0 ||
                                      strcmp(peek(p)->text, "__typeof__") == 0 ||
                                      strcmp(peek(p)->text, "__typeof") == 0)) {
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        Type t;
        if (is_type_start(p, p->pos)) {
            t = parse_type_abstract(p);
        } else {
            const Token *tok = peek(p);
            Type found = type_default_int();
            if (tok->kind == TK_IDENT) {
                const EnumConstant *ec = enum_registry_find_constant(&p->tu->enums, tok->text);
                if (ec) {
                    found = type_default_int();
                } else {
                    for (size_t g = 0; g < p->tu->globals.len; g++) {
                        if (p->tu->globals.data[g].kind == ST_DECL &&
                            strcmp(p->tu->globals.data[g].u.decl.name, tok->text) == 0) {
                            found = type_clone(p->tu->globals.data[g].u.decl.type);
                            break;
                        }
                    }
                }
            }
            Expr *e = parse_expr(p);
            expr_free(e);
            t = found;
        }
        expect_kind(p, TK_RPAREN, "')'");
        parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
        return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
    }

    /* void — only meaningful as a return type or as void* (pointer to void).
     * A lone `void` variable is rejected later in sema. */
    if (peek(p)->kind == TK_KW_VOID) {
        advance(p);
        parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
        Type t = type_make_void();
        return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
    }
    /* struct [Tag] — if Tag is present, it's a use of a (possibly forward)
     * struct; look it up in the registry.  If the next token is `{` (no tag),
     * it's an ANONYMOUS struct definition (`typedef struct { ... } Name;`):
     * generate a unique tag, parse the body, and register it. */
    if (peek(p)->kind == TK_KW_STRUCT) {
        advance(p);
        int attr_align = 0, attr_packed = 0, attr_sso = 0;
        while (parse_attribute(p, &attr_align, &attr_packed, &attr_sso, &attr_vec, NULL)) {}
        if (peek(p)->kind == TK_LBRACE) {
            /* Anonymous struct definition. */
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            StructDef *sd = struct_registry_add(&p->tu->structs, tag, peek(p)->loc);
            if (attr_align > sd->align) sd->align = attr_align;
            if (attr_packed) { sd->align = 1; sd->is_packed = 1; }
            if (attr_sso == 1) struct_def_apply_sso(sd, 1);
            else if (attr_sso == 2) struct_def_apply_sso(sd, 0);
            parse_struct_body(p, sd);
            /* parse_struct_body may realloc the registry (nested anonymous
             * structs/unions), invalidating sd — re-fetch before reading size. */
            sd = struct_registry_find(&p->tu->structs, tag);
            parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
            Type t = type_make_struct(tag, sd ? sd->size : 0);
            return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected struct tag but got '%s'", tag->text);
        }
        advance(p);
        while (parse_attribute(p, &attr_align, &attr_packed, &attr_sso, &attr_vec, NULL)) {}
        /* If '{' follows, this is a struct definition at use site
         * (`struct Tag { ... }`), not just a forward reference. */
        if (peek(p)->kind == TK_LBRACE) {
            if (struct_registry_find(&p->tu->structs, tag->text)) {
                die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                       "redefinition of struct '%s'", tag->text);
            }
            StructDef *sd = struct_registry_add(&p->tu->structs, tag->text, peek(p)->loc);
            if (attr_align > sd->align) sd->align = attr_align;
            if (attr_packed) { sd->align = 1; sd->is_packed = 1; }
            if (attr_sso == 1) struct_def_apply_sso(sd, 1);
            else if (attr_sso == 2) struct_def_apply_sso(sd, 0);
            parse_struct_body(p, sd);
            sd = struct_registry_find(&p->tu->structs, tag->text);
            parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
            Type t = type_make_struct(tag->text, sd ? sd->size : 0);
            return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
        }
        StructDef *sd = struct_registry_find(&p->tu->structs, tag->text);
        long long size = sd ? sd->size : 0;
        parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
        Type t = type_make_struct(tag->text, size);
        return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
    }
    /* union [Tag] — same as struct: a Tag is a use, `{` begins an anonymous
     * union definition. */
    if (peek(p)->kind == TK_KW_UNION) {
        advance(p);
        int attr_align = 0, attr_packed = 0, attr_sso = 0;
        while (parse_attribute(p, &attr_align, &attr_packed, &attr_sso, &attr_vec, NULL)) {}
        if (peek(p)->kind == TK_LBRACE) {
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            StructDef *sd = struct_registry_add(&p->tu->structs, tag, peek(p)->loc);
            sd->is_union = 1;
            if (attr_align > sd->align) sd->align = attr_align;
            if (attr_packed) { sd->align = 1; sd->is_packed = 1; }
            if (attr_sso == 1) struct_def_apply_sso(sd, 1);
            else if (attr_sso == 2) struct_def_apply_sso(sd, 0);
            parse_struct_body(p, sd);
            sd = struct_registry_find(&p->tu->structs, tag);
            parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
            Type t = type_make_struct(tag, sd ? sd->size : 0);
            return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected union tag but got '%s'", tag->text);
        }
        advance(p);
        while (parse_attribute(p, &attr_align, &attr_packed, &attr_sso, &attr_vec, NULL)) {}
        /* If '{' follows, this is a union definition at use site
         * (`union Tag { ... }`), not just a forward reference. */
        if (peek(p)->kind == TK_LBRACE) {
            if (struct_registry_find(&p->tu->structs, tag->text)) {
                die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                       "redefinition of union '%s'", tag->text);
            }
            StructDef *sd = struct_registry_add(&p->tu->structs, tag->text, peek(p)->loc);
            sd->is_union = 1;
            if (attr_align > sd->align) sd->align = attr_align;
            if (attr_packed) { sd->align = 1; sd->is_packed = 1; }
            if (attr_sso == 1) struct_def_apply_sso(sd, 1);
            else if (attr_sso == 2) struct_def_apply_sso(sd, 0);
            parse_struct_body(p, sd);
            sd = struct_registry_find(&p->tu->structs, tag->text);
            parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
            Type t = type_make_struct(tag->text, sd ? sd->size : 0);
            return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
        }
        StructDef *sd = struct_registry_find(&p->tu->structs, tag->text);
        long long size = sd ? sd->size : 0;
        parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
        Type t = type_make_struct(tag->text, size);
        return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
    }
    /* float / double — TY_FLOAT with width 4 or 8. */
    if (peek(p)->kind == TK_KW_FLOAT) {
        advance(p);
        parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
        Type t = type_make_float(4);
        return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
    }
    if (peek(p)->kind == TK_KW_DOUBLE) {
        advance(p);
        parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
        Type t = type_make_float(8);
        return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
    }
    /* long double — TY_FLOAT with width 16 (x87 80-bit extended).  Detected by
     * a `long` keyword immediately followed by `double` (lookahead without
     * consuming on mismatch, since `long` alone begins an integer type). */
    if (peek(p)->kind == TK_KW_LONG
        && p->pos + 1 < p->tokens->len
        && p->tokens->data[p->pos + 1].kind == TK_KW_DOUBLE) {
        advance(p); /* consume `long` */
        advance(p); /* consume `double` */
        parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
        Type t = type_make_float(16);
        return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
    }
    /* enum Tag — treated as int for the type system. */
    if (peek(p)->kind == TK_KW_ENUM) {
        advance(p);
        if (peek(p)->kind == TK_LBRACE) {
            /* Anonymous enum definition (`typedef enum { ... } Name;`).
             * Register it under a unique tag so its constants resolve, even
             * though the type itself is just int. */
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            EnumDef *ed = enum_registry_add(&p->tu->enums, tag, peek(p)->loc);
            parse_enum_body(p, ed);
            parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
            Type t = type_default_int();
            t.enum_id = (int)(ed - p->tu->enums.data + 1);
            return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected enum tag but got '%s'", tag->text);
        }
        advance(p);
        if (peek(p)->kind == TK_LBRACE) {
            EnumDef *ed = enum_registry_add(&p->tu->enums, tag->text, peek(p)->loc);
            parse_enum_body(p, ed);
        }
        EnumDef *ed = enum_registry_find(&p->tu->enums, tag->text);
        parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
        Type t = type_default_int();
        if (ed) t.enum_id = (int)(ed - p->tu->enums.data + 1);
        return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
    }
    /* typedef name — local, pkg.Type, or same-package fallback. */
    if (peek(p)->kind == TK_IDENT) {
        /* Qualified: pkg.Type */
        if (tu_has_import(p->tu, peek(p)->text)
            && p->pos + 2 < p->tokens->len
            && p->tokens->data[p->pos + 1].kind == TK_DOT
            && p->tokens->data[p->pos + 2].kind == TK_IDENT) {
            const char *pkg_name = peek(p)->text;
            const char *type_name = p->tokens->data[p->pos + 2].text;
            SourceLoc loc = peek(p)->loc;
            const Type *alias = resolve_pkg_typedef(p, pkg_name, type_name);
            if (!alias) {
                die_at(loc.file, loc.line, loc.col,
                       "package '%s' has no type '%s'", pkg_name, type_name);
            }
            advance(p); /* pkg */
            advance(p); /* . */
            advance(p); /* Type */
            Type t = type_clone(*alias);
            if (t.kind == TY_STRUCT && t.tag) {
                const StructDef *sd = struct_registry_find(&p->tu->structs, t.tag);
                if (sd) t.width = sd->size;
            }
            parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
            return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
        }
        const Type *alias = find_typedef_with_fallback(p, peek(p)->text);
        if (alias) {
            advance(p);
            Type t = type_clone(*alias);
            /* A typedef to a struct/union is cloned at typedef-creation time,
             * when the struct may have been a forward declaration (size 0).
             * Refresh the width from the current registry so a later full
             * definition is reflected (`typedef struct T T; struct T {…}`). */
            if (t.kind == TY_STRUCT && t.tag) {
                const StructDef *sd = struct_registry_find(&p->tu->structs, t.tag);
                if (sd) t.width = sd->size;
            }
            parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
            return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
        }
    }

    /* _Bool — standalone, takes no signed/unsigned/long modifier. */
    if (peek(p)->kind == TK_KW_BOOL) {
        advance(p);
        parse_trailing_qualifiers(p, &is_const, &is_volatile, &is_restrict, &is_complex, storage_class, &attr_vec);
        Type t = type_make_bool();
        return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
    }

    /* General integer type specifier loop: handles signed/unsigned, char, short, int,
     * long, long long in any valid C permutation, with interleaved qualifiers. */
    int has_type = 0;
    int is_long = 0;
    int is_short = 0;
    int is_int = 0;
    int is_int128 = 0;
    int is_char = 0;
    int is_unsigned = 0;
    int is_signed = 0;

    for (;;) {
        int a_align = 0, a_packed = 0, a_sso = 0, a_vec = 0;
        if (parse_attribute(p, &a_align, &a_packed, &a_sso, &a_vec, NULL)) {
            if (a_vec > 0) attr_vec = a_vec;
            continue;
        }
        TokenKind k = peek(p)->kind;
        if (k == TK_KW_CONST) { is_const = 1; advance(p); }
        else if (k == TK_KW_VOLATILE) { is_volatile = 1; advance(p); }
        else if (k == TK_KW_RESTRICT) {
            const Token *t = peek(p);
            die_at(t->loc.file, t->loc.line, t->loc.col, "invalid use of 'restrict'");
        }
        else if (k == TK_KW_INLINE) { advance(p); }
        else if (k == TK_KW_COMPLEX) { is_complex = 1; advance(p); }
        else if (k == TK_KW_STATIC) {
            if (storage_class) *storage_class = 1;
            advance(p);
        }
        else if (k == TK_KW_EXTERN) {
            if (storage_class) *storage_class = 2;
            advance(p);
        }
        else if (k == TK_KW_SIGNED) { is_signed = 1; has_type = 1; advance(p); }
        else if (k == TK_KW_UNSIGNED) { is_unsigned = 1; has_type = 1; advance(p); }
        else if (k == TK_KW_CHAR) { is_char = 1; has_type = 1; advance(p); }
        else if (k == TK_KW_SHORT) { is_short = 1; has_type = 1; advance(p); }
        else if (k == TK_KW_INT) { is_int = 1; has_type = 1; advance(p); }
        else if (k == TK_KW_LONG) { is_long++; has_type = 1; advance(p); }
        else if (k == TK_IDENT && (strcmp(peek(p)->text, "__int128") == 0 || strcmp(peek(p)->text, "__int128_t") == 0 || strcmp(peek(p)->text, "__uint128_t") == 0)) {
            if (strcmp(peek(p)->text, "__uint128_t") == 0) is_unsigned = 1;
            is_int128 = 1; has_type = 1; advance(p);
        }
        else if (k == TK_IDENT
                 && (strcmp(peek(p)->text, "register") == 0
                     || strcmp(peek(p)->text, "auto") == 0)) {
            advance(p);
        }
        else break;
    }

    if (!has_type) {
        if (is_complex) {
            /* Bare _Complex defaults to _Complex double */
            Type t = get_or_create_complex_type(p, type_make_float(8));
            return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
        }
        const Token *t = peek(p);
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "expected type but got '%s'", t->text);
    }

    int width = 4;
    if (is_int128) width = 16;
    else if (is_char) width = 1;
    else if (is_short) width = 2;
    else if (is_long >= 1) width = 8;
    else width = 4;

    (void)is_int;
    (void)is_signed;

    Type t = type_make_int(width, is_unsigned);
    return finish_specifiers(t, is_const, is_volatile, is_restrict, is_complex, attr_vec, p);
}

static Type parse_specifiers(Parser *p) {
    return parse_specifiers_full(p, NULL);
}

/* ------------------------------------------------------------------ */
/* Param list (shared by function declarator suffix + param parsing)   */
/* ------------------------------------------------------------------ */

static Type parse_type(Parser *p, char **name_out);  /* forward */

/* Build a function type from a base return type and a ParamArray of params.
 * Consumes (frees) the ParamArray.  type_make_func deep-clones the param
 * types we hand it, so the originals stay owned by the ParamArray and are
 * freed by param_array_free below.  The ptys array is a shallow list of
 * *pointers* into the ParamArray — its elements must NOT be type_free'd. */

static void parse_struct_body(Parser *p, StructDef *sd) {
    /* Assumes peek == '{'.  Parse members until '}'.  Each member is a
     * comma-separated list of declarators sharing one base type, e.g.
     * `int a, b;` or `Expr *l, r;` (multi-declarators in struct bodies).
     *
     * NOTE: `sd` is a pointer into the TU's StructRegistry, which reallocs
     * when a nested anonymous struct/union member is defined inline.  We
     * therefore keep the tag and re-fetch `sd` after any operation that may
     * add to the registry (parse_specifiers / parse_declarator). */
    const char *tag = sd->tag;
    advance(p);  /* consume '{' */
    while (peek(p)->kind != TK_RBRACE) {
        int base_align = 0, base_packed = 0, base_sso = 0, base_vec = 0;
        while (parse_attribute(p, &base_align, &base_packed, &base_sso, &base_vec, NULL)) {}
        Type base = parse_specifiers(p);
        while (parse_attribute(p, &base_align, &base_packed, &base_sso, &base_vec, NULL)) {}
        /* Re-fetch sd: parsing `base` may have defined a nested struct. */
        sd = struct_registry_find(&p->tu->structs, tag);
        /* parse_specifiers may have consumed an inline struct/union definition
         * body (`struct Tag { ... }` / `union Tag { ... }`).  If the next token
         * is ';', this is a standalone type definition, not a member declaration
         * — consume the ';' and continue to the next member. */
        if (peek(p)->kind == TK_SEMICOLON) {
            /* C11 anonymous struct/union member: `struct { ... };` inside
             * another struct, with no field name.  Tagged nested definitions
             * (`struct S { ... };`) are type-only and are skipped. */
            if (base.kind == TY_STRUCT && base.tag
                && strncmp(base.tag, "__anon_", 7) == 0) {
                sd = struct_registry_find(&p->tu->structs, tag);
                if (sd)
                    struct_def_push_member_aligned(sd, "", type_clone(base), -1, base_align);
            }
            advance(p);
            type_free(&base);
            continue;
        }
        for (;;) {
            int align = base_align, packed = base_packed, sso = base_sso, vec = base_vec;
            while (parse_attribute(p, &align, &packed, &sso, &vec, NULL)) {}
            char *mname = NULL;
            Type mty = parse_declarator(p, type_clone(base), &mname);
            while (parse_attribute(p, &align, &packed, &sso, &vec, NULL)) {}
            sd = struct_registry_find(&p->tu->structs, tag);
            if (!mname) {
                if (peek(p)->kind == TK_COLON) {
                    mname = xstrdup("");
                } else {
                    const Token *mn = peek(p);
                    if (!is_name_token(mn->kind)) {
                        die_at(mn->loc.file, mn->loc.line, mn->loc.col,
                               "expected member name but got '%s'", mn->text);
                    }
                    mname = xstrdup(mn->text);
                    advance(p);
                }
            }
            int bit_width = -1;
            if (peek(p)->kind == TK_COLON) {
                /* Bitfield `name : N;` or unnamed `: 0`. */
                advance(p);
                const Token *w = peek(p);
                if (w->kind != TK_INT_LITERAL) {
                    die_at(w->loc.file, w->loc.line, w->loc.col,
                           "expected bitfield width but got '%s'", w->text);
                }
                bit_width = int_literal_value(w->text);
                advance(p);
            }
            struct_def_push_member_aligned(sd, mname, mty, bit_width, align);
            free(mname);
            if (peek(p)->kind == TK_COMMA) {
                /* More declarators sharing this base type (`int a, b`). */
                advance(p);
                continue;
            }
            expect_kind(p, TK_SEMICOLON, "';'");
            break;
        }
        type_free(&base);
    }
    expect_kind(p, TK_RBRACE, "'}'");
    int align = 0, packed = 0, sso = 0, vec = 0;
    while (parse_attribute(p, &align, &packed, &sso, &vec, NULL)) {}
    if (align > sd->align) sd->align = align;
    if (packed) {
        sd->is_packed = 1;
        sd->align = 1;
        /* Rebuild offsets with packed alignment, preserving bitfields. */
        int n = sd->num_members;
        StructMember *old = malloc((size_t)n * sizeof(StructMember));
        if (!old) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        memcpy(old, sd->members, (size_t)n * sizeof(StructMember));
        free(sd->members);
        sd->members = NULL;
        sd->num_members = 0;
        sd->cap_members = 0;
        sd->size = 0;
        sd->bf_unit_type = 0;
        sd->bf_unit_used = 0;
        sd->bf_unit_offset = 0;
        for (int i = 0; i < n; i++) {
            struct_def_push_member(sd, old[i].name, old[i].type, old[i].bit_width);
            free(old[i].name);
            type_free(&old[i].type);
        }
        free(old);
    }
    if (sso == 1) struct_def_apply_sso(sd, 1);
    else if (sso == 2) struct_def_apply_sso(sd, 0);
    /* Finalize the struct's total size (round up to natural alignment) now
     * that all members are known.  This must run before fixup so that
     * self-referential pointer members see the final aligned size. */
    struct_def_finish(sd);
    /* Fix up self-referential struct types: during parsing, member types like
     * `struct Type *` were cloned when the struct was still incomplete, so
     * their pointee->width is stale.  Re-fetch sd and correct all widths. */
    sd = struct_registry_find(&p->tu->structs, tag);
    if (sd) struct_def_fixup_self_types(sd);
}

static void parse_enum_body(Parser *p, EnumDef *ed) {
    /* Assumes peek == '{'.  Parse constants until '}'. */
    advance(p);  /* consume '{' */
    while (peek(p)->kind != TK_RBRACE) {
        const Token *cn = peek(p);
        if (cn->kind != TK_IDENT) {
            die_at(cn->loc.file, cn->loc.line, cn->loc.col,
                   "expected enum constant name but got '%s'", cn->text);
        }
        advance(p);
        int has_value = 0, value = 0;
        if (peek(p)->kind == TK_ASSIGN) {
            advance(p);
            Expr *e = parse_ternary(p);
            long long val = 0;
            if (fold_const_int(e, &val)) {
                has_value = 1; value = (int)val;
            } else if (e->kind == EX_VAR) {
                const EnumConstant *ec =
                    enum_registry_find_constant(&p->tu->enums, e->u.var.name);
                if (!ec) {
                    die_at(cn->loc.file, cn->loc.line, cn->loc.col,
                           "enum value '%s' is not a constant", e->u.var.name);
                }
                has_value = 1; value = ec->value;
            } else {
                die_at(cn->loc.file, cn->loc.line, cn->loc.col,
                       "expected integer value for enum constant '%s'", cn->text);
            }
            expr_free(e);
        }
        enum_def_push_constant(ed, cn->text, has_value, value, cn->loc);
        if (peek(p)->kind == TK_COMMA) advance(p);
    }
    expect_kind(p, TK_RBRACE, "'}'");
}

static Type make_func_type(Type ret, ParamArray *params, int is_variadic) {
    /* Build a shallow array of pointers into the ParamArray's owned types.
     * type_make_func_var deep-clones them, so the originals stay owned by the
     * ParamArray and are freed by param_array_free below. */
    Type **ptys = NULL;
    if (params->len > 0) {
        ptys = malloc(params->len * sizeof(Type *));
        if (!ptys) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        for (size_t i = 0; i < params->len; i++)
            ptys[i] = &params->data[i].type;
    }
    Type t = type_make_func_var(ret, ptys, (int)params->len, is_variadic);
    free(ptys);
    param_array_free(params);
    return t;
}

/* Parse a parameter list: (void) means empty, else type declarator pairs.
 * Returns the collected params.  Tolerates a trailing comma. */
static ParamArray parse_param_list(Parser *p, int *is_variadic) {
    if (is_variadic) *is_variadic = 0;
    ParamArray params;
    param_array_init(&params);
    if (peek(p)->kind == TK_KW_VOID
        && p->tokens->data[p->pos + 1].kind == TK_RPAREN) {
        advance(p);  /* consume `void` */
        return params;  /* empty */
    }
    if (peek(p)->kind != TK_RPAREN) {
        for (;;) {
            while (skip_attribute(p)) {}
            if (!is_type_start(p, p->pos)) {
                const Token *t = peek(p);
                die_at(t->loc.file, t->loc.line, t->loc.col,
                       "expected type for parameter but got '%s'", t->text);
            }
            char *pname_in = NULL;
            Type pty = parse_type(p, &pname_in);
            while (skip_attribute(p)) {}
            if (pty.kind == TY_ARRAY && pty.elem_type) {
                Type ptr = type_make_ptr(*pty.elem_type);
                ptr.vla_dim = pty.vla_dim;
                pty.vla_dim = NULL;
                type_free(&pty);
                pty = ptr;
            }
            if (!pname_in) {
                pname_in = xstrdup("");
            }
            param_array_push(&params, pname_in, pty, peek(p)->loc);
            free(pname_in);
            while (skip_attribute(p)) {}
            if (peek(p)->kind == TK_COMMA) {
                advance(p);
                /* Variadic tail of a function type. */
                if (peek(p)->kind == TK_ELLIPSIS) {
                    advance(p);
                    if (is_variadic) *is_variadic = 1;
                    break;
                }
                continue;
            }
            break;
        }
        if (params.len > 16) {
            die_at(peek(p)->loc.file, peek(p)->loc.line, peek(p)->loc.col,
                   "more than 16 parameters not supported");
        }
    }
    return params;
}

/* ------------------------------------------------------------------ */
/* Declarator (right-left rule)                                        */
/* ------------------------------------------------------------------ */

/* Parse the content inside a grouping paren `(*)`.  Collects prefix `*`
 * count, the name, and an optional array `[N]` suffix.  The collected
 * modifiers wrap the group's "core" type (determined by the postfix that
 * follows the group).  Returns the name (owned) via *name_out (NULL if
 * abstract). */
static Expr *parse_expr(Parser *p); /* forward declaration */

/* Parse an array dimension size: an int literal, enum constant,
 * full constant expression, or dynamic expression for VLA.
 * Returns the constant value (0 if absent, -1 if VLA with *dim_expr set). */
static long long parse_array_size_ext(Parser *p, Expr **dim_expr) {
    if (dim_expr) *dim_expr = NULL;
    if (peek(p)->kind == TK_RBRACKET) {
        return 0;
    }
    Expr *e = parse_expr(p);
    long long val = 0;
    if (fold_const_int(e, &val)) {
        expr_free(e);
        return val >= 0 ? val : 1;
    }
    if (e->kind == EX_VAR) {
        const EnumConstant *ec =
            enum_registry_find_constant(&p->tu->enums, e->u.var.name);
        if (ec) {
            expr_free(e);
            return ec->value > 0 ? ec->value : 1;
        }
    }
    if (dim_expr) {
        *dim_expr = e;
        return -1;
    }
    expr_free(e);
    return 1;
}

/* Parse a C declarator given the base (specifier) type.  Implements the
 * right-left rule: prefix `*` wraps the result, grouping parens change
 * precedence, postfix `[N]` (array) and `(params)` (function) bind tightest.
 *
 * Returns the declared type.  *name_out receives the declared name (owned,
 * freed by caller) or NULL for abstract declarators (casts/sizeof). */
/* Apply a pointer wrap with optional qualifiers to `t` (in place). */
static Type ptr_wrap(Type t, int is_const, int is_volatile, int is_restrict) {
    Type w = type_make_ptr(t);
    w.is_const = is_const;
    w.is_volatile = is_volatile;
    w.is_restrict = is_restrict;
    return w;
}

static Type parse_declarator(Parser *p, Type base, char **name_out) {
    /* Prefix: zero-or-more `*`, each optionally followed by qualifiers
     * (`* const`, `*volatile`, `* const restrict`, ...).  Record the qualifier
     * set per pointer level so `Type * const *p` parses as pointer-to-(const-pointer).
     * The leftmost `*` is the innermost pointer (applied first). */
    int pre_align = 0, pre_packed = 0, pre_sso = 0, pre_vec = 0;
    while (parse_attribute(p, &pre_align, &pre_packed, &pre_sso, &pre_vec, NULL)) {}
    if (g_parsed_mode_size > 0) {
        if (base.kind == TY_INT || base.kind == TY_FLOAT) base.width = g_parsed_mode_size;
        g_parsed_mode_size = 0;
    }
    if (pre_vec > 0 && !base.is_vector) base = type_make_vector(base, pre_vec);
    enum { MAX_PTRS = 8 };
    int ptr_const[MAX_PTRS], ptr_volatile[MAX_PTRS], ptr_restrict[MAX_PTRS];
    int ptrs = 0;
    for (;;) {
        while (parse_attribute(p, &pre_align, &pre_packed, &pre_sso, &pre_vec, NULL)) {}
        if (g_parsed_mode_size > 0) {
            if (base.kind == TY_INT || base.kind == TY_FLOAT) base.width = g_parsed_mode_size;
            g_parsed_mode_size = 0;
        }
        if (pre_vec > 0 && !base.is_vector) base = type_make_vector(base, pre_vec);
        if (peek(p)->kind != TK_STAR) break;
        advance(p);
        int c = 0, v = 0, r = 0;
        while (peek(p)->kind == TK_KW_CONST
               || peek(p)->kind == TK_KW_VOLATILE
               || peek(p)->kind == TK_KW_RESTRICT
               || skip_attribute(p)) {
            if (peek(p)->kind == TK_KW_CONST) { c = 1; advance(p); }
            else if (peek(p)->kind == TK_KW_VOLATILE) { v = 1; advance(p); }
            else if (peek(p)->kind == TK_KW_RESTRICT) { r = 1; advance(p); }
        }
        if (ptrs >= MAX_PTRS) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                     peek(p)->loc.col, "too many pointer levels");
        ptr_const[ptrs] = c; ptr_volatile[ptrs] = v; ptr_restrict[ptrs] = r;
        ptrs++;
    }
    while (parse_attribute(p, &pre_align, &pre_packed, &pre_sso, &pre_vec, NULL)) {}
    if (g_parsed_mode_size > 0) {
        if (base.kind == TY_INT || base.kind == TY_FLOAT) base.width = g_parsed_mode_size;
        g_parsed_mode_size = 0;
    }
    if (pre_vec > 0 && !base.is_vector) base = type_make_vector(base, pre_vec);

    Type t;
    if (peek(p)->kind == TK_LPAREN && (p->pos + 1 < p->tokens->len &&
        (p->tokens->data[p->pos + 1].kind == TK_STAR ||
         p->tokens->data[p->pos + 1].kind == TK_LPAREN ||
         p->tokens->data[p->pos + 1].kind == TK_LBRACKET ||
         (p->tokens->data[p->pos + 1].kind == TK_IDENT
          && !is_type_start(p, p->pos + 1))))) {
        /* Grouping: '(' declarator ')' */
        advance(p);  /* '(' */
        size_t group_start = p->pos;
        int paren_depth = 1;
        while (p->pos < p->tokens->len && paren_depth > 0) {
            if (p->tokens->data[p->pos].kind == TK_LPAREN) paren_depth++;
            else if (p->tokens->data[p->pos].kind == TK_RPAREN) {
                paren_depth--;
                if (paren_depth == 0) break;
            }
            p->pos++;
        }
        if (paren_depth != 0) {
            die_at(peek(p)->loc.file, peek(p)->loc.line, peek(p)->loc.col, "unmatched '('");
        }
        advance(p);  /* consume ')' */
        while (skip_attribute(p)) {}

        /* Parse postfixes that follow the group ')' */
        Type ret = base;
        for (int i = 0; i < ptrs; i++)
            ret = ptr_wrap(ret, ptr_const[i], ptr_volatile[i], ptr_restrict[i]);
        ptrs = 0;

        Type outer_t = ret;
        long long dims[8];
        int ndims = 0;
        Expr *vla_dims[8];
        memset(vla_dims, 0, sizeof(vla_dims));
        while (peek(p)->kind == TK_LBRACKET || peek(p)->kind == TK_LPAREN) {
            if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                Expr *vla_e = NULL;
                long long len = parse_array_size_ext(p, &vla_e);
                expect_kind(p, TK_RBRACKET, "']'");
                if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                       peek(p)->loc.col, "too many array dimensions");
                dims[ndims] = len;
                vla_dims[ndims] = vla_e;
                ndims++;
            } else {
                advance(p);
                int is_var = 0;
                ParamArray params = parse_param_list(p, &is_var);
                expect_kind(p, TK_RPAREN, "')'");
                outer_t = make_func_type(ret, &params, is_var);
                break;
            }
        }
        for (int i = ndims - 1; i >= 0; i--) {
            Type wrapped = (dims[i] == -1 && vla_dims[i]) ?
                           type_make_vla(outer_t, vla_dims[i]) :
                           type_make_array(outer_t, dims[i]);
            type_free(&outer_t);
            outer_t = wrapped;
        }

        size_t saved_pos = p->pos;
        p->pos = group_start;
        t = parse_declarator(p, outer_t, name_out);
        p->pos = saved_pos;
    } else if (peek(p)->kind == TK_IDENT) {
        *name_out = xstrdup(peek(p)->text);
        advance(p);
        int attr_align = 0, attr_packed = 0, attr_sso = 0, attr_vec = 0;
        while (parse_attribute(p, &attr_align, &attr_packed, &attr_sso, &attr_vec, NULL)) {}
        if (g_parsed_mode_size > 0) {
            if (base.kind == TY_INT || base.kind == TY_FLOAT) base.width = g_parsed_mode_size;
            g_parsed_mode_size = 0;
        }
        t = base;
        if (attr_vec > 0 && !t.is_vector) t = type_make_vector(t, attr_vec);
        /* Apply prefix pointers FIRST (they wrap the base type, innermost).
         * `int *rows[2]` → ptr(int) then array(2, ptr(int)).  Applying them
         * after the array (at function scope) would wrongly yield
         * ptr(array(2,int)) i.e. `int(*)[2]`. */
        for (int i = 0; i < ptrs; i++)
            t = ptr_wrap(t, ptr_const[i], ptr_volatile[i], ptr_restrict[i]);
        ptrs = 0; /* applied here — prevent double-apply at function scope */
        /* Postfix: array [] or function ().  Collect dims, wrap right-to-left. */
        long long dims[8];
        int ndims = 0;
        Expr *vla_dims[8];
        memset(vla_dims, 0, sizeof(vla_dims));
        while (peek(p)->kind == TK_LBRACKET || peek(p)->kind == TK_LPAREN) {
            if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                Expr *vla_e = NULL;
                long long len = parse_array_size_ext(p, &vla_e);
                expect_kind(p, TK_RBRACKET, "']'");
                if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                       peek(p)->loc.col, "too many array dimensions");
                dims[ndims] = len;
                vla_dims[ndims] = vla_e;
                ndims++;
            } else {
                advance(p);
                int is_var = 0;
                ParamArray params = parse_param_list(p, &is_var);
                expect_kind(p, TK_RPAREN, "')'");
                t = make_func_type(t, &params, is_var);
                break;
            }
        }
        for (int i = ndims - 1; i >= 0; i--) {
            Type wrapped = (dims[i] == -1 && vla_dims[i]) ?
                           type_make_vla(t, vla_dims[i]) :
                           type_make_array(t, dims[i]);
            type_free(&t);
            t = wrapped;
        }
    } else {
        /* Abstract declarator (no name) — for casts/sizeof. */
        *name_out = NULL;
        t = base;
        long long dims[8];
        int ndims = 0;
        Expr *vla_dims[8];
        memset(vla_dims, 0, sizeof(vla_dims));
        while (peek(p)->kind == TK_LBRACKET || peek(p)->kind == TK_LPAREN) {
            if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                Expr *vla_e = NULL;
                long long len = parse_array_size_ext(p, &vla_e);
                expect_kind(p, TK_RBRACKET, "']'");
                if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                       peek(p)->loc.col, "too many array dimensions");
                dims[ndims] = len;
                vla_dims[ndims] = vla_e;
                ndims++;
            } else {
                advance(p);
                int is_var = 0;
                ParamArray params = parse_param_list(p, &is_var);
                expect_kind(p, TK_RPAREN, "')'");
                t = make_func_type(t, &params, is_var);
                break;
            }
        }
        for (int i = ndims - 1; i >= 0; i--) {
            Type wrapped = (dims[i] == -1 && vla_dims[i]) ?
                           type_make_vla(t, vla_dims[i]) :
                           type_make_array(t, dims[i]);
            type_free(&t);
            t = wrapped;
        }
        for (int i = 0; i < ptrs; i++)
            t = ptr_wrap(t, ptr_const[i], ptr_volatile[i], ptr_restrict[i]);
        ptrs = 0;
    }
    int attr_align = 0, attr_packed = 0, attr_sso = 0, attr_vec = 0;
    while (parse_attribute(p, &attr_align, &attr_packed, &attr_sso, &attr_vec, NULL)) {}
    if (g_parsed_mode_size > 0) {
        if (t.kind == TY_INT || t.kind == TY_FLOAT) t.width = g_parsed_mode_size;
        g_parsed_mode_size = 0;
    }
    if (attr_vec > 0 && !t.is_vector) t = type_make_vector(t, attr_vec);
    return t;
}

/* Parse a full type: specifiers + declarator.  This replaces the old
 * `parse_type` for all call sites that need a named type.  *name_out receives
 * the declared name (owned, freed by caller) or NULL for abstract declarators. */
static Type parse_type(Parser *p, char **name_out) {
    Type base = parse_specifiers(p);
    return parse_declarator(p, base, name_out);
}

static Type parse_type_name(Parser *p);

static Type parse_return_type(Parser *p) {
    Type base = parse_specifiers(p);
    Type t = base;
    while (peek(p)->kind == TK_STAR) {
        advance(p);
        for (;;) {
            if (skip_attribute(p)) continue;
            if (peek(p)->kind == TK_KW_CONST || peek(p)->kind == TK_KW_VOLATILE || peek(p)->kind == TK_KW_RESTRICT) {
                advance(p);
                continue;
            }
            break;
        }
        Type w = type_make_ptr(t);
        type_free(&t);
        t = w;
    }
    return t;
}

static Type parse_type_abstract(Parser *p) {
    return parse_type_name(p);
}

/* Parse a type-name (specifiers + abstract declarator): the type in a
 * compound literal `(Type){ ... }`.  Supports a `*` chain and/or array
 * dimensions `[int]` / `[]`, but NOT function types (illegal in a compound
 * literal).  Arrays wrap right-to-left so `int[3][2]` → array(3, array(2)). */
static Type parse_type_name(Parser *p) {
    /* Specifiers plus an abstract declarator: pointers, arrays, grouping
     * `void (*)()`, and function types. */
    Type base = parse_specifiers(p);
    char *name = NULL;
    Type t = parse_declarator(p, base, &name);
    free(name);
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
static Expr *parse_comma(Parser *p);
static Expr *parse_assign(Parser *p);
static Expr *parse_ternary(Parser *p);
static Expr *parse_or(Parser *p);
static Expr *parse_and(Parser *p);
static Expr *parse_bitor(Parser *p);
static Expr *parse_xor(Parser *p);
static Expr *parse_bitand(Parser *p);
static Expr *parse_equality(Parser *p);
static Expr *parse_relational(Parser *p);
static Expr *parse_shift(Parser *p);
static Expr *parse_add(Parser *p);
static Expr *parse_mul(Parser *p);
static Expr *parse_unary(Parser *p);
static Expr *parse_postfix(Parser *p, Expr *lhs);
static Expr *parse_primary(Parser *p);
static Expr *parse_init_list(Parser *p);

static Expr *parse_expr(Parser *p) {
    return parse_comma(p);
}

/* comma-expr = assign-expr { "," assign-expr }  -- left associative, lowest
 * precedence. The result is the rightmost operand. */
static Expr *parse_comma(Parser *p) {
    Expr *lhs = parse_assign(p);
    while (peek(p)->kind == TK_COMMA) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_assign(p);
        lhs = expr_new_comma(lhs, rhs, loc);
    }
    return lhs;
}

/* Map a compound-assignment token to its underlying binary op.
 * Returns 1 if `k` is a compound-assignment token (op is filled), else 0. */
static int compound_op(TokenKind k, BinOp *op) {
    switch (k) {
    case TK_PLUS_EQ:    *op = BOP_ADD;     break;
    case TK_MINUS_EQ:   *op = BOP_SUB;     break;
    case TK_STAR_EQ:    *op = BOP_MUL;     break;
    case TK_SLASH_EQ:   *op = BOP_DIV;     break;
    case TK_PERCENT_EQ: *op = BOP_MOD;     break;
    case TK_AMP_EQ:     *op = BOP_BITAND;  break;
    case TK_BITOR_EQ:   *op = BOP_BITOR;   break;
    case TK_XOR_EQ:     *op = BOP_BITXOR;  break;
    case TK_SHL_EQ:     *op = BOP_SHL;     break;
    case TK_SHR_EQ:     *op = BOP_SHR;     break;
    default: return 0;
    }
    return 1;
}

/* assign-expr = ternary-expr [ ( "=" | op "=" ) assign-expr ]  -- right
 * associative. Compound assignment (e.g. a += b) is right-associative too:
 * a += b += c  ==  a += (b += c). */
static Expr *parse_assign(Parser *p) {
    Expr *lhs = parse_ternary(p);
    TokenKind k = peek(p)->kind;
    if (k == TK_ASSIGN) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_assign(p);   /* recursive → right associative */
        return expr_new_assign(lhs, rhs, loc);
    }
    BinOp op;
    if (compound_op(k, &op)) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_assign(p);
        return expr_new_compound_assign(lhs, rhs, op, loc);
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

/* and-expr = bitor-expr { "&&" bitor-expr }  -- left associative */
static Expr *parse_and(Parser *p) {
    Expr *lhs = parse_bitor(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_ANDAND) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_bitor(p);
        lhs = expr_new_binop(BOP_AND, lhs, rhs, loc);
    }
    return lhs;
}

/* bitor-expr = xor-expr { "|" xor-expr }  -- left associative */
static Expr *parse_bitor(Parser *p) {
    Expr *lhs = parse_xor(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_BITOR) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_xor(p);
        lhs = expr_new_binop(BOP_BITOR, lhs, rhs, loc);
    }
    return lhs;
}

/* xor-expr = bitand-expr { "^" bitand-expr }  -- left associative */
static Expr *parse_xor(Parser *p) {
    Expr *lhs = parse_bitand(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_XOR) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_bitand(p);
        lhs = expr_new_binop(BOP_BITXOR, lhs, rhs, loc);
    }
    return lhs;
}

/* bitand-expr = equality-expr { "&" equality-expr }  -- left associative */
static Expr *parse_bitand(Parser *p) {
    Expr *lhs = parse_equality(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_AMP) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_equality(p);
        lhs = expr_new_binop(BOP_BITAND, lhs, rhs, loc);
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

/* relational-expr = shift-expr { ("<" | "<=" | ">" | ">=") shift-expr } */
static Expr *parse_relational(Parser *p) {
    Expr *lhs = parse_shift(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_LT && k != TK_LE && k != TK_GT && k != TK_GE) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_shift(p);
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

/* shift-expr = add-expr { ("<<" | ">>") add-expr }  -- left associative */
static Expr *parse_shift(Parser *p) {
    Expr *lhs = parse_add(p);
    for (;;) {
        TokenKind k = peek(p)->kind;
        if (k != TK_SHL && k != TK_SHR) break;
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *rhs = parse_add(p);
        BinOp op = (k == TK_SHL) ? BOP_SHL : BOP_SHR;
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

static int types_compatible_unqual(const Type *a, const Type *b) {
    if (!a || !b) return 0;
    if (a->is_vector != b->is_vector) return 0;
    if (a->is_vector) {
        return a->width == b->width && types_compatible_unqual(a->elem_type, b->elem_type);
    }
    if (a->kind != b->kind) return 0;
    if (a->kind == TY_INT) {
        if (a->enum_id != 0 && b->enum_id != 0 && a->enum_id != b->enum_id) return 0;
        return (a->width == b->width && a->is_unsigned == b->is_unsigned);
    }
    if (a->kind == TY_FLOAT) {
        return (a->width == b->width);
    }
    if (a->kind == TY_PTR) {
        if (!a->pointee || !b->pointee) return 0;
        if (a->pointee->is_const != b->pointee->is_const ||
            a->pointee->is_volatile != b->pointee->is_volatile) return 0;
        return types_compatible_unqual(a->pointee, b->pointee);
    }
    if (a->kind == TY_ARRAY) {
        if (a->length != b->length && a->length != 0 && b->length != 0) return 0;
        if (!a->elem_type || !b->elem_type) return 0;
        if (a->elem_type->is_const != b->elem_type->is_const ||
            a->elem_type->is_volatile != b->elem_type->is_volatile) return 0;
        return types_compatible_unqual(a->elem_type, b->elem_type);
    }
    if (a->kind == TY_STRUCT) {
        if (a->tag && b->tag) return strcmp(a->tag, b->tag) == 0;
        return 1;
    }
    return 1;
}

/* unary-expr = ("+"|"-"|"&"|"*"|"~") unary-expr | sizeof unary-or-type | primary { postfix } */
static Expr *parse_unary(Parser *p) {
    TokenKind k = peek(p)->kind;
    if (k == TK_KW_REAL) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *op = parse_unary(p);
        return expr_new_member(op, "__real", loc);
    }
    if (k == TK_KW_IMAG) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *op = parse_unary(p);
        return expr_new_member(op, "__imag", loc);
    }
    if (k == TK_PLUS || k == TK_MINUS) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Expr *operand = parse_unary(p);
        UnaryOp op = (k == TK_MINUS) ? UOP_NEG : UOP_POS;
        return expr_new_unary(op, operand, loc);
    }
    if (k == TK_TILDE) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        return expr_new_unary(UOP_BITNOT, parse_unary(p), loc);
    }
    if (k == TK_NOT) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        return expr_new_unary(UOP_NOT, parse_unary(p), loc);
    }
    if (k == TK_INC || k == TK_DEC) {
        SourceLoc loc = peek(p)->loc;
        int is_inc = (k == TK_INC);
        advance(p);
        return expr_new_inc_dec(parse_unary(p), is_inc, 1 /* prefix */, loc);
    }
    if (k == TK_ANDAND) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        const Token *lbl = peek(p);
        if (!is_name_token(lbl->kind)) {
            die_at(lbl->loc.file, lbl->loc.line, lbl->loc.col,
                   "expected label name after '&&' but got '%s'", lbl->text ? lbl->text : "NULL");
        }
        advance(p);
        return expr_new_label_addr(lbl->text, loc);
    }
    if (k == TK_KW_INLINE || (k == TK_IDENT && (strcmp(peek(p)->text, "__extension__") == 0 || strcmp(peek(p)->text, "__extension") == 0))) {
        advance(p);
        return parse_unary(p);
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
        if (peek(p)->kind == TK_LPAREN && is_type_start(p, p->pos + 1)) {
            advance(p);
            Type t = parse_type_abstract(p);
            expect_kind(p, TK_RPAREN, "')'");
            Expr *e = expr_new_sizeof_type(t, loc);
            type_free(&t);
            return e;
        }
        return expr_new_sizeof_expr(parse_unary(p), loc);
    }
    if (k == TK_KW_ALIGNOF) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        /* _Alignof(T) — C standard _Alignof takes a type-name only.
         * __alignof__(expr) / __alignof__(T) — GCC extension also takes expressions. */
        if (peek(p)->kind != TK_LPAREN)
            die_at(peek(p)->loc.file, peek(p)->loc.line, peek(p)->loc.col,
                   "expected '(' after '_Alignof'");
        advance(p);
        if (is_type_start(p, p->pos)) {
            Type t = parse_type_abstract(p);
            expect_kind(p, TK_RPAREN, "')'");
            Expr *e = expr_new_alignof_type(t, loc);
            type_free(&t);
            return e;
        }
        Expr *sub = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        return expr_new_alignof_expr(sub, loc);
    }
    if (k == TK_IDENT && strcmp(peek(p)->text, "__builtin_types_compatible_p") == 0) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        Type t1 = parse_type_abstract(p);
        expect_kind(p, TK_COMMA, "','");
        Type t2 = parse_type_abstract(p);
        expect_kind(p, TK_RPAREN, "')'");
        int compat = types_compatible_unqual(&t1, &t2);
        type_free(&t1);
        type_free(&t2);
        return expr_new_int(compat ? 1 : 0, loc);
    }
    if (k == TK_IDENT && strcmp(peek(p)->text, "__builtin_constant_p") == 0) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        Expr *arg = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        long long v;
        int is_const = fold_const_int(arg, &v) || (arg->kind == EX_STR) || (arg->kind == EX_FLOAT_LIT);
        expr_free(arg);
        return expr_new_int(is_const ? 1 : 0, loc);
    }
    if (k == TK_IDENT && strcmp(peek(p)->text, "__builtin_choose_expr") == 0) {
        /* __builtin_choose_expr(const_expr, expr1, expr2): if const_expr is a
         * compile-time constant, return expr1, else expr2.  Unlike the ternary
         * operator, the unselected branch is discarded at compile time.
         * Use parse_assign (not parse_expr) so commas separate arguments
         * instead of being parsed as the comma operator. */
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        Expr *cond = parse_assign(p);
        expect_kind(p, TK_COMMA, "','");
        Expr *then = parse_assign(p);
        expect_kind(p, TK_COMMA, "','");
        Expr *else_ = parse_assign(p);
        expect_kind(p, TK_RPAREN, "')'");
        long long v;
        int is_const = fold_const_int(cond, &v) || (cond->kind == EX_STR) || (cond->kind == EX_FLOAT_LIT);
        expr_free(cond);
        if (is_const) {
            expr_free(else_);
            return then;
        } else {
            expr_free(then);
            return else_;
        }
    }
    if (k == TK_IDENT && strcmp(peek(p)->text, "__builtin_offsetof") == 0) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        expect_kind(p, TK_LPAREN, "'('");
        Type t = parse_type_abstract(p);
        expect_kind(p, TK_COMMA, "','");
        long long offset = 0;
        Type cur_type = type_clone(t);
        for (;;) {
            if (peek(p)->kind == TK_IDENT) {
                const char *mname = peek(p)->text;
                advance(p);
                if (cur_type.kind == TY_STRUCT && cur_type.tag) {
                    const StructDef *sd = struct_registry_find(&p->tu->structs, cur_type.tag);
                    if (sd) {
                        long long moff = 0;
                        const StructMember *sm = struct_lookup_member(&p->tu->structs, sd, mname, &moff);
                        if (sm) {
                            offset += moff;
                            Type next = type_clone(sm->type);
                            type_free(&cur_type);
                            cur_type = next;
                        } else {
                            die_at(peek(p)->loc.file, peek(p)->loc.line, peek(p)->loc.col,
                                   "no member named '%s'", mname);
                        }
                    }
                }
            } else if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                Expr *idx_expr = parse_expr(p);
                long long idx = 0;
                fold_const_int(idx_expr, &idx);
                expr_free(idx_expr);
                expect_kind(p, TK_RBRACKET, "']'");
                if (cur_type.kind == TY_ARRAY && cur_type.elem_type) {
                    long long esz = type_size(*cur_type.elem_type);
                    offset += idx * esz;
                    Type next = type_clone(*cur_type.elem_type);
                    type_free(&cur_type);
                    cur_type = next;
                }
            } else if (peek(p)->kind == TK_DOT) {
                advance(p);
                continue;
            } else {
                break;
            }
            if (peek(p)->kind == TK_DOT) {
                advance(p);
            } else if (peek(p)->kind != TK_LBRACKET) {
                break;
            }
        }
        expect_kind(p, TK_RPAREN, "')'");
        type_free(&cur_type);
        type_free(&t);
        return expr_new_int(offset, loc);
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
            if (!is_name_token(mn->kind)) {
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
            if (!is_name_token(mn->kind)) {
                die_at(mn->loc.file, mn->loc.line, mn->loc.col,
                       "expected member name after '->'");
            }
            advance(p);
            /* Desugar `p->x` to `(*p).x`. */
            Expr *deref = expr_new_deref(lhs, loc);
            lhs = expr_new_member(deref, mn->text, loc);
            continue;
        }
        if (peek(p)->kind == TK_INC || peek(p)->kind == TK_DEC) {
            /* Postfix ++ / -- : binds tighter than prefix, so handle here. */
            SourceLoc loc = peek(p)->loc;
            int is_inc = (peek(p)->kind == TK_INC);
            advance(p);
            lhs = expr_new_inc_dec(lhs, is_inc, 0 /* postfix */, loc);
            continue;
        }
        if (peek(p)->kind == TK_LPAREN) {
            /* Function call: lhs(arg, ...).  lhs is any expression — a bare
             * `IDENT` (direct call), a deref `(*fp)` (indirect call), an array
             * element `arr[i]`, etc.  Sema decides direct vs indirect. */
            SourceLoc loc = peek(p)->loc;
            advance(p);  /* '(' */
            Expr *call = expr_new_call(lhs, loc);
            /* The `va_arg(list, T)` builtin's second argument is a TYPE, not
             * an expression. Detect it by callee name and parse accordingly. */
            int is_va_arg = (lhs->kind == EX_VAR
                             && (strcmp(lhs->u.var.name, "va_arg") == 0 ||
                                 strcmp(lhs->u.var.name, "__builtin_va_arg") == 0));
            if (peek(p)->kind != TK_RPAREN) {
                for (;;) {
                    /* Arguments are assignment-expressions, NOT
                     * comma-expressions — so a comma here is always an
                     * argument separator, never the comma operator. */
                    Expr *arg = parse_assign(p);
                    expr_call_push_arg(call, arg);
                    if (peek(p)->kind == TK_COMMA) {
                        advance(p);
                        if (is_va_arg && call->u.call.args.len == 1) {
                            /* Second arg of va_arg is the requested type.
                             * va_arg always has exactly two arguments, so
                             * parse the type and end the argument list. */
                            Type t = parse_type_abstract(p);
                            call->va_arg_type = t;
                            break;
                        }
                        continue;
                    }
                    break;
                }
            }
            expect_kind(p, TK_RPAREN, "')'");
            lhs = call;
            continue;
        }
        break;
    }
    return lhs;
}

/* Decode a char-literal token's text (e.g. "'A'" or "'\\n'") to its int value.
 * Caller guarantees text starts and ends with a single quote. */
/* Classify a TK_FLOAT_LITERAL's suffix into a width: f/F -> float (4),
 * l/L -> long double (16), otherwise double (8).  The numeric value is left
 * in the AST's source text and parsed at IR time (strtold preserves the
 * 80-bit precision a double cannot hold). */
static void float_literal_width(const char *text, int *out_width) {
    size_t len = strlen(text);
    *out_width = 8;  /* default: double */
    if (len > 0) {
        char last = text[len - 1];
        if (last == 'f' || last == 'F')
            *out_width = 4;
        else if (last == 'l' || last == 'L')
            *out_width = 16;
    }
}

/* Decode an integer-literal token's text.  strtol(base=0) auto-detects the
 * radix from the prefix: `0x`/`0X` = hex, leading `0` = octal, otherwise
 * decimal — matching C's integer-literal rules.  Replaces raw atoi so hex
 * (`0xFF`) and octal (`077`) literals work.  Returns the value as int. */
static int int_literal_value(const char *text) {
    return (int)strtol(text, NULL, 0);
}

static int u128_fits(unsigned long long lo, unsigned long long hi,
                    int bits, int is_unsigned) {
    if (bits >= 128) {
        if (is_unsigned) return 1;
        return (hi & 0x8000000000000000ULL) == 0;
    }
    if (hi != 0) return 0;
    if (bits >= 64) {
        if (is_unsigned) return 1;
        return (lo & 0x8000000000000000ULL) == 0;
    }
    if (bits <= 0) return lo == 0 && hi == 0;
    unsigned long long max = is_unsigned
        ? ((1ULL << bits) - 1ULL)
        : ((1ULL << (bits - 1)) - 1ULL);
    return lo <= max;
}

/* (hi:lo) * base + digit.  base is 8, 10, or 16.  Returns 1 on 128-bit
 * overflow.  Schoolbook 64-bit multiply-split-into-32-bit halves. */
static int u128_mul_add(unsigned long long *lo, unsigned long long *hi,
                        unsigned base, unsigned digit) {
    unsigned long long a = *lo >> 32, b = *lo & 0xffffffffULL;
    unsigned long long pa = a * base, pb = b * base;
    unsigned long long mid = (pb >> 32) + (pa & 0xffffffffULL);
    unsigned long long new_lo = (pb & 0xffffffffULL) | ((mid & 0xffffffffULL) << 32);
    unsigned long long carry = (pa >> 32) + (mid >> 32);
    a = *hi >> 32; b = *hi & 0xffffffffULL;
    pa = a * base; pb = b * base;
    mid = (pb >> 32) + (pa & 0xffffffffULL);
    unsigned long long loh = (pb & 0xffffffffULL) | ((mid & 0xffffffffULL) << 32);
    unsigned long long hih = (pa >> 32) + (mid >> 32);
    if (hih) return 1;
    unsigned long long new_hi = loh + carry;
    if (new_hi < loh) return 1;
    unsigned long long t = new_lo + digit;
    if (t < new_lo) {
        new_hi++;
        if (new_hi == 0) return 1;
    }
    *lo = t;
    *hi = new_hi;
    return 0;
}

/* Parse and validate the integer suffix.  Writes the body end (past the
 * last suffix char) to *body_end, and reports whether `u` / `l` appear.
 * Rejects malformed suffixes like `123LLL`, `123lul`, `123UU`.
 * Accepts `U`, `L`, `LL`, `UL`, `LU`, `ULL`, `LLU`, and any case variant. */
static int parse_int_suffix(const char *text, size_t n, size_t *body_end,
                            int *suffix_u, int *suffix_l) {
    size_t pos = n;
    int u = 0, l = 0;
    while (pos > 0) {
        char c = text[pos - 1];
        if (c == 'u' || c == 'U') {
            if (u) return 0;            /* duplicate 'u' */
            u = 1; pos--;
        } else if (c == 'l' || c == 'L') {
            if (l >= 2) return 0;       /* more than two 'l' */
            l++; pos--;
        } else break;
    }
    *body_end = pos;
    *suffix_u = u;
    *suffix_l = (l > 0) ? 1 : 0;
    return 1;
}

/* Decode an integer literal the way GCC does on LP64 (gnu99).
 *
 * Magnitude is accumulated as unsigned 128-bit (hi:lo).  The type ladder
 * follows C §6.4.4.1 with GCC's extra ranks:
 *
 *   Decimal:  int → long → long long → __int128 → unsigned __int128.
 *   Hex/octal:  int → unsigned int → long → unsigned long →
 *               long long → unsigned long long.  (Never __int128.)
 *
 * A constant that does not fit the widest rank for its radix is truncated
 * to low 64 bits (and a warning issued) — matching GCC's behaviour.
 *
 * Deriving the type here (rather than defaulting every literal to int) is what
 * makes `1UL << 63` and `0xFFu << 24 >> 24` come out right: the shift result
 * type is the promoted type of the left operand, so a mistyped literal
 * silently truncates the whole expression. */
static void int_literal_typed(const char *text, SourceLoc loc,
                              unsigned long long *out_lo, unsigned long long *out_hi,
                              int *out_width, int *out_unsigned) {
    size_t n = strlen(text);
    int suffix_u = 0, suffix_l = 0;
    size_t body = n;

    if (!parse_int_suffix(text, n, &body, &suffix_u, &suffix_l)) {
        die_at(loc.file, loc.line, loc.col, "invalid suffix on integer constant");
        /* Fall through with empty suffix so typing still completes. */
        suffix_u = 0; suffix_l = 0;
        body = n;
    }

    int base = 10;
    size_t i = 0;
    int decimal = 1;
    if (body >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        base = 16; i = 2; decimal = 0;
    } else if (body >= 2 && text[0] == '0') {
        base = 8; i = 1; decimal = 0;
        /* A leading 0 with no further digits is just `0`. */
        if (i >= body) { base = 10; i = 0; decimal = 1; }
    }
    if (i >= body && base != 10) {
        die_at(loc.file, loc.line, loc.col, "integer literal has no digits");
    }
    unsigned long long lo = 0, hi = 0;
    for (; i < body; i++) {
        unsigned char c = (unsigned char)text[i];
        unsigned d;
        if (c == '\'') continue; /* GCC digit separator */
        if (c >= '0' && c <= '9') d = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') d = (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = (unsigned)(c - 'A' + 10);
        else {
            die_at(loc.file, loc.line, loc.col,
                   "invalid digit in integer literal");
            return;
        }
        if (d >= (unsigned)base) {
            die_at(loc.file, loc.line, loc.col,
                   "invalid digit in integer literal");
            return;
        }
        if (u128_mul_add(&lo, &hi, (unsigned)base, d)) {
            /* 128-bit overflow: warn and truncate to low 64 bits (GCC-like). */
            fprintf(stderr, "%s:%d:%d: warning: integer constant is too large "
                    "for its type\n", loc.file, loc.line, loc.col);
            lo = 0; hi = 0;
            /* Re-accumulate low 64 bits only. */
            {
                unsigned long long tlo = 0;
                for (size_t j = 0; j < body; j++) {
                    unsigned char cc = (unsigned char)text[j];
                    if (cc == '\'') continue;
                    unsigned dd;
                    if (cc >= '0' && cc <= '9') dd = (unsigned)(cc - '0');
                    else if (cc >= 'a' && cc <= 'f') dd = (unsigned)(cc - 'a' + 10);
                    else if (cc >= 'A' && cc <= 'F') dd = (unsigned)(cc - 'A' + 10);
                    else continue;
                    if (dd >= (unsigned)base) continue;
                    tlo = tlo * (unsigned long long)base + (unsigned long long)dd;
                }
                lo = tlo;
            }
            break;
        }
    }
    /* Hex / octal wider than 64 bits: wrap to low 64 bits (GCC behaviour). */
    if (!decimal && hi != 0) {
        fprintf(stderr, "%s:%d:%d: warning: integer constant is too large "
                "for its type\n", loc.file, loc.line, loc.col);
        hi = 0;
    }
    int width, is_unsigned;
    if (suffix_u) {
        /* Unsigned ladder: try 32/64/128-bit unsigned.  Hex/octal with U
         * suffix never exceed 64 bits (they wrap), so 128 is only reachable
         * for decimal literals like 340282366920938463463374607431768211455u. */
        is_unsigned = 1;
        if (u128_fits(lo, hi, 32, 1) && !suffix_l) width = 4;
        else if (u128_fits(lo, hi, 64, 1)) width = 8;
        else width = 16; /* unsigned __int128 */
    } else if (decimal) {
        /* Decimal ladder (GCC extension): int → long → long long → __int128
         * → unsigned __int128.  Values in [2^63, 2^127-1] become __int128;
         * values in [2^127, 2^128-1] become unsigned __int128. */
        is_unsigned = 0;
        if (u128_fits(lo, hi, 32, 0) && !suffix_l) width = 4;
        else if (u128_fits(lo, hi, 64, 0)) width = 8;
        else if (u128_fits(lo, hi, 128, 0)) width = 16;            /* __int128 */
        else if (u128_fits(lo, hi, 128, 1)) { width = 16; is_unsigned = 1; } /* unsigned __int128 */
        else { width = 4; lo = 0; hi = 0; }  /* shouldn't reach (overflow handled above) */
    } else {
        /* Hex / octal: signed then unsigned at each rank; never __int128. */
        if (u128_fits(lo, hi, 32, 0) && !suffix_l) { width = 4; is_unsigned = 0; }
        else if (u128_fits(lo, hi, 32, 1) && !suffix_l) { width = 4; is_unsigned = 1; }
        else if (u128_fits(lo, hi, 64, 0)) { width = 8; is_unsigned = 0; }
        else { width = 8; is_unsigned = 1; }
    }
    *out_lo = lo;
    *out_hi = hi;
    *out_width = width;
    *out_unsigned = is_unsigned;
}

static int simple_escape_value(int c) {
    switch (c) {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'v': return '\v';
    case 'e': return 27;
    case '\\': return '\\';
    case '\'': return '\'';
    case '"': return '"';
    case '?': return '?';
    case '0': return '\0';
    default:  return c;
    }
}

static int char_literal_value(const char *text) {
    /* text[0] == '\'' */
    if (text[1] == '\\') {
        /* Hex escape `\xHH...`: parse all consecutive hex digits. */
        if (text[2] == 'x' || text[2] == 'X') {
            int val = 0;
            int i = 3;
            while (text[i] != '\0' && isxdigit((unsigned char)text[i])) {
                char c = text[i];
                int d = (c >= '0' && c <= '9') ? c - '0'
                      : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                      : c - 'A' + 10;
                val = val * 16 + d;
                i++;
            }
            /* lexer guarantees at least one hex digit follows `\x` */
            return val & 0xff;
        }
        /* Octal escape `\NNN`: up to 3 octal digits. */
        if (text[2] >= '0' && text[2] <= '7') {
            int val = 0;
            int i = 2;
            int n = 0;
            while (text[i] != '\0' && n < 3 && text[i] >= '0' && text[i] <= '7') {
                val = val * 8 + (text[i] - '0');
                i++; n++;
            }
            return val & 0xff;
        }
        return simple_escape_value((unsigned char)text[2]);
    }
    const unsigned char *s = (const unsigned char *)text + 1;
    if (s[0] < 0x80) return s[0];
    if ((s[0] & 0xe0) == 0xc0 && (s[1] & 0xc0) == 0x80) {
        return ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
    }
    if ((s[0] & 0xf0) == 0xe0 && (s[1] & 0xc0) == 0x80 && (s[2] & 0xc0) == 0x80) {
        return ((s[0] & 0x0f) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
    }
    if ((s[0] & 0xf8) == 0xf0 && (s[1] & 0xc0) == 0x80 && (s[2] & 0xc0) == 0x80 && (s[3] & 0xc0) == 0x80) {
        return ((s[0] & 0x07) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
    }
    return s[0];
}

static int is_imag_literal(const char *text) {
    if (!text) return 0;
    for (const char *s = text; *s; s++) {
        if (*s == 'i' || *s == 'I' || *s == 'j' || *s == 'J') return 1;
    }
    return 0;
}

static Expr *make_imag_literal(Parser *p, const char *text, SourceLoc loc) {
    int is_float = 0;
    int is_ld = 0;
    for (const char *s = text; *s; s++) {
        if (*s == 'f' || *s == 'F') is_float = 1;
        if (*s == 'l' || *s == 'L') is_ld = 1;
    }
    Type base = is_float ? type_make_float(4) : (is_ld ? type_make_float(16) : type_make_float(8));
    Type cty = get_or_create_complex_type(p, base);

    /* Clean imag string by stripping i/I/j/J */
    char clean[64];
    size_t ci = 0;
    for (size_t i = 0; text[i] && ci + 1 < sizeof(clean); i++) {
        if (text[i] != 'i' && text[i] != 'I' && text[i] != 'j' && text[i] != 'J')
            clean[ci++] = text[i];
    }
    clean[ci] = '\0';
    if (ci == 0 || (ci == 1 && (clean[0] == 'f' || clean[0] == 'F' || clean[0] == 'l' || clean[0] == 'L'))) {
        snprintf(clean, sizeof(clean), "1.0");
    }

    Expr *er = expr_new_float_lit("0.0", base.width, loc);
    Expr *ei = expr_new_float_lit(clean, base.width, loc);
    Expr **elems = malloc(2 * sizeof(Expr *));
    elems[0] = er;
    elems[1] = ei;
    Expr *init = expr_new_init_list(elems, 2, loc);
    return expr_new_compound_literal(cty, init, loc);
}

/* primary-expr = INT_LITERAL | CHAR_LITERAL | IDENT [ "(" arg-list? ")" ]  | "(" (type ")" unary | expr ")" ) | ... */
static Expr *parse_primary(Parser *p) {
    const Token *t = peek(p);
    if (t->kind == TK_INT_LITERAL) {
        if (is_imag_literal(t->text)) {
            Expr *e = make_imag_literal(p, t->text, t->loc);
            advance(p);
            return parse_postfix(p, e);
        }
        int width, is_unsigned;
        unsigned long long lo = 0, hi = 0;
        int_literal_typed(t->text, t->loc, &lo, &hi, &width, &is_unsigned);
        Expr *e = expr_new_int_bits(lo, hi, width, is_unsigned, t->loc);
        advance(p);
        return parse_postfix(p, e);
    }
    if (t->kind == TK_FLOAT_LITERAL) {
        if (is_imag_literal(t->text)) {
            Expr *e = make_imag_literal(p, t->text, t->loc);
            advance(p);
            return parse_postfix(p, e);
        }
        int width = 8;
        float_literal_width(t->text, &width); /* classify width (4/8/16) */
        Expr *e = expr_new_float_lit(t->text, width, t->loc);
        advance(p);
        return e;  /* float literal is a primary — no postfix needed */
    }
    if (t->kind == TK_IDENT
        && (strcmp(t->text, "L") == 0 || strcmp(t->text, "U") == 0)
        && p->pos + 1 < p->tokens->len
        && (p->tokens->data[p->pos + 1].kind == TK_CHAR_LITERAL
            || p->tokens->data[p->pos + 1].kind == TK_STRING_LITERAL)) {
        SourceLoc loc = t->loc;
        advance(p); /* consume L / U */
        if (peek(p)->kind == TK_CHAR_LITERAL) {
            Expr *e = expr_new_int(char_literal_value(peek(p)->text), loc);
            advance(p);
            return e;
        }
        /* Wide string: L"a" "b" → (int[]){'a','b',0} so indexing uses wchar units. */
        int total = 0;
        int *wbuf = NULL;
        while (peek(p)->kind == TK_STRING_LITERAL) {
            const char *src = peek(p)->text;
            size_t slen = strlen(src);
            if (slen >= 1 && src[0] == '"') { src++; slen--; }
            if (slen >= 1 && src[slen - 1] == '"') slen--;
            for (size_t i = 0; i < slen; ) {
                int ch;
                if (src[i] == '\\' && i + 1 < slen) {
                    i++;
                    ch = simple_escape_value((unsigned char)src[i]);
                    i++;
                } else {
                    const unsigned char *s = (const unsigned char *)src + i;
                    if (s[0] < 0x80) {
                        ch = s[0];
                        i++;
                    } else if ((s[0] & 0xe0) == 0xc0 && i + 1 < slen && (s[1] & 0xc0) == 0x80) {
                        ch = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
                        i += 2;
                    } else if ((s[0] & 0xf0) == 0xe0 && i + 2 < slen && (s[1] & 0xc0) == 0x80 && (s[2] & 0xc0) == 0x80) {
                        ch = ((s[0] & 0x0f) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
                        i += 3;
                    } else if ((s[0] & 0xf8) == 0xf0 && i + 3 < slen && (s[1] & 0xc0) == 0x80 && (s[2] & 0xc0) == 0x80 && (s[3] & 0xc0) == 0x80) {
                        ch = ((s[0] & 0x07) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
                        i += 4;
                    } else {
                        ch = s[0];
                        i++;
                    }
                }
                wbuf = realloc(wbuf, (size_t)(total + 1) * sizeof(int));
                wbuf[total++] = ch;
            }
            advance(p);
        }
        wbuf = realloc(wbuf, (size_t)(total + 1) * sizeof(int));
        wbuf[total] = 0;
        int n = total + 1;
        Expr **els = malloc((size_t)n * sizeof(Expr *));
        for (int i = 0; i < n; i++)
            els[i] = expr_new_int(wbuf[i], loc);
        free(wbuf);
        Type it = type_make_int(4, 0);
        Type arr = type_make_array(it, n);
        type_free(&it);
        Expr *init = expr_new_init_list(els, n, loc);
        Expr *e = expr_new_compound_literal(arr, init, loc);
        type_free(&arr);
        return parse_postfix(p, e);
    }
    if (t->kind == TK_STRING_LITERAL) {
        /* Token text includes surrounding quotes; strip and process escapes.
         * Adjacent string literals ("a" "b") are concatenated by the parser
         * (NOT the lexer) so each literal's escapes are decoded independently
         * before joining — required because `\xHH...` consumes consecutive hex
         * digits, so "\x7f" "ELF" must NOT be merged into "\x7fELF". */
        int total = 0;
        char *buf = NULL;
        while (peek(p)->kind == TK_STRING_LITERAL) {
            const char *src = peek(p)->text;
            size_t slen = strlen(src);
            if (slen >= 1 && src[0] == '"') { src++; slen--; }
            if (slen >= 1 && src[slen-1] == '"') slen--;
            char *seg = malloc(slen + 1);
            int slen2 = 0;
            for (size_t i = 0; i < slen; i++) {
                char c = src[i];
                if (c == '\\' && i + 1 < slen) {
                    i++;
                    if (src[i] == 'x' || src[i] == 'X') {
                        int val = 0;
                        while (i + 1 < slen && isxdigit((unsigned char)src[i + 1])) {
                            i++;
                            char h = src[i];
                            int d = (h >= '0' && h <= '9') ? h - '0'
                                  : (h >= 'a' && h <= 'f') ? h - 'a' + 10
                                  : h - 'A' + 10;
                            val = val * 16 + d;
                        }
                        seg[slen2++] = (char)(val & 0xff);
                    } else if (src[i] >= '0' && src[i] <= '7') {
                        int val = src[i] - '0';
                        int n = 1;
                        while (n < 3 && i + 1 < slen
                               && src[i + 1] >= '0' && src[i + 1] <= '7') {
                            i++;
                            val = val * 8 + (src[i] - '0');
                            n++;
                        }
                        seg[slen2++] = (char)(val & 0xff);
                    } else switch (src[i]) {
                    case 'n':  seg[slen2++] = '\n'; break;
                    case 't':  seg[slen2++] = '\t'; break;
                    case 'r':  seg[slen2++] = '\r'; break;
                    case 'a':  seg[slen2++] = '\a'; break;
                    case 'b':  seg[slen2++] = '\b'; break;
                    case 'f':  seg[slen2++] = '\f'; break;
                    case 'v':  seg[slen2++] = '\v'; break;
                    case 'e':  seg[slen2++] = 27; break;
                    case '0':  seg[slen2++] = '\0'; break;
                    case '\\': seg[slen2++] = '\\'; break;
                    case '"':  seg[slen2++] = '"'; break;
                    case '\'': seg[slen2++] = '\''; break;
                    case '?':  seg[slen2++] = '?'; break;
                    default:   seg[slen2++] = src[i]; break;
                    }
                } else {
                    seg[slen2++] = c;
                }
            }
            buf = realloc(buf, (size_t)total + slen2 + 1);
            memcpy(buf + total, seg, slen2);
            total += slen2;
            free(seg);
            advance(p);
        }
        Expr *e = expr_new_str(buf, total, t->loc);
        free(buf);
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
        /* Enum constant?  Resolve to an int literal at parse time. */
        {
            const EnumConstant *ec =
                enum_registry_find_constant(&p->tu->enums, ident->text);
            if (ec)
                return expr_new_int(ec->value, ident->loc);
        }
        if (strcmp(ident->text, "__FUNCTION__") == 0
            || strcmp(ident->text, "__func__") == 0
            || strcmp(ident->text, "__PRETTY_FUNCTION__") == 0) {
            const char *nm = p->cur_fn_name ? p->cur_fn_name : "";
            Expr *e = expr_new_str(nm, (int)strlen(nm), ident->loc);
            return parse_postfix(p, e);
        }
        /* A call `IDENT(args)` is built by parse_postfix so that indirect
         * calls (`fp(x)`, `(*fp)(x)`, `arr[i](x)`) parse uniformly. */
        Expr *e = expr_new_var(ident->text, ident->loc);
        return parse_postfix(p, e);
    }
    if (t->kind == TK_LPAREN) {
        if (p->pos + 1 < p->tokens->len && p->tokens->data[p->pos + 1].kind == TK_LBRACE) {
            /* GNU C Statement Expression: ({ ... }) */
            SourceLoc loc = peek(p)->loc;
            advance(p); /* consume '(' */
            advance(p); /* consume '{' */
            StmtArray stmts;
            stmt_array_init(&stmts);
            parse_stmt_list(p, &stmts);
            expect_kind(p, TK_RBRACE, "'}'");
            expect_kind(p, TK_RPAREN, "')'");
            Expr *e = expr_new_stmt_expr(&stmts, loc);
            return parse_postfix(p, e);
        }
        /* Cast?  (Type) unary   vs.   compound literal (Type){ ... }  vs.  (expr) */
        if (is_type_start(p, p->pos + 1)) {
            SourceLoc loc = peek(p)->loc;
            advance(p);
            Type ty = parse_type_name(p); /* handles array dims for compound literals */
            expect_kind(p, TK_RPAREN, "')'");
            if (peek(p)->kind == TK_LBRACE) {
                /* Compound literal: (Type){ initializer-list }.  parse_init_list
                 * consumes the opening '{', so do not advance past it here. */
                Expr *init = parse_init_list(p);
                Expr *e = expr_new_compound_literal(ty, init, loc);
                type_free(&ty);
                return parse_postfix(p, e);
            }
            /* Cast: (Type) unary */
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

/* init-list = "{" { init-list | assign-expr } [ "," ] "}"
 * Parsed only as a declarator initializer (`int a[] = {1,2,3}`).  Each element
 * is either a nested brace list (`{{1,2},{3,4}}`) or an assignment-expression.
 * A trailing comma is tolerated.  The result is an EX_INIT_LIST expression. */
static Expr *parse_designator_chain(Parser *p, SourceLoc loc) {
    int kind = -1, idx = -1;
    char *member = NULL;
    int chained = 0;
    if (peek(p)->kind == TK_DOT) {
        advance(p);
        const Token *fn = peek(p);
        if (fn->kind != TK_IDENT)
            die_at(fn->loc.file, fn->loc.line, fn->loc.col,
                   "expected field name after '.' but got '%s'", fn->text);
        advance(p);
        member = xstrdup(fn->text);
        kind = 1;
        if (peek(p)->kind == TK_DOT || peek(p)->kind == TK_LBRACKET) chained = 1;
        else expect_kind(p, TK_ASSIGN, "'='");
    } else if (peek(p)->kind == TK_LBRACKET) {
        advance(p);
        const Token *ix = peek(p);
        if (ix->kind != TK_INT_LITERAL)
            die_at(ix->loc.file, ix->loc.line, ix->loc.col,
                   "expected integer constant in designator but got '%s'", ix->text);
        idx = int_literal_value(ix->text);
        advance(p);
        expect_kind(p, TK_RBRACKET, "']'");
        kind = 0;
        if (peek(p)->kind == TK_DOT || peek(p)->kind == TK_LBRACKET) chained = 1;
        else expect_kind(p, TK_ASSIGN, "'='");
    }
    Expr *val = NULL;
    if (chained) {
        val = parse_designator_chain(p, loc);
    } else if (peek(p)->kind == TK_LBRACE) {
        val = parse_init_list(p);
    } else {
        val = parse_assign(p);
    }
    Expr **elems = malloc(sizeof(Expr *));
    elems[0] = val;
    Expr *list = expr_new_init_list(elems, 1, loc);
    list->u.init_list.desig_kind[0] = kind;
    list->u.init_list.desig_index[0] = idx;
    list->u.init_list.desig_member[0] = member;
    return list;
}

static Expr *parse_init_list(Parser *p) {
    SourceLoc loc = peek(p)->loc;
    expect_kind(p, TK_LBRACE, "'{'");
    Expr **elements = NULL;
    /* Per-element designator (parallel to elements): kind -1 = positional,
     * 0 = [index] (index in dindex), 1 = .member (name in dmember). */
    int *dkind = NULL, *dindex = NULL;
    char **dmember = NULL;
    int num = 0, cap = 0;
    while (peek(p)->kind != TK_RBRACE) {
        if (peek(p)->kind == TK_EOF) {
            die_at(loc.file, loc.line, loc.col,
                   "unterminated initializer list");
        }
        int kind = -1, idx = -1, end_idx = -1;
        char *member = NULL;
        int has_chained = 0;
        if (peek(p)->kind == TK_DOT) {
            /* .field = expr or .field.subfield = expr */
            advance(p);
            const Token *fn = peek(p);
            if (fn->kind != TK_IDENT)
                die_at(fn->loc.file, fn->loc.line, fn->loc.col,
                       "expected field name after '.' but got '%s'", fn->text);
            advance(p);
            member = xstrdup(fn->text);
            kind = 1;
            if (peek(p)->kind == TK_DOT || peek(p)->kind == TK_LBRACKET) {
                has_chained = 1;
            } else {
                expect_kind(p, TK_ASSIGN, "'='");
            }
        } else if (peek(p)->kind == TK_LBRACKET) {
            /* [index] = expr or [start ... end] = expr */
            advance(p);
            const Token *ix = peek(p);
            if (ix->kind != TK_INT_LITERAL)
                die_at(ix->loc.file, ix->loc.line, ix->loc.col,
                       "expected integer constant in designator but got '%s'",
                       ix->text);
            idx = int_literal_value(ix->text);
            advance(p);
            if (peek(p)->kind == TK_ELLIPSIS) {
                advance(p);
                const Token *eix = peek(p);
                if (eix->kind != TK_INT_LITERAL)
                    die_at(eix->loc.file, eix->loc.line, eix->loc.col,
                           "expected integer constant after '...' in designator");
                end_idx = int_literal_value(eix->text);
                advance(p);
            }
            expect_kind(p, TK_RBRACKET, "']'");
            kind = 0;
            if (peek(p)->kind == TK_DOT || peek(p)->kind == TK_LBRACKET) {
                has_chained = 1;
            } else {
                expect_kind(p, TK_ASSIGN, "'='");
            }
        } else if (peek(p)->kind == TK_IDENT
                   && p->pos + 1 < p->tokens->len
                   && p->tokens->data[p->pos + 1].kind == TK_COLON) {
            /* Obsolete GNU designated initializer: `field: expr`. */
            member = xstrdup(peek(p)->text);
            advance(p); /* name */
            advance(p); /* ':' */
            kind = 1;
        }
        Expr *elem;
        if (has_chained) {
            elem = parse_designator_chain(p, loc);
        } else if (peek(p)->kind == TK_LBRACE) {
            elem = parse_init_list(p);
        } else {
            elem = parse_assign(p);
        }
        if (end_idx >= idx) {
            for (int r = idx; r <= end_idx; r++) {
                if (num >= cap) {
                    cap = cap ? cap * 2 : 8;
                    elements = realloc(elements, cap * sizeof(Expr *));
                    dkind = realloc(dkind, cap * sizeof(int));
                    dindex = realloc(dindex, cap * sizeof(int));
                    dmember = realloc(dmember, cap * sizeof(char *));
                }
                elements[num] = (r == end_idx) ? elem : expr_clone(elem);
                dkind[num] = kind;
                dindex[num] = r;
                dmember[num] = member ? xstrdup(member) : NULL;
                num++;
            }
            free(member);
        } else {
            if (num >= cap) {
                cap = cap ? cap * 2 : 8;
                elements = realloc(elements, cap * sizeof(Expr *));
                dkind = realloc(dkind, cap * sizeof(int));
                dindex = realloc(dindex, cap * sizeof(int));
                dmember = realloc(dmember, cap * sizeof(char *));
            }
            elements[num] = elem;
            dkind[num] = kind;
            dindex[num] = idx;
            dmember[num] = member;
            num++;
        }
        if (peek(p)->kind == TK_COMMA) {
            advance(p);
            if (peek(p)->kind == TK_RBRACE) break; /* trailing comma */
        } else if (peek(p)->kind != TK_RBRACE) {
            const Token *t = peek(p);
            die_at(t->loc.file, t->loc.line, t->loc.col,
                   "expected ',' or '}' in initializer list but got '%s'",
                   t->text);
        }
    }
    expect_kind(p, TK_RBRACE, "'}'");
    Expr *list = expr_new_init_list(elements, num, loc);
    for (int i = 0; i < num; i++) {
        list->u.init_list.desig_kind[i] = dkind[i];
        list->u.init_list.desig_index[i] = dindex[i];
        list->u.init_list.desig_member[i] = dmember[i];
    }
    free(dkind);
    free(dindex);
    free(dmember);
    return list;
}

/* ---- statement parsing (forward declarations) ---- */

static void parse_stmt_list(Parser *p, StmtArray *out);
static Stmt parse_stmt(Parser *p);
static Stmt parse_typedef_stmt(Parser *p);
static Stmt parse_switch(Parser *p);

/* stmt-list = { stmt } until '}' */
static void parse_stmt_list(Parser *p, StmtArray *out) {
    while (peek(p)->kind != TK_RBRACE) {
        const Token *t = peek(p);
        if (t->kind == TK_EOF) {
            die_at(t->loc.file, t->loc.line, t->loc.col,
                   "expected '}' but got end of file");
        }
        /* Flush any multi-declarator trailers queued by the previous stmt
         * before parsing the next one — they belong to the same scope. */
        for (size_t i = 0; i < p->prepend.len; i++)
            stmt_array_push(out, p->prepend.data[i]);
        p->prepend.len = 0;
        stmt_array_push(out, parse_stmt(p));
    }
    /* Flush any trailing queued declarators at the end of the list. */
    for (size_t i = 0; i < p->prepend.len; i++)
        stmt_array_push(out, p->prepend.data[i]);
    p->prepend.len = 0;
}

static int is_function_declaration_lookahead(Parser *p) {
    size_t save = p->pos;
    for (;;) {
        if (skip_attribute(p)) continue;
        TokenKind tk = peek(p)->kind;
        if (tk == TK_KW_STATIC || tk == TK_KW_EXTERN || tk == TK_KW_INLINE ||
            tk == TK_KW_CONST || tk == TK_KW_VOLATILE || tk == TK_KW_RESTRICT ||
            tk == TK_KW_SIGNED || tk == TK_KW_UNSIGNED ||
            tk == TK_KW_VOID || tk == TK_KW_INT || tk == TK_KW_CHAR ||
            tk == TK_KW_SHORT || tk == TK_KW_LONG || tk == TK_KW_FLOAT ||
            tk == TK_KW_DOUBLE || tk == TK_KW_BOOL || tk == TK_KW_COMPLEX) {
            advance(p);
        } else if (tk == TK_KW_STRUCT || tk == TK_KW_UNION || tk == TK_KW_ENUM) {
            advance(p);
            if (peek(p)->kind == TK_IDENT) advance(p);
            if (peek(p)->kind == TK_LBRACE) {
                int depth = 0;
                do {
                    if (peek(p)->kind == TK_LBRACE) depth++;
                    else if (peek(p)->kind == TK_RBRACE) depth--;
                    advance(p);
                } while (depth > 0 && peek(p)->kind != TK_EOF);
            }
        } else if (tk == TK_IDENT
                   && (strcmp(peek(p)->text, "register") == 0
                       || strcmp(peek(p)->text, "auto") == 0)) {
            advance(p);
        } else if (tk == TK_IDENT
                   && find_typedef_with_fallback(p, peek(p)->text)) {
            advance(p);
        } else if (tk == TK_IDENT
                   && (strcmp(peek(p)->text, "__int128") == 0
                       || strcmp(peek(p)->text, "__int128_t") == 0
                       || strcmp(peek(p)->text, "__uint128_t") == 0)) {
            advance(p);
        } else {
            break;
        }
    }
    while (peek(p)->kind == TK_STAR || peek(p)->kind == TK_KW_CONST ||
           peek(p)->kind == TK_KW_VOLATILE || peek(p)->kind == TK_KW_RESTRICT) advance(p);
    for (;;) { if (!skip_attribute(p)) break; }
    int saw_name = 0;
    if (peek(p)->kind == TK_IDENT) {
        advance(p);
        saw_name = 1;
    }
    for (;;) { if (!skip_attribute(p)) break; }
    int has_bracket = 0;
    while (peek(p)->kind == TK_LBRACKET) {
        has_bracket = 1;
        advance(p);
        while (peek(p)->kind != TK_RBRACKET && peek(p)->kind != TK_EOF) advance(p);
        if (peek(p)->kind == TK_RBRACKET) advance(p);
    }
    int is_func = (!has_bracket && saw_name && peek(p)->kind == TK_LPAREN);
    p->pos = save;
    return is_func;
}

static int is_function_definition_lookahead(Parser *p) {
    size_t save = p->pos;
    for (;;) {
        if (skip_attribute(p)) continue;
        TokenKind tk = peek(p)->kind;
        if (tk == TK_KW_STATIC || tk == TK_KW_EXTERN || tk == TK_KW_INLINE ||
            tk == TK_KW_CONST || tk == TK_KW_VOLATILE || tk == TK_KW_RESTRICT ||
            tk == TK_KW_SIGNED || tk == TK_KW_UNSIGNED ||
            tk == TK_KW_VOID || tk == TK_KW_INT || tk == TK_KW_CHAR ||
            tk == TK_KW_SHORT || tk == TK_KW_LONG || tk == TK_KW_FLOAT ||
            tk == TK_KW_DOUBLE || tk == TK_KW_BOOL || tk == TK_KW_COMPLEX) {
            advance(p);
        } else if (tk == TK_KW_STRUCT || tk == TK_KW_UNION || tk == TK_KW_ENUM) {
            advance(p);
            if (peek(p)->kind == TK_IDENT) advance(p);
            if (peek(p)->kind == TK_LBRACE) {
                int depth = 0;
                do {
                    if (peek(p)->kind == TK_LBRACE) depth++;
                    else if (peek(p)->kind == TK_RBRACE) depth--;
                    advance(p);
                } while (depth > 0 && peek(p)->kind != TK_EOF);
            }
        } else if (tk == TK_IDENT
                   && (strcmp(peek(p)->text, "register") == 0
                       || strcmp(peek(p)->text, "auto") == 0)) {
            advance(p);
        } else if (tk == TK_IDENT
                   && find_typedef_with_fallback(p, peek(p)->text)) {
            advance(p);
        } else if (tk == TK_IDENT
                   && (strcmp(peek(p)->text, "__int128") == 0
                       || strcmp(peek(p)->text, "__int128_t") == 0
                       || strcmp(peek(p)->text, "__uint128_t") == 0)) {
            advance(p);
        } else {
            break;
        }
    }
    while (peek(p)->kind == TK_STAR || peek(p)->kind == TK_KW_CONST ||
           peek(p)->kind == TK_KW_VOLATILE || peek(p)->kind == TK_KW_RESTRICT) advance(p);
    for (;;) { if (!skip_attribute(p)) break; }
    int saw_name = 0;
    if (peek(p)->kind == TK_IDENT) {
        advance(p);
        saw_name = 1;
    }
    for (;;) { if (!skip_attribute(p)) break; }
    while (peek(p)->kind == TK_LBRACKET) {
        advance(p);
        while (peek(p)->kind != TK_RBRACKET && peek(p)->kind != TK_EOF) advance(p);
        if (peek(p)->kind == TK_RBRACKET) advance(p);
    }
    int is_def = 0;
    if (saw_name && peek(p)->kind == TK_LPAREN) {
        advance(p);
        int depth = 1;
        while (depth > 0 && peek(p)->kind != TK_EOF) {
            if (peek(p)->kind == TK_LPAREN) depth++;
            else if (peek(p)->kind == TK_RPAREN) depth--;
            advance(p);
        }
        while (skip_attribute(p)) {}
        /* Handle K&R parameter declarations before '{' */
        while (is_type_start(p, p->pos)) {
            advance(p);
            while (peek(p)->kind != TK_SEMICOLON && peek(p)->kind != TK_EOF) advance(p);
            if (peek(p)->kind == TK_SEMICOLON) advance(p);
            while (skip_attribute(p)) {}
        }
        while (skip_attribute(p)) {}
        is_def = (peek(p)->kind == TK_LBRACE);
    }
    p->pos = save;
    return is_def;
}

static Stmt parse_stmt(Parser *p);

/* Evaluate a constant in a case label — may be an integer literal or an enum
 * constant defined in this file, a sibling file, or an imported package.
 * `name` is the case label name for error messages. */
static long long case_constant_value(Parser *p, const char *text) {
    if (text[0] >= '0' && text[0] <= '9') {
        return (long long)strtoll(text, NULL, 0);
    }
    const EnumConstant *ec =
        enum_registry_find_constant(&p->tu->enums, text);
    if (ec) return ec->value;
    /* Check same-package sibling files and the package export table. */
    if (p->pkg_ctx && p->tu->package.name) {
        Package *cur = pkg_find(p->pkg_ctx, p->tu->package.name);
        if (cur) {
            ec = enum_registry_find_constant(&cur->enums, text);
            if (ec) return ec->value;
            for (size_t f = 0; f < cur->nfiles; f++) {
                if (&cur->files[f] == p->tu) continue;
                ec = enum_registry_find_constant(&cur->files[f].enums, text);
                if (ec) return ec->value;
            }
        }
    }
    {
        const Token *t = peek(p);
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "case label '%s' is not a constant", text);
    }
    return 0; /* unreachable */
}

typedef struct SwitchContext {
    Stmt *switch_stmt;
    int switch_id;
    int case_count;
    struct SwitchContext *parent;
} SwitchContext;

static SwitchContext *g_cur_switch = NULL;

/* Parse a switch statement:
 *   switch (expr) stmt
 * Supports Duff's device and arbitrary nesting of case/default labels. */
static Stmt parse_switch(Parser *p) {
    const Token *kw = peek(p);
    advance(p);  /* consume "switch" */
    expect_kind(p, TK_LPAREN, "'('");
    Expr *cond = parse_expr(p);
    expect_kind(p, TK_RPAREN, "')'");

    Stmt s;
    s.kind = ST_SWITCH;
    s.loc = kw->loc;
    s.u.switch_s.cond = cond;
    s.u.switch_s.body = NULL;
    s.u.switch_s.cases = NULL;
    s.u.switch_s.num_cases = 0;
    s.u.switch_s.cap_cases = 0;

    SwitchContext ctx;
    ctx.switch_stmt = &s;
    ctx.switch_id = p->anon_counter++;
    ctx.case_count = 0;
    ctx.parent = g_cur_switch;
    g_cur_switch = &ctx;

    Stmt body = parse_stmt(p);
    s.u.switch_s.body = stmt_alloc();
    *s.u.switch_s.body = body;

    g_cur_switch = ctx.parent;
    return s;
}

static Stmt parse_stmt(Parser *p) {
    if (peek(p)->kind != TK_KW_TYPEDEF && !is_type_start(p, p->pos)) {
        for (;;) { if (!skip_attribute(p)) break; }
    }
    TokenKind k = peek(p)->kind;
    /* Null statement: `;` */
    if (k == TK_SEMICOLON) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        Stmt s;
        s.kind = ST_BLOCK;
        s.loc = loc;
        stmt_array_init(&s.u.block);
        return s;
    }
    /* `typedef` at block scope is parsed as a decl-stmt whose init is NULL;
     * sema registers the alias.  A leading typedef-name identifier is treated
     * as a type start (declaration), matching C's "lexer hack" resolution. */
    if (k == TK_KW_TYPEDEF || is_type_start(p, p->pos)) {
        /* decl-stmt: [typedef] type IDENT [ "[" N "]" ]* ["=" expr] ";" */
        if (k == TK_KW_TYPEDEF) {
            return parse_typedef_stmt(p);
        }
        if (is_function_definition_lookahead(p)) {
            FunctionDecl fn = parse_function_decl(p);
            if (p->tu->functions.len >= p->tu->functions.cap) {
                size_t new_cap = p->tu->functions.cap ? p->tu->functions.cap * 2 : 4;
                p->tu->functions.data = realloc(p->tu->functions.data,
                                             new_cap * sizeof(FunctionDecl));
                if (!p->tu->functions.data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
                p->tu->functions.cap = new_cap;
            }
            p->tu->functions.data[p->tu->functions.len++] = fn;
            Stmt s;
            s.kind = ST_BLOCK;
            s.loc = fn.loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        SourceLoc decl_loc = peek(p)->loc;
        /* Storage class: `static` (persistent) / `extern` (declaration only).
         * `const` is handled inside parse_specifiers. */
        int storage_class = 0; /* 0=default, 1=static, 2=extern */
        for (;;) {
            if (peek(p)->kind == TK_KW_STATIC) {
                storage_class = 1;
                advance(p);
            } else if (peek(p)->kind == TK_KW_EXTERN) {
                storage_class = 2;
                advance(p);
            } else if (peek(p)->kind == TK_IDENT
                       && (strcmp(peek(p)->text, "register") == 0
                           || strcmp(peek(p)->text, "auto") == 0)) {
                advance(p);
            } else break;
        }
        /* Parse the common specifier base type once; each declarator in a
         * comma-separated list (`int a, b, c`) shares this base.  Clone it for
         * each declarator because parse_declarator takes its argument by value
         * and may consume (free) its heap children along some paths. */
        Type base = parse_specifiers_full(p, &storage_class);
        /* C allows mixing storage-class specifiers with the type:
         * `static int x` and `struct S { } static x` are both valid. */
        for (;;) {
            if (skip_attribute(p)) continue;
            if (peek(p)->kind == TK_KW_STATIC) {
                storage_class = 1;
                advance(p);
            } else if (peek(p)->kind == TK_KW_EXTERN) {
                storage_class = 2;
                advance(p);
            } else if (peek(p)->kind == TK_IDENT
                       && (strcmp(peek(p)->text, "register") == 0
                           || strcmp(peek(p)->text, "auto") == 0)) {
                advance(p);
            } else break;
        }
        /* A definition-only declaration (`enum { ... };`, `struct { ... };`)
         * has no declarator or variable name — the specifiers consumed the
         * whole definition and the next token is `;`.  Accept it as a no-op. */
        if (peek(p)->kind == TK_SEMICOLON) {
            advance(p);
            Stmt s;
            memset(&s, 0, sizeof(s));
            s.kind = ST_BLOCK;
            s.loc = decl_loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        /* Collect one ST_DECL per declarator. */
        StmtArray decls;
        stmt_array_init(&decls);
        for (;;) {
            char *decl_name = NULL;
            Type ty = parse_declarator(p, type_clone(base), &decl_name);
            if (!decl_name) {
                const Token *name = peek(p);
                if (name->kind != TK_IDENT) {
                    die_at(name->loc.file, name->loc.line, name->loc.col,
                           "expected variable name but got '%s'", name->text);
                }
                decl_name = xstrdup(name->text);
                advance(p);
            }
            Stmt s;
            s.kind = ST_DECL;
            s.loc = decl_loc;
            s.u.decl.name = decl_name;
            s.u.decl.type = ty;
            s.u.decl.storage_class = storage_class;
            s.u.decl.init = NULL;
            s.u.decl.alias_target = NULL;
            while (parse_attribute(p, NULL, NULL, NULL, NULL, &s.u.decl.alias_target)) {}
            if (peek(p)->kind == TK_ASSIGN) {
                advance(p);
                /* `extern` may not have an initializer. */
                if (storage_class == 2) {
                    die_at(s.loc.file, s.loc.line, s.loc.col,
                           "cannot initialize an 'extern' variable");
                }
                if (peek(p)->kind == TK_LBRACE) {
                    s.u.decl.init = parse_init_list(p);
                } else {
                    /* Initializer is an ASSIGNMENT-expression, NOT a comma
                     * expression — otherwise `int a = 5, b = 6` would let the
                     * init of `a` consume the `,` as a comma operator. */
                    s.u.decl.init = parse_assign(p);
                }
            }
            while (parse_attribute(p, NULL, NULL, NULL, NULL, &s.u.decl.alias_target)) {}
            if (!s.u.decl.alias_target && g_parsed_alias) {
                s.u.decl.alias_target = g_parsed_alias;
                g_parsed_alias = NULL;
            }
            stmt_array_push(&decls, s);
            if (peek(p)->kind == TK_COMMA) {
                advance(p);
                continue;
            }
            break;
        }
        expect_kind(p, TK_SEMICOLON, "';'");
        type_free(&base);
        /* A stmt slot holds one declaration.  data[0]'s ownership passes back
         * to the caller; the trailing declarators' ownership moves (shallow
         * copy) into the prepend queue so they land in the same scope.  No
         * element's heap fields are freed here — they all live on, either in
         * the returned stmt or in prepend — only the array buffer is freed. */
        Stmt first = decls.data[0];
        for (size_t i = 1; i < decls.len; i++)
            stmt_array_push(&p->prepend, decls.data[i]);
        free(decls.data);
        return first;
    }
    if (k == TK_KW_RETURN) {
        /* return-stmt — bare `return;` (value==NULL) is allowed for void
         * functions; sema enforces the void/non-void mismatch rules. */
        const Token *kw = peek(p);
        advance(p);  /* consume "return" */
        Stmt s;
        s.kind = ST_RETURN;
        s.loc = kw->loc;
        if (peek(p)->kind == TK_SEMICOLON) {
            s.u.value = NULL;
        } else {
            s.u.value = parse_expr(p);
        }
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
    if (k == TK_KW_DO) {
        const Token *kw = peek(p);
        advance(p);  /* consume "do" */
        Stmt body = parse_stmt(p);
        expect_kind(p, TK_KW_WHILE, "'while'");
        expect_kind(p, TK_LPAREN, "'('");
        Expr *cond = parse_expr(p);
        expect_kind(p, TK_RPAREN, "')'");
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s;
        s.kind = ST_DO_WHILE;
        s.loc = kw->loc;
        s.u.do_s.body = stmt_alloc();
        *s.u.do_s.body = body;
        s.u.do_s.cond = cond;
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
            /* A multi-declarator init (`for (unsigned _w = v, over; ...)`)
             * parks the trailing declarators in p->prepend.  They must share
             * the for-loop's scope and be visible in cond/step/body, so fold
             * them into the init as a block (sema checks the block's stmts
             * directly in the for scope — no extra nesting). */
            if (p->prepend.len > 0) {
                Stmt blk;
                blk.kind = ST_BLOCK;
                blk.loc = is.loc;
                stmt_array_init(&blk.u.block);
                stmt_array_push(&blk.u.block, is);
                for (size_t pi = 0; pi < p->prepend.len; pi++)
                    stmt_array_push(&blk.u.block, p->prepend.data[pi]);
                p->prepend.len = 0;
                init_ptr = stmt_alloc();
                *init_ptr = blk;
            } else {
                init_ptr = stmt_alloc();
                *init_ptr = is;
            }
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
    if (k == TK_KW_GOTO) {
        const Token *kw = peek(p);
        advance(p);  /* consume "goto" */
        /* GNU C indirect (computed) goto: goto *expr; */
        if (peek(p)->kind == TK_STAR) {
            advance(p);  /* consume '*' */
            Expr *target = parse_expr(p);
            expect_kind(p, TK_SEMICOLON, "';'");
            Stmt s;
            s.kind = ST_GOTO;
            s.loc = kw->loc;
            s.u.goto_s.target = NULL;
            s.u.goto_s.target_expr = target;
            return s;
        }
        const Token *label = peek(p);
        if (label->kind != TK_IDENT) {
            die_at(label->loc.file, label->loc.line, label->loc.col,
                   "expected label name after 'goto' but got '%s'", label->text);
        }
        advance(p);
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s;
        s.kind = ST_GOTO;
        s.loc = kw->loc;
        s.u.goto_s.target = xstrdup(label->text);
        s.u.goto_s.target_expr = NULL;
        return s;
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
    /* label: stmt — an identifier followed by ':' is a label. */
    if (k == TK_IDENT && p->tokens->data[p->pos + 1].kind == TK_COLON) {
        const Token *name = peek(p);
        advance(p);          /* consume label name */
        advance(p);          /* consume ':' */
        Stmt inner = parse_stmt(p);
        Stmt s;
        s.kind = ST_LABEL;
        s.loc = name->loc;
        s.u.label_s.name = xstrdup(name->text);
        s.u.label_s.stmt = stmt_alloc();
        *s.u.label_s.stmt = inner;
        return s;
    }
    if (k == TK_KW_CASE) {
        const Token *kw = peek(p);
        advance(p);  /* consume "case" */
        const Token *cv = peek(p);
        long long value = 0;
        long long high_value = 0;
        int is_range = 0;
        if (cv->kind == TK_IDENT) {
            value = case_constant_value(p, cv->text);
            advance(p);
        } else {
            Expr *ce = parse_ternary(p);
            long long folded;
            if (!fold_const_int(ce, &folded))
                die_at(cv->loc.file, cv->loc.line, cv->loc.col,
                       "case label must be an integer constant expression");
            expr_free(ce);
            value = folded;
        }
        if (peek(p)->kind == TK_ELLIPSIS) {
            advance(p); /* consume "..." */
            const Token *hv = peek(p);
            if (hv->kind == TK_IDENT) {
                high_value = case_constant_value(p, hv->text);
                advance(p);
            } else {
                Expr *he = parse_ternary(p);
                long long folded_h;
                if (!fold_const_int(he, &folded_h))
                    die_at(hv->loc.file, hv->loc.line, hv->loc.col,
                           "case range high value must be an integer constant expression");
                expr_free(he);
                high_value = folded_h;
            }
            is_range = 1;
        }
        expect_kind(p, TK_COLON, "':'");
        char lbl[64];
        if (g_cur_switch) {
            snprintf(lbl, sizeof(lbl), "__sw_%d_case_%d", g_cur_switch->switch_id, g_cur_switch->case_count++);
            switch_push_case_range(g_cur_switch->switch_stmt, 0, value, high_value, is_range, lbl);
        } else {
            snprintf(lbl, sizeof(lbl), "__case_%d", p->anon_counter++);
        }
        Stmt inner;
        if (peek(p)->kind == TK_RBRACE) {
            inner.kind = ST_EXPR;
            inner.loc = kw->loc;
            inner.u.expr = NULL;
        } else {
            inner = parse_stmt(p);
        }
        Stmt s;
        s.kind = ST_LABEL;
        s.loc = kw->loc;
        s.u.label_s.name = xstrdup(lbl);
        s.u.label_s.stmt = stmt_alloc();
        *s.u.label_s.stmt = inner;
        return s;
    }
    if (k == TK_KW_DEFAULT) {
        const Token *kw = peek(p);
        advance(p);
        expect_kind(p, TK_COLON, "':'");
        char lbl[64];
        if (g_cur_switch) {
            snprintf(lbl, sizeof(lbl), "__sw_%d_default", g_cur_switch->switch_id);
            switch_push_case(g_cur_switch->switch_stmt, 1, 0, lbl);
        } else {
            snprintf(lbl, sizeof(lbl), "__default_%d", p->anon_counter++);
        }
        Stmt inner;
        if (peek(p)->kind == TK_RBRACE) {
            inner.kind = ST_EXPR;
            inner.loc = kw->loc;
            inner.u.expr = NULL;
        } else {
            inner = parse_stmt(p);
        }
        Stmt s;
        s.kind = ST_LABEL;
        s.loc = kw->loc;
        s.u.label_s.name = xstrdup(lbl);
        s.u.label_s.stmt = stmt_alloc();
        *s.u.label_s.stmt = inner;
        return s;
    }
    if (k == TK_KW_SWITCH) {
        return parse_switch(p);
    }
    if (k == TK_IDENT && strcmp(peek(p)->text, "__label__") == 0) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        while (peek(p)->kind != TK_SEMICOLON && peek(p)->kind != TK_EOF)
            advance(p);
        if (peek(p)->kind == TK_SEMICOLON) advance(p);
        Stmt s;
        memset(&s, 0, sizeof(s));
        s.kind = ST_BLOCK;
        s.loc = loc;
        stmt_array_init(&s.u.block);
        return s;
    }
    if (k == TK_IDENT && (strcmp(peek(p)->text, "asm") == 0 ||
                          strcmp(peek(p)->text, "__asm__") == 0 ||
                          strcmp(peek(p)->text, "__asm") == 0)) {
        SourceLoc loc = peek(p)->loc;
        advance(p);
        while (peek(p)->kind == TK_KW_VOLATILE || peek(p)->kind == TK_KW_CONST ||
               (peek(p)->kind == TK_IDENT && (strcmp(peek(p)->text, "__volatile__") == 0 ||
                                              strcmp(peek(p)->text, "__volatile") == 0 ||
                                              strcmp(peek(p)->text, "goto") == 0 ||
                                              strcmp(peek(p)->text, "__inline__") == 0))) {
            advance(p);
        }
        Stmt s;
        memset(&s, 0, sizeof(s));
        s.kind = ST_BLOCK;
        s.loc = loc;
        stmt_array_init(&s.u.block);
        if (peek(p)->kind == TK_LPAREN) {
            advance(p);
            /* Parse asm template string(s) */
            int is_empty_template = 1;
            while (peek(p)->kind == TK_STRING_LITERAL) {
                const char *src = peek(p)->text;
                size_t slen = strlen(src);
                if (slen >= 1 && src[0] == '"') { src++; slen--; }
                if (slen >= 1 && src[slen-1] == '"') slen--;
                for (size_t i = 0; i < slen; i++) {
                    if (!isspace((unsigned char)src[i])) { is_empty_template = 0; break; }
                }
                advance(p);
            }
            Expr *outputs[16];
            char *out_constr[16];
            int num_outputs = 0;
            Expr *inputs[16];
            char *in_constr[16];
            int num_inputs = 0;
            memset(outputs, 0, sizeof(outputs));
            memset(out_constr, 0, sizeof(out_constr));
            memset(inputs, 0, sizeof(inputs));
            memset(in_constr, 0, sizeof(in_constr));
            if (peek(p)->kind == TK_COLON) {
                advance(p); /* consume ':' */
                /* Outputs */
                while (peek(p)->kind != TK_COLON && peek(p)->kind != TK_RPAREN && peek(p)->kind != TK_EOF) {
                    char *c_str = NULL;
                    if (peek(p)->kind == TK_STRING_LITERAL) {
                        const char *src = peek(p)->text;
                        size_t slen = strlen(src);
                        if (slen >= 1 && src[0] == '"') { src++; slen--; }
                        if (slen >= 1 && src[slen-1] == '"') slen--;
                        c_str = malloc(slen + 1);
                        memcpy(c_str, src, slen);
                        c_str[slen] = '\0';
                        advance(p);
                    }
                    if (peek(p)->kind == TK_LPAREN) {
                        advance(p);
                        Expr *e = parse_expr(p);
                        expect_kind(p, TK_RPAREN, "')'");
                        if (num_outputs < 16) {
                            out_constr[num_outputs] = c_str;
                            outputs[num_outputs++] = e;
                        } else {
                            expr_free(e);
                            free(c_str);
                        }
                    }
                    if (peek(p)->kind == TK_COMMA) advance(p);
                    else break;
                }
                if (peek(p)->kind == TK_COLON) {
                    advance(p); /* consume ':' */
                    /* Inputs */
                    while (peek(p)->kind != TK_COLON && peek(p)->kind != TK_RPAREN && peek(p)->kind != TK_EOF) {
                        char *c_str = NULL;
                        if (peek(p)->kind == TK_STRING_LITERAL) {
                            const char *src = peek(p)->text;
                            size_t slen = strlen(src);
                            if (slen >= 1 && src[0] == '"') { src++; slen--; }
                            if (slen >= 1 && src[slen-1] == '"') slen--;
                            c_str = malloc(slen + 1);
                            memcpy(c_str, src, slen);
                            c_str[slen] = '\0';
                            advance(p);
                        }
                        if (peek(p)->kind == TK_LPAREN) {
                            advance(p);
                            Expr *e = parse_expr(p);
                            expect_kind(p, TK_RPAREN, "')'");
                            if (num_inputs < 16) {
                                in_constr[num_inputs] = c_str;
                                inputs[num_inputs++] = e;
                            } else {
                                expr_free(e);
                                free(c_str);
                            }
                        }
                        if (peek(p)->kind == TK_COMMA) advance(p);
                        else break;
                    }
                }
            }
            /* Skip remaining clobbers / labels until matching RPAREN */
            int depth = 1;
            while (depth > 0 && peek(p)->kind != TK_EOF) {
                if (peek(p)->kind == TK_LPAREN) depth++;
                else if (peek(p)->kind == TK_RPAREN) {
                    depth--;
                    if (depth == 0) { advance(p); break; }
                }
                advance(p);
            }
            if (is_empty_template) {
                for (int oi = 0; oi < num_outputs; oi++) {
                    int match_idx = -1;
                    char match_char = (char)('0' + oi);
                    for (int ii = 0; ii < num_inputs; ii++) {
                        if (in_constr[ii] && strchr(in_constr[ii], match_char)) {
                            match_idx = ii;
                            break;
                        }
                    }
                    if (match_idx < 0 && oi < num_inputs && (!out_constr[oi] || !strchr(out_constr[oi], '+')))
                        match_idx = oi;
                    if (match_idx >= 0 && outputs[oi] && inputs[match_idx]) {
                        Expr *assign = expr_new_assign(outputs[oi], inputs[match_idx], loc);
                        outputs[oi] = NULL;
                        inputs[match_idx] = NULL;
                        Stmt es;
                        memset(&es, 0, sizeof(es));
                        es.kind = ST_EXPR;
                        es.loc = loc;
                        es.u.expr = assign;
                        stmt_array_push(&s.u.block, es);
                    }
                }
            }
            for (int oi = 0; oi < num_outputs; oi++) {
                if (outputs[oi]) {
                    Stmt es;
                    memset(&es, 0, sizeof(es));
                    es.kind = ST_EXPR;
                    es.loc = loc;
                    es.u.expr = outputs[oi];
                    outputs[oi] = NULL;
                    stmt_array_push(&s.u.block, es);
                }
                free(out_constr[oi]);
            }
            for (int ii = 0; ii < num_inputs; ii++) {
                if (inputs[ii]) {
                    Stmt es;
                    memset(&es, 0, sizeof(es));
                    es.kind = ST_EXPR;
                    es.loc = loc;
                    es.u.expr = inputs[ii];
                    inputs[ii] = NULL;
                    stmt_array_push(&s.u.block, es);
                }
                free(in_constr[ii]);
            }
        }
        if (peek(p)->kind == TK_SEMICOLON) advance(p);
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

/* typedef-stmt = "typedef" type IDENT ";"
 * Registers the alias in the TU-level typedef registry.  A typedef occupies
 * no runtime statement, so we return an empty ST_BLOCK no-op — the caller
 * (parse_stmt_list or the file-scope loop) just moves on. */
static Stmt parse_typedef_stmt(Parser *p) {
    const Token *kw = peek(p);
    advance(p);  /* consume "typedef" */
    Type base = parse_specifiers(p);
    for (;;) {
        char *decl_name = NULL;
        Type ty = parse_declarator(p, type_clone(base), &decl_name);
        const Token *name = peek(p);
        if (!decl_name) {
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            decl_name = xstrdup(name->text);
            advance(p);
        }
        int attr_align = 0, attr_packed = 0, attr_sso = 0, attr_vec = 0;
        while (parse_attribute(p, &attr_align, &attr_packed, &attr_sso, &attr_vec, NULL)) {}
        if (attr_vec > 0 && !ty.is_vector) {
            Type vt = type_make_vector(ty, attr_vec);
            type_free(&ty);
            ty = vt;
        }
        TypedefEntry *exist = typedef_registry_get(&p->tu->typedefs, decl_name);
        if (exist) {
            size_t exist_idx = (size_t)(exist - p->tu->typedefs.data);
            int builtin_va = (strcmp(decl_name, "va_list") == 0
                              || strcmp(decl_name, "__builtin_va_list") == 0);
            if (p->cur_fn_name && exist_idx < p->typedef_mark) {
                /* Shadow an outer typedef; C block scope. */
                typedef_registry_add(&p->tu->typedefs, decl_name, ty);
            } else if (builtin_va) {
                /* The TU predeclares SysV va_list; torture files restated it
                 * with an equivalent layout under a fresh anonymous tag. */
                type_free(&ty);
            } else if (!type_same_typedef(exist->type, ty)) {
                die_at(kw->loc.file, kw->loc.line, kw->loc.col,
                       "redefinition of typedef '%s' with a different type",
                       decl_name);
            } else {
                type_free(&ty);
            }
            free(decl_name);
        } else {
            typedef_registry_add(&p->tu->typedefs, decl_name, ty);
            free(decl_name);
        }
        while (parse_attribute(p, &attr_align, &attr_packed, &attr_sso, &attr_vec, NULL)) {}
        if (peek(p)->kind == TK_COMMA) {
            advance(p);
            continue;
        }
        break;
    }
    type_free(&base);
    for (;;) { if (!skip_attribute(p)) break; }
    expect_kind(p, TK_SEMICOLON, "';'");
    Stmt s;
    s.kind = ST_BLOCK;
    s.loc = kw->loc;
    stmt_array_init(&s.u.block);
    return s;
}

static FunctionDecl parse_function_decl(Parser *p) {
    for (;;) { if (!skip_attribute(p)) break; }
    SourceLoc fn_loc = peek(p)->loc;
    /* Consume an optional leading storage class.  `extern` means a
     * declaration with no body; `static` gives the function LOCAL linkage;
     * `inline` is a no-op hint accepted so it doesn't choke
     * parse_type_abstract. */
    int is_extern = 0;
    int is_static = 0;
    for (;;) {
        if (skip_attribute(p)) continue;
        if (peek(p)->kind == TK_KW_STATIC) { advance(p); is_static = 1; }
        else if (peek(p)->kind == TK_KW_EXTERN) { advance(p); is_extern = 1; }
        else if (peek(p)->kind == TK_KW_INLINE) { advance(p); }
        else break;
    }
    Type ret_ty;
    const Token *name;
    if (is_type_start(p, p->pos)
        || peek(p)->kind == TK_KW_VOID
        || peek(p)->kind == TK_KW_STRUCT
        || peek(p)->kind == TK_KW_UNION
        || peek(p)->kind == TK_KW_ENUM) {
        ret_ty = parse_return_type(p);
        for (;;) { if (!skip_attribute(p)) break; }
        name = peek(p);
        if (name->kind != TK_IDENT) {
            die_at(name->loc.file, name->loc.line, name->loc.col,
                   "expected function name but got '%s'", name->text);
        }
        advance(p);
    } else if (peek(p)->kind == TK_IDENT) {
        /* GNU89 implicit-int: `s(i){ ... }` / `main(){ ... }`. */
        ret_ty = type_default_int();
        name = peek(p);
        advance(p);
    } else {
        const Token *t = peek(p);
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "expected function name but got '%s'", t->text);
        name = t;
        ret_ty = type_default_int();
    }

    expect_kind(p, TK_LPAREN, "'('");

    FunctionDecl fn;
    fn.name = xstrdup(name->text);
    fn.ret_type = ret_ty;
    param_array_init(&fn.params);
    stmt_array_init(&fn.body);
    fn.loc = fn_loc;
    fn.is_variadic = 0;
    fn.is_unprototyped = 0;
    fn.is_extern = is_extern;
    fn.is_static = is_static;
    fn.alias_target = NULL;
    fn.align = 0;
    fn.no_instrument = g_parsed_no_instrument;
    g_parsed_no_instrument = 0;

    /* Parameter list.  Three forms:
     *   (void)              — prototyped, no parameters
     *   (int x, char *p)    — prototyped
     *   (a, b) int a; ...   — K&R identifier list, possibly followed by decls
     *   ()                  — K&R unprototyped empty list */
    char *kr_names[16];
    int nkr = 0;
    if (peek(p)->kind == TK_KW_VOID
        && p->tokens->data[p->pos + 1].kind == TK_RPAREN) {
        advance(p);  /* consume `void` */
    } else if (peek(p)->kind != TK_RPAREN && is_type_start(p, p->pos)) {
        for (;;) {
            if (!is_type_start(p, p->pos)) {
                const Token *t = peek(p);
                die_at(t->loc.file, t->loc.line, t->loc.col,
                       "expected type for parameter but got '%s'", t->text);
            }
            char *pname = NULL;
            Type pty = parse_type(p, &pname);
            if (!pname) {
                pname = xstrdup("");
            }
            if (pty.kind == TY_ARRAY && pty.elem_type) {
                Type ptr = type_make_ptr(*pty.elem_type);
                ptr.vla_dim = pty.vla_dim;
                pty.vla_dim = NULL;
                type_free(&pty);
                pty = ptr;
            }
            param_array_push(&fn.params, pname, pty, peek(p)->loc);
            free(pname);
            if (peek(p)->kind == TK_COMMA) {
                advance(p);
                if (peek(p)->kind == TK_ELLIPSIS) {
                    advance(p);
                    fn.is_variadic = 1;
                    break;
                }
                continue;
            }
            break;
        }
        if (fn.params.len > 16) {
            die_at(fn.loc.file, fn.loc.line, fn.loc.col,
                   "more than 16 parameters not supported");
        }
    } else if (peek(p)->kind == TK_ELLIPSIS) {
        /* Old-style varargs with no named parameters: `f(...)`. */
        advance(p);
        fn.is_variadic = 1;
        fn.is_unprototyped = 1;
    } else if (peek(p)->kind != TK_RPAREN) {
        /* K&R identifier list: `f(a, b)`. */
        fn.is_unprototyped = 1;
        for (;;) {
            const Token *id = peek(p);
            if (id->kind != TK_IDENT) {
                die_at(id->loc.file, id->loc.line, id->loc.col,
                       "expected parameter name but got '%s'", id->text);
            }
            if (nkr >= 16) {
                die_at(id->loc.file, id->loc.line, id->loc.col,
                       "more than 16 parameters not supported");
            }
            kr_names[nkr++] = xstrdup(id->text);
            advance(p);
            if (peek(p)->kind == TK_COMMA) { advance(p); continue; }
            break;
        }
    } else {
        fn.is_unprototyped = 1;
    }

    expect_kind(p, TK_RPAREN, "')'");

    /* K&R declarations between `)` and `{`: `f(a, b) int a; char *b; {`. */
    if (nkr > 0) {
        Type kr_ty[16];
        int kr_set[16];
        for (int i = 0; i < nkr; i++) {
            kr_ty[i] = type_default_int();
            kr_set[i] = 0;
        }
        while (peek(p)->kind != TK_LBRACE && peek(p)->kind != TK_SEMICOLON
               && peek(p)->kind != TK_EOF
               && (is_type_start(p, p->pos)
                   || (peek(p)->kind == TK_IDENT
                       && (strcmp(peek(p)->text, "register") == 0
                           || strcmp(peek(p)->text, "auto") == 0)))) {
            if (peek(p)->kind == TK_IDENT
                && (strcmp(peek(p)->text, "register") == 0
                    || strcmp(peek(p)->text, "auto") == 0))
                advance(p);
            Type base = parse_specifiers(p);
            for (;;) {
                char *dname = NULL;
                Type ty = parse_declarator(p, type_clone(base), &dname);
                if (!dname) {
                    const Token *nm = peek(p);
                    if (nm->kind != TK_IDENT) {
                        die_at(nm->loc.file, nm->loc.line, nm->loc.col,
                               "expected parameter name but got '%s'", nm->text);
                    }
                    dname = xstrdup(nm->text);
                    advance(p);
                }
                if (ty.kind == TY_ARRAY && ty.elem_type) {
                    Type ptr = type_make_ptr(*ty.elem_type);
                    type_free(&ty);
                    ty = ptr;
                }
                int found = 0;
                for (int i = 0; i < nkr; i++) {
                    if (strcmp(kr_names[i], dname) == 0) {
                        if (kr_set[i]) type_free(&kr_ty[i]);
                        kr_ty[i] = ty;
                        kr_set[i] = 1;
                        found = 1;
                        break;
                    }
                }
                if (!found) type_free(&ty);
                free(dname);
                if (peek(p)->kind == TK_COMMA) { advance(p); continue; }
                break;
            }
            expect_kind(p, TK_SEMICOLON, "';'");
            type_free(&base);
        }
        for (int i = 0; i < nkr; i++) {
            param_array_push(&fn.params, kr_names[i], kr_ty[i], fn.loc);
            free(kr_names[i]);
        }
    }

    /* Optional GCC `__attribute__((...))` annotations (e.g.
     * `__attribute__((noreturn))` or `__attribute__((alias("target")))` or
     * `__attribute__((aligned(N)))`) that may follow the parameter list. */
    while (parse_attribute(p, &fn.align, NULL, NULL, NULL, &fn.alias_target)) {}
    if (!fn.alias_target && g_parsed_alias) {
        fn.alias_target = g_parsed_alias;
        g_parsed_alias = NULL;
    }
    if (g_parsed_no_instrument) {
        fn.no_instrument = 1;
        g_parsed_no_instrument = 0;
    }
    if (p->tu) {
        for (size_t i = 0; i < p->tu->functions.len; i++) {
            if (strcmp(p->tu->functions.data[i].name, fn.name) == 0 && p->tu->functions.data[i].no_instrument) {
                fn.no_instrument = 1;
                break;
            }
        }
    }

    if (fn.is_extern || peek(p)->kind == TK_SEMICOLON || peek(p)->kind == TK_COMMA) {
        /* Declaration only / forward declaration — no body. */
        fn.is_extern = 1;
        while (peek(p)->kind == TK_COMMA) {
            advance(p); /* consume ',' */
            for (;;) { if (!skip_attribute(p)) break; }
            const Token *next_name = peek(p);
            if (next_name->kind != TK_IDENT) break;
            advance(p);
            FunctionDecl extra_fn;
            memset(&extra_fn, 0, sizeof(extra_fn));
            extra_fn.name = xstrdup(next_name->text);
            extra_fn.ret_type = type_clone(ret_ty);
            param_array_init(&extra_fn.params);
            stmt_array_init(&extra_fn.body);
            extra_fn.loc = next_name->loc;
            extra_fn.is_extern = 1;
            extra_fn.is_static = is_static;
            extra_fn.no_instrument = fn.no_instrument;
            if (peek(p)->kind == TK_LPAREN) {
                advance(p);
                while (peek(p)->kind != TK_RPAREN && peek(p)->kind != TK_EOF) advance(p);
                if (peek(p)->kind == TK_RPAREN) advance(p);
            }
            if (p->tu->functions.len >= p->tu->functions.cap) {
                size_t new_cap = p->tu->functions.cap ? p->tu->functions.cap * 2 : 4;
                p->tu->functions.data = realloc(p->tu->functions.data,
                                             new_cap * sizeof(FunctionDecl));
                p->tu->functions.cap = new_cap;
            }
            p->tu->functions.data[p->tu->functions.len++] = extra_fn;
        }
        expect_kind(p, TK_SEMICOLON, "';'");
        return fn;
    }

    expect_kind(p, TK_LBRACE, "'{'");

    const char *saved_fn = p->cur_fn_name;
    size_t saved_td = p->typedef_mark;
    p->cur_fn_name = fn.name;
    p->typedef_mark = p->tu->typedefs.len;
    parse_stmt_list(p, &fn.body);
    typedef_registry_truncate(&p->tu->typedefs, p->typedef_mark);
    p->cur_fn_name = saved_fn;
    p->typedef_mark = saved_td;

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

static int is_definition_only_lookahead(const Parser *p) {
    if (p->tokens->data[p->pos].kind != TK_LBRACE) return 0;
    size_t pos = p->pos + 1;
    int depth = 1;
    while (pos < p->tokens->len && depth > 0) {
        if (p->tokens->data[pos].kind == TK_LBRACE) depth++;
        else if (p->tokens->data[pos].kind == TK_RBRACE) depth--;
        pos++;
    }
    return (pos < p->tokens->len && p->tokens->data[pos].kind == TK_SEMICOLON);
}

void parse_in_pkg(const TokenArray *tokens, TranslationUnit *tu, PkgContext *ctx) {
    Parser p;
    p.tokens = tokens;
    p.pos = 0;
    p.tu = tu;
    p.pkg_ctx = ctx;
    stmt_array_init(&p.prepend);
    p.anon_counter = 0;
    p.cur_fn_name = NULL;
    p.typedef_mark = 0;

    g_parser_tu = tu;
    /* must start with package declaration */
    if (peek(&p)->kind != TK_KW_PACKAGE) {
        const Token *t = peek(&p);
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "expected 'package' declaration at start of file");
    }

    tu->package = parse_package_decl(&p);

    /* Zero or more `import IDENT;` — each eagerly loads that package. */
    while (peek(&p)->kind == TK_KW_IMPORT) {
        const Token *kw = peek(&p);
        advance(&p);
        const Token *ident = peek(&p);
        if (ident->kind != TK_IDENT) {
            die_at(ident->loc.file, ident->loc.line, ident->loc.col,
                   "expected package name after 'import'");
        }
        advance(&p);
        expect_kind(&p, TK_SEMICOLON, "';'");
        if (!ctx) {
            die_at(kw->loc.file, kw->loc.line, kw->loc.col,
                   "'import' requires a package search path (driver bug)");
        }
        for (size_t i = 0; i < tu->imports.len; i++) {
            if (strcmp(tu->imports.data[i].name, ident->text) == 0) {
                die_at(kw->loc.file, kw->loc.line, kw->loc.col,
                       "duplicate import of package '%s'", ident->text);
            }
        }
        import_array_push(&tu->imports, ident->text, kw->loc);
        pkg_load(ctx, ident->text, kw->loc);
    }

    /* At file scope: each top-level declaration starts with a type OR
     * with `struct TAG { ... };` — a struct definition (no variable). */
    while (peek(&p)->kind != TK_EOF) {
        if (skip_attribute(&p)) continue;
        if (peek(&p)->kind == TK_KW_INLINE) { advance(&p); continue; }
        if (peek(&p)->kind == TK_SEMICOLON) {
            advance(&p);
            continue;
        }
        /* Flush any multi-declarator trailers queued by the previous stmt. */
        for (size_t i = 0; i < p.prepend.len; i++)
            stmt_array_push(&tu->globals, p.prepend.data[i]);
        p.prepend.len = 0;
        /* Struct definition: `struct TAG { type NAME [ [N] ]* ; ... };`
         * Distinguish from `struct TAG x;` global by lookahead — after
         * `struct TAG` we expect `{` for a definition. */
        if (peek(&p)->kind == TK_KW_STRUCT) {
            size_t save = p.pos;
            advance(&p);
            const Token *tag = peek(&p);
            if (tag->kind == TK_IDENT) advance(&p);
            /* A definition requires an IDENT tag followed by `{`, and terminated by `;`.
             * If declarators follow the struct body, fall through to declaration path. */
            if (tag->kind == TK_IDENT && peek(&p)->kind == TK_LBRACE && is_definition_only_lookahead(&p)) {
                /* Struct definition. */
                if (struct_registry_find(&tu->structs, tag->text)) {
                    die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                           "redefinition of struct '%s'", tag->text);
                }
                StructDef *sd = struct_registry_add(&tu->structs, tag->text, tag->loc);
                parse_struct_body(&p, sd);
                expect_kind(&p, TK_SEMICOLON, "';'");
                continue;
            } else {
                /* Not a definition-only — reset and fall through to declaration. */
                p.pos = save;
            }
        }

        /* Union definition: `union TAG { type NAME [ [N] ]* ; ... };`
         * Distinguish from `union TAG var;` global by lookahead — after
         * `union TAG` we expect `{` for a definition. */
        if (peek(&p)->kind == TK_KW_UNION) {
            size_t save = p.pos;
            advance(&p);
            const Token *tag = peek(&p);
            if (tag->kind == TK_IDENT) advance(&p);
            if (tag->kind == TK_IDENT && peek(&p)->kind == TK_LBRACE && is_definition_only_lookahead(&p)) {
                /* Union definition. */
                if (struct_registry_find(&tu->structs, tag->text)) {
                    die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                           "redefinition of '%s'", tag->text);
                }
                StructDef *sd = struct_registry_add(&tu->structs, tag->text, tag->loc);
                sd->is_union = 1;
                parse_struct_body(&p, sd);
                expect_kind(&p, TK_SEMICOLON, "';'");
                continue;
            } else {
                /* Not a definition-only — reset and fall through to declaration. */
                p.pos = save;
            }
        }

        /* Enum definition: `enum TAG { IDENT [= expr], ... };`
         * Distinguish from `enum TAG var;` global by lookahead — after
         * `enum TAG` we expect `{` for a definition. */
        if (peek(&p)->kind == TK_KW_ENUM) {
            size_t save = p.pos;
            advance(&p);
            const Token *tag = peek(&p);
            if (tag->kind == TK_IDENT) advance(&p);
            if (tag->kind == TK_IDENT && peek(&p)->kind == TK_LBRACE && is_definition_only_lookahead(&p)) {
                /* Enum definition. */
                if (enum_registry_find(&tu->enums, tag->text)) {
                    die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                           "redefinition of enum '%s'", tag->text);
                }
                EnumDef *ed = enum_registry_add(&tu->enums, tag->text, tag->loc);
                parse_enum_body(&p, ed);
                expect_kind(&p, TK_SEMICOLON, "';'");
                continue;
            } else {
                /* Not a definition-only — reset and fall through to declaration. */
                p.pos = save;
            }
        }

        /* Typedef at file scope: `typedef <type> <name>;`.  parse_typedef_stmt
         * registers the alias and returns an empty no-op ST_BLOCK, which we
         * simply discard here (it carries no runtime meaning). */
        if (peek(&p)->kind == TK_KW_TYPEDEF) {
            Stmt s = parse_typedef_stmt(&p);
            stmt_free(&s);
            continue;
        }

        if (is_function_declaration_lookahead(&p)) {
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
            if (s.kind == ST_BLOCK) {
                for (size_t k = 0; k < s.u.block.len; k++) {
                    if (s.u.block.data[k].kind == ST_DECL) {
                        stmt_array_push(&tu->globals, s.u.block.data[k]);
                    } else {
                        stmt_free(&s.u.block.data[k]);
                    }
                }
                free(s.u.block.data);
            } else if (s.kind != ST_DECL) {
                die_at(s.loc.file, s.loc.line, s.loc.col,
                       "only variable declarations allowed at file scope (got stmt kind %d, next token '%s')",
                       s.kind, peek(&p)->text ? peek(&p)->text : "NULL");
            } else {
                stmt_array_push(&tu->globals, s);
            }
        }
    }
    /* Flush any trailing queued declarators and free the prepend buffer. */
    for (size_t i = 0; i < p.prepend.len; i++)
        stmt_array_push(&tu->globals, p.prepend.data[i]);
    p.prepend.len = 0;
    stmt_array_free(&p.prepend);
    g_parser_tu = NULL;
}

void parse(const TokenArray *tokens, TranslationUnit *tu) {
    parse_in_pkg(tokens, tu, NULL);
}
