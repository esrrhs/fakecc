#include "fakecc/parser.h"
#include "fakecc/common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Parser state                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    const TokenArray *tokens;
    size_t pos;
    TranslationUnit *tu;   /* backing TU — parser writes structs directly */
    /* Prepended statements: multi-declarators (`int a, b;`) parse as several
     * ST_DECLs but a statement slot holds one — the trailing declarators are
     * queued here and flushed into the enclosing stmt-list before the next
     * real statement, so all names land in the same (current) scope. */
    StmtArray prepend;
    /* Counter for generating unique tags for anonymous structs/unions/enums
     * (`__anon_0`, `__anon_1`, ...).  Each anonymous definition gets a fresh
     * tag so its type is distinct (C semantics). */
    int anon_counter;
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

static int skip_attribute(Parser *p) {
    if (peek(p)->kind != TK_IDENT
        || strcmp(peek(p)->text, "__attribute__") != 0)
        return 0;
    advance(p);  /* consume `__attribute__` */
    if (peek(p)->kind != TK_LPAREN) return 0;
    advance(p);  /* consume outer `(` */
    if (peek(p)->kind != TK_LPAREN) {
        /* Malformed: `__attribute__(...)` without the inner parens — but the
         * canonical form is `__attribute__((...))`, so require the inner `(`. */
        return 0;
    }
    advance(p);  /* consume inner `(` */
    /* Skip the balanced parenthesized group (attributes may contain nested
     * parens and arbitrary tokens).  Two parens are already open. */
    int depth = 2;
    while (depth > 0 && peek(p)->kind != TK_EOF) {
        if (peek(p)->kind == TK_LPAREN) depth++;
        else if (peek(p)->kind == TK_RPAREN) depth--;
        advance(p);
    }
    return 1;
}

/* Recognize an integer type at the current position.
 *   [signed|unsigned] (char|short|int|long)
 * Also accepts bare "signed" / "unsigned" (= int).
 * Returns Type; emits a diagnostic if no type keyword is present. */
static int is_type_start(const Parser *p, size_t pos) {
    TokenKind k = p->tokens->data[pos].kind;
    if (k == TK_KW_VOID || k == TK_KW_INT || k == TK_KW_CHAR || k == TK_KW_SHORT
        || k == TK_KW_LONG || k == TK_KW_SIGNED || k == TK_KW_UNSIGNED
        || k == TK_KW_FLOAT || k == TK_KW_DOUBLE || k == TK_KW_BOOL
        || k == TK_KW_STRUCT || k == TK_KW_ENUM || k == TK_KW_UNION
        || k == TK_KW_CONST || k == TK_KW_STATIC || k == TK_KW_EXTERN
        || k == TK_KW_VOLATILE || k == TK_KW_RESTRICT || k == TK_KW_INLINE)
        return 1;
    /* A typedef name looks like an ordinary identifier. */
    if (k == TK_IDENT
        && typedef_registry_find(&p->tu->typedefs, p->tokens->data[pos].text))
        return 1;
    return 0;
}

/* Parse specifiers: const + base type (void/struct/union/enum/typedef/int).
 * This is the old `parse_type` minus the trailing `*` chain — pointers and
 * other declarator suffixes are handled separately by `parse_declarator`. */
static Type parse_specifiers(Parser *p) {
    /* Type qualifiers — flag the resulting type.  `const` gates assignment in
     * sema; `volatile`/`restrict` are no-ops without an optimizer (stored for
     * completeness).  All three may appear in any order (C permits mixing). */
    int is_const = 0, is_volatile = 0, is_restrict = 0;
    for (;;) {
        if (peek(p)->kind == TK_KW_CONST) { is_const = 1; advance(p); }
        else if (peek(p)->kind == TK_KW_VOLATILE) { is_volatile = 1; advance(p); }
        else if (peek(p)->kind == TK_KW_RESTRICT) { is_restrict = 1; advance(p); }
        else if (peek(p)->kind == TK_KW_INLINE) { advance(p); }  /* no-op hint */
        else break;
    }
    /* void — only meaningful as a return type or as void* (pointer to void).
     * A lone `void` variable is rejected later in sema. */
    if (peek(p)->kind == TK_KW_VOID) {
        advance(p);
        Type t = type_make_void();
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    /* struct [Tag] — if Tag is present, it's a use of a (possibly forward)
     * struct; look it up in the registry.  If the next token is `{` (no tag),
     * it's an ANONYMOUS struct definition (`typedef struct { ... } Name;`):
     * generate a unique tag, parse the body, and register it. */
    if (peek(p)->kind == TK_KW_STRUCT) {
        advance(p);
        if (peek(p)->kind == TK_LBRACE) {
            /* Anonymous struct definition. */
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            StructDef *sd = struct_registry_add(&p->tu->structs, tag, peek(p)->loc);
            parse_struct_body(p, sd);
            /* parse_struct_body may realloc the registry (nested anonymous
             * structs/unions), invalidating sd — re-fetch before reading size. */
            sd = struct_registry_find(&p->tu->structs, tag);
            Type t = type_make_struct(tag, sd ? sd->size : 0);
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected struct tag but got '%s'", tag->text);
        }
        advance(p);
        /* If '{' follows, this is a struct definition at use site
         * (`struct Tag { ... }`), not just a forward reference. */
        if (peek(p)->kind == TK_LBRACE) {
            if (struct_registry_find(&p->tu->structs, tag->text)) {
                die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                       "redefinition of struct '%s'", tag->text);
            }
            StructDef *sd = struct_registry_add(&p->tu->structs, tag->text, peek(p)->loc);
            parse_struct_body(p, sd);
            sd = struct_registry_find(&p->tu->structs, tag->text);
            Type t = type_make_struct(tag->text, sd ? sd->size : 0);
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
        StructDef *sd = struct_registry_find(&p->tu->structs, tag->text);
        int size = sd ? sd->size : 0;
        Type t = type_make_struct(tag->text, size);
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    /* union [Tag] — same as struct: a Tag is a use, `{` begins an anonymous
     * union definition. */
    if (peek(p)->kind == TK_KW_UNION) {
        advance(p);
        if (peek(p)->kind == TK_LBRACE) {
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            StructDef *sd = struct_registry_add(&p->tu->structs, tag, peek(p)->loc);
            sd->is_union = 1;
            parse_struct_body(p, sd);
            sd = struct_registry_find(&p->tu->structs, tag);
            Type t = type_make_struct(tag, sd ? sd->size : 0);
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected union tag but got '%s'", tag->text);
        }
        advance(p);
        /* If '{' follows, this is a union definition at use site
         * (`union Tag { ... }`), not just a forward reference. */
        if (peek(p)->kind == TK_LBRACE) {
            if (struct_registry_find(&p->tu->structs, tag->text)) {
                die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                       "redefinition of union '%s'", tag->text);
            }
            StructDef *sd = struct_registry_add(&p->tu->structs, tag->text, peek(p)->loc);
            sd->is_union = 1;
            parse_struct_body(p, sd);
            sd = struct_registry_find(&p->tu->structs, tag->text);
            Type t = type_make_struct(tag->text, sd ? sd->size : 0);
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
        StructDef *sd = struct_registry_find(&p->tu->structs, tag->text);
        int size = sd ? sd->size : 0;
        Type t = type_make_struct(tag->text, size);
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    /* float / double — TY_FLOAT with width 4 or 8. */
    if (peek(p)->kind == TK_KW_FLOAT) {
        advance(p);
        Type t = type_make_float(4);
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    if (peek(p)->kind == TK_KW_DOUBLE) {
        advance(p);
        Type t = type_make_float(8);
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    /* long double — TY_FLOAT with width 16 (x87 80-bit extended).  Detected by
     * a `long` keyword immediately followed by `double` (lookahead without
     * consuming on mismatch, since `long` alone begins an integer type). */
    if (peek(p)->kind == TK_KW_LONG
        && p->pos + 1 < p->tokens->len
        && p->tokens->data[p->pos + 1].kind == TK_KW_DOUBLE) {
        advance(p); /* consume `long` */
        advance(p); /* consume `double` */
        Type t = type_make_float(16);
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
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
            Type t = type_default_int();
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected enum tag but got '%s'", tag->text);
        }
        advance(p);
        Type t = type_default_int();
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }
    /* typedef name — resolve to the aliased type. */
    if (peek(p)->kind == TK_IDENT) {
        const Type *alias = typedef_registry_find(&p->tu->typedefs, peek(p)->text);
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
            t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
            return t;
        }
    }
    int is_unsigned = 0;
    int saw_sign = 0;
    int width = -1;

    /* _Bool — standalone, takes no signed/unsigned/long modifier. */
    if (peek(p)->kind == TK_KW_BOOL) {
        advance(p);
        Type t = type_make_bool();
        t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
        return t;
    }

    TokenKind k = peek(p)->kind;
    if (k == TK_KW_SIGNED)    { is_unsigned = 0; saw_sign = 1; advance(p); }
    else if (k == TK_KW_UNSIGNED) { is_unsigned = 1; saw_sign = 1; advance(p); }

    k = peek(p)->kind;
    switch (k) {
    case TK_KW_CHAR:  advance(p); width = 1; break;
    case TK_KW_SHORT: advance(p); width = 2; break;
    case TK_KW_INT:   advance(p); width = 4; break;
    case TK_KW_LONG:
        advance(p); width = 8;
        /* `long long` (and `long long int`) — consume a second `long`. */
        if (peek(p)->kind == TK_KW_LONG) advance(p);
        /* `long int` / `long long int` — consume a trailing `int`. */
        if (peek(p)->kind == TK_KW_INT) advance(p);
        break;
    default:
        if (saw_sign) { width = 4; break; }
        {
            const Token *t = peek(p);
            die_at(t->loc.file, t->loc.line, t->loc.col,
                   "expected type but got '%s'", t->text);
        }
    }
    Type t = type_make_int(width, is_unsigned);
    t.is_const = is_const; t.is_volatile = is_volatile; t.is_restrict = is_restrict;
    return t;
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
        Type base = parse_specifiers(p);
        /* Re-fetch sd: parsing `base` may have defined a nested struct. */
        sd = struct_registry_find(&p->tu->structs, tag);
        /* parse_specifiers may have consumed an inline struct/union definition
         * body (`struct Tag { ... }` / `union Tag { ... }`).  If the next token
         * is ';', this is a standalone type definition, not a member declaration
         * — consume the ';' and continue to the next member. */
        if (peek(p)->kind == TK_SEMICOLON) {
            advance(p);
            type_free(&base);
            continue;
        }
        for (;;) {
            char *mname = NULL;
            Type mty = parse_declarator(p, type_clone(base), &mname);
            sd = struct_registry_find(&p->tu->structs, tag);
            if (!mname) {
                const Token *mn = peek(p);
                if (!is_name_token(mn->kind)) {
                    die_at(mn->loc.file, mn->loc.line, mn->loc.col,
                           "expected member name but got '%s'", mn->text);
                }
                mname = xstrdup(mn->text);
                advance(p);
            }
            int bit_width = 0;
            if (peek(p)->kind == TK_COLON) {
                /* Bitfield `name : N;`.  Consume the width (an int literal). */
                advance(p);
                const Token *w = peek(p);
                if (w->kind != TK_INT_LITERAL) {
                    die_at(w->loc.file, w->loc.line, w->loc.col,
                           "expected bitfield width but got '%s'", w->text);
                }
                bit_width = int_literal_value(w->text);
                advance(p);
            }
            struct_def_push_member(sd, mname, mty, bit_width);
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
            int sign = 1;
            /* Leading unary +/- on the value: `enum { A = -1 };`. */
            if (peek(p)->kind == TK_MINUS) { sign = -1; advance(p); }
            else if (peek(p)->kind == TK_PLUS) { advance(p); }
            const Token *v = peek(p);
            if (v->kind == TK_INT_LITERAL) {
                has_value = 1; value = sign * int_literal_value(v->text); advance(p);
            } else if (v->kind == TK_IDENT) {
                const EnumConstant *ec =
                    enum_registry_find_constant(&p->tu->enums, v->text);
                if (!ec) {
                    die_at(v->loc.file, v->loc.line, v->loc.col,
                           "enum value '%s' is not a constant", v->text);
                }
                has_value = 1; value = ec->value; advance(p);
            } else {
                die_at(v->loc.file, v->loc.line, v->loc.col,
                       "expected integer value for enum constant '%s'", cn->text);
            }
        }
        enum_def_push_constant(ed, cn->text, has_value, value, cn->loc);
        if (peek(p)->kind == TK_COMMA) advance(p);
    }
    expect_kind(p, TK_RBRACE, "'}'");
}

static Type make_func_type(Type ret, ParamArray *params) {
    /* Build a shallow array of pointers into the ParamArray's owned types.
     * type_make_func deep-clones them, so the originals stay owned by the
     * ParamArray and are freed by param_array_free below. */
    Type **ptys = NULL;
    if (params->len > 0) {
        ptys = malloc(params->len * sizeof(Type *));
        if (!ptys) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        for (size_t i = 0; i < params->len; i++)
            ptys[i] = &params->data[i].type;
    }
    Type t = type_make_func(ret, ptys, (int)params->len);
    free(ptys);
    param_array_free(params);
    return t;
}

/* Parse a parameter list: (void) means empty, else type declarator pairs.
 * Returns the collected params.  Tolerates a trailing comma. */
static ParamArray parse_param_list(Parser *p) {
    ParamArray params;
    param_array_init(&params);
    if (peek(p)->kind == TK_KW_VOID
        && p->tokens->data[p->pos + 1].kind == TK_RPAREN) {
        advance(p);  /* consume `void` */
        return params;  /* empty */
    }
    if (peek(p)->kind != TK_RPAREN) {
        for (;;) {
            if (!is_type_start(p, p->pos)) {
                const Token *t = peek(p);
                die_at(t->loc.file, t->loc.line, t->loc.col,
                       "expected type for parameter but got '%s'", t->text);
            }
            char *pname_in = NULL;
            Type pty = parse_type(p, &pname_in);
            const Token *pname = peek(p);
            if (!pname_in) {
                /* Unnamed parameter — the declarator produced no name
                 * (e.g. `int`, `int*`).  Synthesize a name. */
                pname_in = xstrdup("");
            }
            param_array_push(&params, pname_in, pty, pname->loc);
            free(pname_in);
            if (peek(p)->kind == TK_COMMA) {
                advance(p);
                /* Variadic tail of a function type. The variadic-ness of a
                 * type is deferred (indirect variadic calls are out of scope);
                 * here we just consume the `...` so the syntax parses. */
                if (peek(p)->kind == TK_ELLIPSIS) {
                    advance(p);
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
static int int_literal_value(const char *text);
/* Parse a constant array dimension size: an int literal or an enum constant
 * (e.g. `arr[MAX]` where MAX is an enum const).  Consumes the size token.
 * Returns the value (0 if absent). */
static int parse_array_size(Parser *p) {
    if (peek(p)->kind == TK_INT_LITERAL) {
        int v = int_literal_value(peek(p)->text);
        advance(p);
        return v;
    }
    if (peek(p)->kind == TK_IDENT) {
        const EnumConstant *ec =
            enum_registry_find_constant(&p->tu->enums, peek(p)->text);
        if (ec) { advance(p); return ec->value; }
    }
    return 0;
}
static void parse_group_content(Parser *p, char **name_out, int *out_ptrs,
                                int *out_array_len, int *out_has_array) {
    int ptrs = 0;
    while (peek(p)->kind == TK_STAR) { advance(p); ptrs++; }
    if (peek(p)->kind == TK_IDENT) {
        *name_out = xstrdup(peek(p)->text);
        advance(p);
    } else {
        *name_out = NULL;  /* abstract declarator */
    }
    int array_len = 0;
    int has_array = 0;
    if (peek(p)->kind == TK_LBRACKET) {
        advance(p);
        array_len = parse_array_size(p);
        expect_kind(p, TK_RBRACKET, "']'");
        has_array = 1;
    }
    *out_ptrs = ptrs;
    *out_array_len = array_len;
    *out_has_array = has_array;
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
    enum { MAX_PTRS = 8 };
    int ptr_const[MAX_PTRS], ptr_volatile[MAX_PTRS], ptr_restrict[MAX_PTRS];
    int ptrs = 0;
    while (peek(p)->kind == TK_STAR) {
        advance(p);
        int c = 0, v = 0, r = 0;
        while (peek(p)->kind == TK_KW_CONST
               || peek(p)->kind == TK_KW_VOLATILE
               || peek(p)->kind == TK_KW_RESTRICT) {
            if (peek(p)->kind == TK_KW_CONST) c = 1;
            else if (peek(p)->kind == TK_KW_VOLATILE) v = 1;
            else r = 1;
            advance(p);
        }
        if (ptrs >= MAX_PTRS) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                     peek(p)->loc.col, "too many pointer levels");
        ptr_const[ptrs] = c; ptr_volatile[ptrs] = v; ptr_restrict[ptrs] = r;
        ptrs++;
    }

    Type t;
    if (peek(p)->kind == TK_LPAREN) {
        /* Grouping: '(' group_content ')'.  The group content's modifiers
         * (prefix `*`, array `[N]`) wrap the group's core type. */
        advance(p);  /* '(' */
        char *inner_name = NULL;
        int inner_ptrs = 0, inner_array_len = 0, inner_has_array = 0;
        parse_group_content(p, &inner_name, &inner_ptrs, &inner_array_len,
                            &inner_has_array);
        *name_out = inner_name;
        expect_kind(p, TK_RPAREN, "')'");
        /* Parse the postfix that follows the group (array/function). */
        t = base;
        /* Collect array dims and optional function suffix.  Arrays wrap
         * right-to-left (so `int a[3][2]` → array(3, array(2, int))). */
        int dims[8], ndims = 0;
        while (peek(p)->kind == TK_LBRACKET || peek(p)->kind == TK_LPAREN) {
            if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                int len = parse_array_size(p);
                expect_kind(p, TK_RBRACKET, "']'");
                if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                       peek(p)->loc.col, "too many array dimensions");
                dims[ndims++] = len;
            } else {
                advance(p);
                ParamArray params = parse_param_list(p);
                expect_kind(p, TK_RPAREN, "')'");
                t = make_func_type(base, &params);
                /* A function result cannot be further arrayed in valid C. */
                break;
            }
        }
        /* Wrap arrays right-to-left around the core type. */
        for (int i = ndims - 1; i >= 0; i--) {
            Type wrapped = type_make_array(t, dims[i]);
            type_free(&t);
            t = wrapped;
        }
        /* Apply the group content's modifiers (they wrap the core type). */
        for (int i = 0; i < inner_ptrs; i++) t = type_make_ptr(t);
        if (inner_has_array) t = type_make_array(t, inner_array_len);
    } else if (peek(p)->kind == TK_IDENT) {
        *name_out = xstrdup(peek(p)->text);
        advance(p);
        t = base;
        /* Postfix: array [] or function ().  Collect dims, wrap right-to-left. */
        int dims[8], ndims = 0;
        while (peek(p)->kind == TK_LBRACKET || peek(p)->kind == TK_LPAREN) {
            if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                int len = parse_array_size(p);
                expect_kind(p, TK_RBRACKET, "']'");
                if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                       peek(p)->loc.col, "too many array dimensions");
                dims[ndims++] = len;
            } else {
                advance(p);
                ParamArray params = parse_param_list(p);
                expect_kind(p, TK_RPAREN, "')'");
                t = make_func_type(base, &params);
                break;
            }
        }
        for (int i = ndims - 1; i >= 0; i--) {
            Type wrapped = type_make_array(t, dims[i]);
            type_free(&t);
            t = wrapped;
        }
    } else {
        /* Abstract declarator (no name) — for casts/sizeof. */
        *name_out = NULL;
        t = base;
        int dims[8], ndims = 0;
        while (peek(p)->kind == TK_LBRACKET || peek(p)->kind == TK_LPAREN) {
            if (peek(p)->kind == TK_LBRACKET) {
                advance(p);
                int len = parse_array_size(p);
                expect_kind(p, TK_RBRACKET, "']'");
                if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                       peek(p)->loc.col, "too many array dimensions");
                dims[ndims++] = len;
            } else {
                advance(p);
                ParamArray params = parse_param_list(p);
                expect_kind(p, TK_RPAREN, "')'");
                t = make_func_type(base, &params);
                break;
            }
        }
        for (int i = ndims - 1; i >= 0; i--) {
            Type wrapped = type_make_array(t, dims[i]);
            type_free(&t);
            t = wrapped;
        }
    }

    /* Apply prefix pointers (they wrap the whole thing), innermost first. */
    for (int i = 0; i < ptrs; i++)
        t = ptr_wrap(t, ptr_const[i], ptr_volatile[i], ptr_restrict[i]);
    return t;
}

/* Parse a full type: specifiers + declarator.  This replaces the old
 * `parse_type` for all call sites that need a named type.  *name_out receives
 * the declared name (owned, freed by caller) or NULL for abstract declarators. */
static Type parse_type(Parser *p, char **name_out) {
    Type base = parse_specifiers(p);
    return parse_declarator(p, base, name_out);
}

/* Parse an abstract type (no name): specifiers + trailing `*` chain.  Used
 * for function return types, casts, and sizeof where the declarator must NOT
 * consume an identifier (the name belongs to the function/var, not the type).
 * This mirrors the old `parse_type` — it parses the specifiers and any `*`
 * pointers glued to them, but stops before an identifier.  (Full declarators
 * with grouping/array/function suffixes for return types — e.g. a function
 * returning a function pointer — are beyond this slice.) */
static Type parse_type_abstract(Parser *p) {
    Type base = parse_specifiers(p);
    Type t = base;
    while (peek(p)->kind == TK_STAR) {
        advance(p);
        Type w = type_make_ptr(t);
        type_free(&t);
        t = w;
    }
    return t;
}

/* Parse a type-name (specifiers + abstract declarator): the type in a
 * compound literal `(Type){ ... }`.  Supports a `*` chain and/or array
 * dimensions `[int]` / `[]`, but NOT function types (illegal in a compound
 * literal).  Arrays wrap right-to-left so `int[3][2]` → array(3, array(2)). */
static Type parse_type_name(Parser *p) {
    Type base = parse_specifiers(p);
    /* Prefix pointers. */
    int ptrs = 0;
    while (peek(p)->kind == TK_STAR) { advance(p); ptrs++; }
    /* Postfix array dimensions. */
    int dims[8], ndims = 0;
    while (peek(p)->kind == TK_LBRACKET) {
        advance(p);
        int len = parse_array_size(p);
        expect_kind(p, TK_RBRACKET, "']'");
        if (ndims >= 8) die_at(peek(p)->loc.file, peek(p)->loc.line,
                                   peek(p)->loc.col, "too many array dimensions");
        dims[ndims++] = len;
    }
    if (ptrs == 0 && ndims == 0)
        return base; /* plain scalar/struct type */
    /* Wrap arrays right-to-left around the base type. */
    Type t = base;
    for (int i = ndims - 1; i >= 0; i--) {
        Type w = type_make_array(t, dims[i]);
        type_free(&t);
        t = w;
    }
    /* Apply prefix pointers around the whole thing. */
    for (int i = 0; i < ptrs; i++) {
        Type w = type_make_ptr(t);
        type_free(&t);
        t = w;
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

/* unary-expr = ("+"|"-"|"&"|"*"|"~") unary-expr | sizeof unary-or-type | primary { postfix } */
static Expr *parse_unary(Parser *p) {
    TokenKind k = peek(p)->kind;
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
        /* _Alignof(T) — C standard _Alignof takes a type-name only. */
        if (peek(p)->kind != TK_LPAREN)
            die_at(peek(p)->loc.file, peek(p)->loc.line, peek(p)->loc.col,
                   "expected '(' after '_Alignof'");
        advance(p);
        Type t = parse_type_abstract(p);
        expect_kind(p, TK_RPAREN, "')'");
        Expr *e = expr_new_alignof_type(t, loc);
        type_free(&t);
        return e;
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
                             && strcmp(lhs->u.var.name, "va_arg") == 0);
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
        Expr *e = expr_new_int(int_literal_value(t->text), t->loc);
        advance(p);
        return parse_postfix(p, e);
    }
    if (t->kind == TK_FLOAT_LITERAL) {
        int width = 8;
        float_literal_width(t->text, &width); /* classify width (4/8/16) */
        Expr *e = expr_new_float_lit(t->text, width, t->loc);
        advance(p);
        return e;  /* float literal is a primary — no postfix needed */
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
                    case '0':  seg[slen2++] = '\0'; break;
                    case '\\': seg[slen2++] = '\\'; break;
                    case '"':  seg[slen2++] = '"'; break;
                    case '\'': seg[slen2++] = '\''; break;
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
        /* A call `IDENT(args)` is built by parse_postfix so that indirect
         * calls (`fp(x)`, `(*fp)(x)`, `arr[i](x)`) parse uniformly. */
        return parse_postfix(p, expr_new_var(ident->text, ident->loc));
    }
    if (t->kind == TK_LPAREN) {
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
        int kind = -1, idx = -1;
        char *member = NULL;
        if (peek(p)->kind == TK_DOT) {
            /* .field = expr */
            advance(p);
            const Token *fn = peek(p);
            if (fn->kind != TK_IDENT)
                die_at(fn->loc.file, fn->loc.line, fn->loc.col,
                       "expected field name after '.' but got '%s'", fn->text);
            advance(p);
            member = xstrdup(fn->text);
            expect_kind(p, TK_ASSIGN, "'='");
            kind = 1;
        } else if (peek(p)->kind == TK_LBRACKET) {
            /* [index] = expr — index must be an integer literal */
            advance(p);
            const Token *ix = peek(p);
            if (ix->kind != TK_INT_LITERAL)
                die_at(ix->loc.file, ix->loc.line, ix->loc.col,
                       "expected integer constant in designator but got '%s'",
                       ix->text);
            idx = int_literal_value(ix->text);
            advance(p);
            expect_kind(p, TK_RBRACKET, "']'");
            expect_kind(p, TK_ASSIGN, "'='");
            kind = 0;
        }
        Expr *elem;
        if (peek(p)->kind == TK_LBRACE) {
            elem = parse_init_list(p);
        } else {
            elem = parse_assign(p);
        }
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

/* stmt = decl-stmt | return-stmt | if-stmt | while-stmt | block | expr-stmt */
static Stmt parse_stmt(Parser *p) {
    TokenKind k = peek(p)->kind;
    /* `typedef` at block scope is parsed as a decl-stmt whose init is NULL;
     * sema registers the alias.  A leading typedef-name identifier is treated
     * as a type start (declaration), matching C's "lexer hack" resolution. */
    if (k == TK_KW_TYPEDEF || is_type_start(p, p->pos)) {
        /* decl-stmt: [typedef] type IDENT [ "[" N "]" ]* ["=" expr] ";" */
        if (k == TK_KW_TYPEDEF) {
            return parse_typedef_stmt(p);
        }
        SourceLoc decl_loc = peek(p)->loc;
        /* Storage class: `static` (persistent) / `extern` (declaration only).
         * `const` is handled inside parse_specifiers. */
        int storage_class = 0; /* 0=default, 1=static, 2=extern */
        if (peek(p)->kind == TK_KW_STATIC) {
            storage_class = 1;
            advance(p);
        } else if (peek(p)->kind == TK_KW_EXTERN) {
            storage_class = 2;
            advance(p);
        }
        /* Parse the common specifier base type once; each declarator in a
         * comma-separated list (`int a, b, c`) shares this base.  Clone it for
         * each declarator because parse_declarator takes its argument by value
         * and may consume (free) its heap children along some paths. */
        Type base = parse_specifiers(p);
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
    if (k == TK_KW_SWITCH) {
        return parse_switch(p);
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
    /* `typedef struct Tag { ... } Name;` and `typedef enum Tag { ... } Name;`
     * combine a definition with a typedef.  Detect the `struct`/`enum` keyword
     * followed by an IDENT tag and a `{`; parse the body, then bind `Name`. */
    if (peek(p)->kind == TK_KW_STRUCT || peek(p)->kind == TK_KW_UNION) {
        TokenKind base = peek(p)->kind;
        advance(p);  /* consume struct/union */
        if (peek(p)->kind == TK_IDENT
            && p->tokens->data[p->pos + 1].kind == TK_LBRACE) {
            const Token *tag = peek(p);
            advance(p);  /* consume tag */
            if (struct_registry_find(&p->tu->structs, tag->text)) {
                die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                       "redefinition of '%s'", tag->text);
            }
            StructDef *sd = struct_registry_add(&p->tu->structs, tag->text, tag->loc);
            sd->is_union = (base == TK_KW_UNION);
            parse_struct_body(p, sd);
            /* Now expect the typedef name and ';'. */
            const Token *name = peek(p);
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            advance(p);
            Type ty = type_make_struct(tag->text, sd->size);
            typedef_registry_add(&p->tu->typedefs, name->text, ty);
            expect_kind(p, TK_SEMICOLON, "';'");
            Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        /* Not a definition — fall back.  We already consumed struct/union, so
         * reconstruct the type via parse_specifiers-like logic by hand.  If the
         * next token is `{`, this is an anonymous struct typedef
         * (`typedef struct { ... } Name;`). */
        if (peek(p)->kind == TK_LBRACE) {
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            StructDef *sd = struct_registry_add(&p->tu->structs, tag, peek(p)->loc);
            sd->is_union = (base == TK_KW_UNION);
            parse_struct_body(p, sd);
            const Token *name = peek(p);
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            advance(p);
            Type ty = type_make_struct(tag, sd->size);
            typedef_registry_add(&p->tu->typedefs, name->text, ty);
            expect_kind(p, TK_SEMICOLON, "';'");
            Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected struct tag but got '%s'", tag->text);
        }
        advance(p);
        StructDef *sd = struct_registry_find(&p->tu->structs, tag->text);
        int size = sd ? sd->size : 0;
        Type ty = type_make_struct(tag->text, size);
        /* Continue as a normal typedef with this base type. */
        char *decl_name = NULL;
        ty = parse_declarator(p, ty, &decl_name);
        const Token *name = peek(p);
        if (!decl_name) {
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            decl_name = xstrdup(name->text);
            advance(p);
        } else if (name->kind == TK_IDENT) {
            advance(p);
        }
        if (typedef_registry_find(&p->tu->typedefs, decl_name)) {
            die_at(name->loc.file, name->loc.line, name->loc.col,
                   "redefinition of typedef '%s'", decl_name);
        }
        typedef_registry_add(&p->tu->typedefs, decl_name, ty);
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
        stmt_array_init(&s.u.block);
        return s;
    }
    if (peek(p)->kind == TK_KW_ENUM) {
        advance(p);  /* consume enum */
        if (peek(p)->kind == TK_IDENT
            && p->tokens->data[p->pos + 1].kind == TK_LBRACE) {
            const Token *tag = peek(p);
            advance(p);  /* consume tag */
            if (enum_registry_find(&p->tu->enums, tag->text)) {
                die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                       "redefinition of enum '%s'", tag->text);
            }
            EnumDef *ed = enum_registry_add(&p->tu->enums, tag->text, tag->loc);
            parse_enum_body(p, ed);
            const Token *name = peek(p);
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            advance(p);
            typedef_registry_add(&p->tu->typedefs, name->text, type_default_int());
            expect_kind(p, TK_SEMICOLON, "';'");
            Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        /* Not a definition — fall back.  If the next token is `{`, this is an
         * anonymous enum typedef (`typedef enum { ... } Name;`). */
        if (peek(p)->kind == TK_LBRACE) {
            char tag[64];
            snprintf(tag, sizeof(tag), "__anon_%d", p->anon_counter++);
            EnumDef *ed = enum_registry_add(&p->tu->enums, tag, peek(p)->loc);
            parse_enum_body(p, ed);
            const Token *name = peek(p);
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            advance(p);
            typedef_registry_add(&p->tu->typedefs, name->text, type_default_int());
            expect_kind(p, TK_SEMICOLON, "';'");
            Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
            stmt_array_init(&s.u.block);
            return s;
        }
        const Token *tag = peek(p);
        if (tag->kind != TK_IDENT) {
            die_at(tag->loc.file, tag->loc.line, tag->loc.col,
                   "expected enum tag but got '%s'", tag->text);
        }
        advance(p);
        char *decl_name = NULL;
        Type ty = type_default_int();
        ty = parse_declarator(p, ty, &decl_name);
        const Token *name = peek(p);
        if (!decl_name) {
            if (name->kind != TK_IDENT) {
                die_at(name->loc.file, name->loc.line, name->loc.col,
                       "expected typedef name but got '%s'", name->text);
            }
            decl_name = xstrdup(name->text);
            advance(p);
        } else if (name->kind == TK_IDENT) {
            advance(p);
        }
        if (typedef_registry_find(&p->tu->typedefs, decl_name)) {
            die_at(name->loc.file, name->loc.line, name->loc.col,
                   "redefinition of typedef '%s'", decl_name);
        }
        typedef_registry_add(&p->tu->typedefs, decl_name, ty);
        expect_kind(p, TK_SEMICOLON, "';'");
        Stmt s; s.kind = ST_BLOCK; s.loc = kw->loc;
        stmt_array_init(&s.u.block);
        return s;
    }
    char *decl_name = NULL;
    Type ty = parse_type(p, &decl_name);
    const Token *name = peek(p);
    if (!decl_name) {
        /* No name from the declarator — expect one here. */
        if (name->kind != TK_IDENT) {
            die_at(name->loc.file, name->loc.line, name->loc.col,
                   "expected typedef name but got '%s'", name->text);
        }
        decl_name = xstrdup(name->text);
        advance(p);
    } else {
        /* Name came from the declarator; the next token should be ';'. */
        if (name->kind == TK_IDENT) advance(p);  /* consume if somehow present */
    }
    /* Reject a collision with an existing typedef. */
    if (typedef_registry_find(&p->tu->typedefs, decl_name)) {
        die_at(name->loc.file, name->loc.line, name->loc.col,
               "redefinition of typedef '%s'", decl_name);
    }
    typedef_registry_add(&p->tu->typedefs, decl_name, ty);
    /* registry took ownership of `ty` (and its heap sub-types) — do NOT
     * type_free it. */
    expect_kind(p, TK_SEMICOLON, "';'");
    Stmt s;
    s.kind = ST_BLOCK;
    s.loc = kw->loc;
    stmt_array_init(&s.u.block);
    return s;
}

/* Evaluate a compile-time integer constant for a case label: an int literal
 * or a previously defined enum constant.  Returns the value; dies on error.
 * `name` is the case label name for error messages. */
static int case_constant_value(Parser *p, const char *text) {
    if (text[0] >= '0' && text[0] <= '9') {
        return int_literal_value(text);
    }
    const EnumConstant *ec =
        enum_registry_find_constant(&p->tu->enums, text);
    if (ec) return ec->value;
    {
        const Token *t = peek(p);
        die_at(t->loc.file, t->loc.line, t->loc.col,
               "case label '%s' is not a constant", text);
    }
    return 0; /* unreachable */
}

/* Parse a switch statement:
 *   switch (expr) { case CONST: stmts ... default: stmts }
 * Statements belong to the most recent case/default label (fall-through). */
static Stmt parse_switch(Parser *p) {
    const Token *kw = peek(p);
    advance(p);  /* consume "switch" */
    expect_kind(p, TK_LPAREN, "'('");
    Expr *cond = parse_expr(p);
    expect_kind(p, TK_RPAREN, "')'");
    expect_kind(p, TK_LBRACE, "'{'");

    Stmt s;
    s.kind = ST_SWITCH;
    s.loc = kw->loc;
    s.u.switch_s.cond = cond;
    s.u.switch_s.cases = NULL;
    s.u.switch_s.num_cases = 0;
    s.u.switch_s.cap_cases = 0;

    while (peek(p)->kind != TK_RBRACE) {
        TokenKind k = peek(p)->kind;
        if (k == TK_KW_CASE) {
            advance(p);  /* consume "case" */
            const Token *cv = peek(p);
            int value;
            if (cv->kind == TK_INT_LITERAL) {
                value = int_literal_value(cv->text);
                advance(p);
            } else if (cv->kind == TK_IDENT) {
                value = case_constant_value(p, cv->text);
                advance(p);
            } else if (cv->kind == TK_CHAR_LITERAL) {
                value = char_literal_value(cv->text);
                advance(p);
            } else {
                die_at(cv->loc.file, cv->loc.line, cv->loc.col,
                       "expected constant case label but got '%s'", cv->text);
            }
            expect_kind(p, TK_COLON, "':'");
            switch_push_case(&s, 0, value);
        } else if (k == TK_KW_DEFAULT) {
            advance(p);  /* consume "default" */
            expect_kind(p, TK_COLON, "':'");
            switch_push_case(&s, 1, 0);
        } else {
            /* A statement — attach to the most recent case arm. */
            if (s.u.switch_s.num_cases == 0) {
                die_at(peek(p)->loc.file, peek(p)->loc.line, peek(p)->loc.col,
                       "statement before any case label in switch");
            }
            Stmt stmt = parse_stmt(p);
            SwitchCase *arm = &s.u.switch_s.cases[s.u.switch_s.num_cases - 1];
            stmt_array_push(&arm->stmts, stmt);
        }
    }
    expect_kind(p, TK_RBRACE, "'}'");
    return s;
}

static FunctionDecl parse_function_decl(Parser *p) {
    SourceLoc fn_loc = peek(p)->loc;
    /* Consume an optional leading storage class.  `extern` means a
     * declaration with no body; `static` gives the function LOCAL linkage;
     * `inline` is a no-op hint accepted so it doesn't choke
     * parse_type_abstract. */
    int is_extern = 0;
    int is_static = 0;
    for (;;) {
        if (peek(p)->kind == TK_KW_STATIC) { advance(p); is_static = 1; }
        else if (peek(p)->kind == TK_KW_EXTERN) { advance(p); is_extern = 1; }
        else if (peek(p)->kind == TK_KW_INLINE) { advance(p); }
        else break;
    }
    Type ret_ty = parse_type_abstract(p);

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
    fn.is_variadic = 0;
    fn.is_extern = is_extern;
    fn.is_static = is_static;

    /* Parameter list: type declarator ("," type declarator)* — or empty.
     * The special form `(void)` means "no parameters" (standard C). */
    if (peek(p)->kind == TK_KW_VOID
        && p->tokens->data[p->pos + 1].kind == TK_RPAREN) {
        advance(p);  /* consume `void` */
    } else if (peek(p)->kind != TK_RPAREN) {
        for (;;) {
            if (!is_type_start(p, p->pos)) {
                const Token *t = peek(p);
                die_at(t->loc.file, t->loc.line, t->loc.col,
                       "expected type for parameter but got '%s'", t->text);
            }
            char *pname = NULL;
            Type pty = parse_type(p, &pname);
            if (!pname) {
                /* Unnamed parameter — the declarator produced no name
                 * (e.g. `int`, `int*`).  Synthesize a name. */
                pname = xstrdup("");
            }
            /* Normalize array parameters (`int a[]`, `int a[N]`) to pointers,
             * matching standard C parameter adjustment. */
            if (pty.kind == TY_ARRAY) {
                Type elem = *pty.elem_type;
                type_free(&pty);
                pty = type_make_ptr(elem);
                type_free(&elem);
            }
            param_array_push(&fn.params, pname, pty, peek(p)->loc);
            free(pname);
            if (peek(p)->kind == TK_COMMA) {
                advance(p);
                /* Variadic tail: `...` must be the last thing before ')'.
                 * Consume it and end the parameter list. */
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
    }

    if (fn.is_variadic && fn.params.len == 0) {
        die_at(fn.loc.file, fn.loc.line, fn.loc.col,
               "'...' must follow a named parameter");
    }

    expect_kind(p, TK_RPAREN, "')'");

    /* Skip optional GCC `__attribute__((...))` annotations (e.g.
     * `__attribute__((noreturn))`) that may follow the parameter list. */
    for (;;) {
        if (!skip_attribute(p)) break;
    }

    if (fn.is_extern || peek(p)->kind == TK_SEMICOLON) {
        /* Declaration only / forward declaration — no body. */
        expect_kind(p, TK_SEMICOLON, "';'");
        fn.is_extern = 1;
        return fn;
    }

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
    stmt_array_init(&p.prepend);
    p.anon_counter = 0;

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
            /* A definition requires an IDENT tag followed by `{`.  Anonymous
             * structs (`struct { ... }` with no tag) fall through to the
             * declaration path, where parse_specifiers defines them inline. */
            if (tag->kind == TK_IDENT && peek(&p)->kind == TK_LBRACE) {
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
                /* Not a definition — reset and fall through to declaration. */
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
            /* A definition requires an IDENT tag followed by `{`.  Anonymous
             * unions fall through to the declaration path. */
            if (tag->kind == TK_IDENT && peek(&p)->kind == TK_LBRACE) {
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
                /* Not a definition — reset and fall through to declaration. */
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
            /* A definition requires an IDENT tag followed by `{`.  Anonymous
             * enums fall through to the declaration path. */
            if (tag->kind == TK_IDENT && peek(&p)->kind == TK_LBRACE) {
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
                /* Not a definition — reset and fall through to declaration. */
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

        /* Look ahead to distinguish function declaration/definition from global variable declaration.
         * A function has a parameter list '(' after the declarator name. */
        size_t save = p.pos;

        /* Skip specifiers and declarator to test if function parameter list LPAREN follows */
        /* 1. Skip storage-class / qualifiers / type specifiers */
        for (;;) {
            TokenKind tk = peek(&p)->kind;
            if (tk == TK_KW_STATIC || tk == TK_KW_EXTERN || tk == TK_KW_INLINE ||
                tk == TK_KW_CONST || tk == TK_KW_VOLATILE || tk == TK_KW_RESTRICT ||
                tk == TK_KW_SIGNED || tk == TK_KW_UNSIGNED ||
                tk == TK_KW_VOID || tk == TK_KW_INT || tk == TK_KW_CHAR ||
                tk == TK_KW_SHORT || tk == TK_KW_LONG || tk == TK_KW_FLOAT ||
                tk == TK_KW_DOUBLE || tk == TK_KW_BOOL) {
                advance(&p);
            } else if (tk == TK_KW_STRUCT || tk == TK_KW_UNION || tk == TK_KW_ENUM) {
                advance(&p);
                if (peek(&p)->kind == TK_IDENT) advance(&p);
                if (peek(&p)->kind == TK_LBRACE) {
                    int depth = 0;
                    do {
                        if (peek(&p)->kind == TK_LBRACE) depth++;
                        else if (peek(&p)->kind == TK_RBRACE) depth--;
                        advance(&p);
                    } while (depth > 0 && peek(&p)->kind != TK_EOF);
                }
            } else if (tk == TK_IDENT && typedef_registry_find(&tu->typedefs, peek(&p)->text)) {
                advance(&p);
            } else {
                break;
            }
        }
        /* 2. Skip declarator (stars, function ptr parens, identifier) */
        while (peek(&p)->kind == TK_STAR || peek(&p)->kind == TK_KW_CONST ||
               peek(&p)->kind == TK_KW_VOLATILE || peek(&p)->kind == TK_KW_RESTRICT) advance(&p);
        for (;;) { if (!skip_attribute(&p)) break; }
        int saw_name = 0;
        if (peek(&p)->kind == TK_LPAREN && p.pos + 1 < p.tokens->len &&
            (p.tokens->data[p.pos + 1].kind == TK_STAR || p.tokens->data[p.pos + 1].kind == TK_KW_CONST)) {
            advance(&p);
            while (peek(&p)->kind == TK_STAR || peek(&p)->kind == TK_KW_CONST) advance(&p);
            if (peek(&p)->kind == TK_IDENT) { advance(&p); saw_name = 1; }
            if (peek(&p)->kind == TK_RPAREN) advance(&p);
        } else {
            if (peek(&p)->kind == TK_IDENT) { advance(&p); saw_name = 1; }
        }
        for (;;) { if (!skip_attribute(&p)) break; }
        while (peek(&p)->kind == TK_LBRACKET) {
            advance(&p);
            while (peek(&p)->kind != TK_RBRACKET && peek(&p)->kind != TK_EOF) advance(&p);
            if (peek(&p)->kind == TK_RBRACKET) advance(&p);
        }
        int is_func = (saw_name && peek(&p)->kind == TK_LPAREN);
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
            if (s.kind == ST_BLOCK) {
                stmt_free(&s);
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
}
