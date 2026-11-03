/* ldscript.c — subset GNU ld script parser (fakeos S1).
 *
 * Accepts -T / --script= for boot/linker.ld and user/user.ld shapes.
 * Does not implement a general layout engine: parse + classify, then
 * configure link_kernel_multiboot / link_set_image_base.
 */

#include "fakecc/ldscript.h"
#include "fakecc/emit.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    TK_EOF = 0,
    TK_IDENT,
    TK_NUMBER,
    TK_LBRACE,   /* { */
    TK_RBRACE,   /* } */
    TK_LPAREN,   /* ( */
    TK_RPAREN,   /* ) */
    TK_SEMI,     /* ; */
    TK_COLON,    /* : */
    TK_COMMA,    /* , */
    TK_EQ,       /* = */
    TK_PLUS,     /* + */
    TK_MINUS,    /* - */
    TK_DOT,      /* .  (location counter or .section) */
    TK_STAR,     /* * */
    TK_PLUSEQ,   /* += */
    TK_OTHER
};

typedef struct {
    const char *src;
    const char *name;
    size_t pos;
    int tok;
    char text[128];
    uint64_t num;
    int line;
} Parser;

static void p_error(Parser *p, const char *msg) {
    fprintf(stderr, "fakecc: linker script %s:%d: %s\n",
            p->name ? p->name : "<script>", p->line, msg);
}

static void skip_ws_comments(Parser *p) {
    const char *s = p->src;
    size_t i = p->pos;
    for (;;) {
        while (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n') {
            if (s[i] == '\n') p->line++;
            i++;
        }
        if (s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (s[i] && !(s[i] == '*' && s[i + 1] == '/')) {
                if (s[i] == '\n') p->line++;
                i++;
            }
            if (s[i] == '*' && s[i + 1] == '/') i += 2;
            continue;
        }
        if (s[i] == '/' && s[i + 1] == '/') {
            while (s[i] && s[i] != '\n') i++;
            continue;
        }
        break;
    }
    p->pos = i;
}

static int is_ident_start(int c) {
    return isalpha(c) || c == '_' || c == '.';
}
static int is_ident_cont(int c) {
    return isalnum(c) || c == '_' || c == '.' || c == '*';
}

static void next_tok(Parser *p) {
    skip_ws_comments(p);
    const char *s = p->src;
    size_t i = p->pos;
    p->text[0] = '\0';
    p->num = 0;
    if (!s[i]) {
        p->tok = TK_EOF;
        return;
    }
    char c = s[i];
    if (c == '{') { p->tok = TK_LBRACE; p->pos = i + 1; return; }
    if (c == '}') { p->tok = TK_RBRACE; p->pos = i + 1; return; }
    if (c == '(') { p->tok = TK_LPAREN; p->pos = i + 1; return; }
    if (c == ')') { p->tok = TK_RPAREN; p->pos = i + 1; return; }
    if (c == ';') { p->tok = TK_SEMI; p->pos = i + 1; return; }
    if (c == ':') { p->tok = TK_COLON; p->pos = i + 1; return; }
    if (c == ',') { p->tok = TK_COMMA; p->pos = i + 1; return; }
    if (c == '*') { p->tok = TK_STAR; p->pos = i + 1; return; }
    if (c == '+') {
        if (s[i + 1] == '=') { p->tok = TK_PLUSEQ; p->pos = i + 2; return; }
        p->tok = TK_PLUS; p->pos = i + 1; return;
    }
    if (c == '-') { p->tok = TK_MINUS; p->pos = i + 1; return; }
    if (c == '=') { p->tok = TK_EQ; p->pos = i + 1; return; }
    if (c == '.' && !is_ident_cont((unsigned char)s[i + 1])) {
        p->tok = TK_DOT;
        p->pos = i + 1;
        strcpy(p->text, ".");
        return;
    }
    if (isdigit((unsigned char)c)) {
        char *end = NULL;
        unsigned long long v = strtoull(s + i, &end, 0);
        p->num = (uint64_t)v;
        size_t n = (size_t)(end - (s + i));
        if (n >= sizeof p->text) n = sizeof p->text - 1;
        memcpy(p->text, s + i, n);
        p->text[n] = '\0';
        /* optional K/M suffix like 4K */
        if (end && (*end == 'K' || *end == 'k')) {
            p->num *= 1024ULL;
            end++;
        } else if (end && (*end == 'M' || *end == 'm')) {
            p->num *= 1024ULL * 1024ULL;
            end++;
        }
        p->pos = (size_t)(end - s);
        p->tok = TK_NUMBER;
        return;
    }
    if (is_ident_start((unsigned char)c)) {
        size_t n = 0;
        while (is_ident_cont((unsigned char)s[i + n])) {
            if (n + 1 < sizeof p->text) p->text[n] = s[i + n];
            n++;
        }
        if (n >= sizeof p->text) n = sizeof p->text - 1;
        p->text[n] = '\0';
        p->pos = i + n;
        /* Lone "." already handled; ".text" etc. are idents starting with '.' */
        if (strcmp(p->text, ".") == 0) {
            p->tok = TK_DOT;
        } else {
            p->tok = TK_IDENT;
        }
        return;
    }
    p->tok = TK_OTHER;
    p->text[0] = c;
    p->text[1] = '\0';
    p->pos = i + 1;
}

static int accept(Parser *p, int t) {
    if (p->tok == t) { next_tok(p); return 1; }
    return 0;
}

static int expect(Parser *p, int t, const char *what) {
    if (accept(p, t)) return 1;
    char buf[128];
    snprintf(buf, sizeof buf, "expected %s", what);
    p_error(p, buf);
    return 0;
}

/* Forward */
static int parse_expr(Parser *p, LdScriptInfo *info, uint64_t *val);

/* Symbol table for absolute assigns during parse (small). */
typedef struct {
    char name[64];
    uint64_t val;
} Sym;

#define MAX_SYMS 64
typedef struct {
    Sym syms[MAX_SYMS];
    int n;
} SymTab;

static void sym_set(SymTab *st, const char *name, uint64_t v) {
    for (int i = 0; i < st->n; i++) {
        if (strcmp(st->syms[i].name, name) == 0) {
            st->syms[i].val = v;
            return;
        }
    }
    if (st->n >= MAX_SYMS) return;
    {
        size_t lim = sizeof st->syms[0].name - 1;
        size_t n = 0;
        while (name[n] && n < lim) {
            st->syms[st->n].name[n] = name[n];
            n++;
        }
        st->syms[st->n].name[n] = '\0';
    }
    st->syms[st->n].val = v;
    st->n++;
}

static int sym_get(SymTab *st, const char *name, uint64_t *out) {
    for (int i = 0; i < st->n; i++) {
        if (strcmp(st->syms[i].name, name) == 0) {
            *out = st->syms[i].val;
            return 1;
        }
    }
    return 0;
}

static SymTab *g_syms; /* active during one parse */

static int parse_primary(Parser *p, LdScriptInfo *info, uint64_t *val) {
    if (p->tok == TK_NUMBER) {
        *val = p->num;
        next_tok(p);
        return 1;
    }
    if (p->tok == TK_DOT) {
        /* location counter — unknown at configure time; treat as 0 */
        *val = 0;
        next_tok(p);
        return 1;
    }
    if (p->tok == TK_IDENT) {
        char name[128];
        {
            size_t lim = sizeof name - 1, n = 0;
            while (p->text[n] && n < lim) { name[n] = p->text[n]; n++; }
            name[n] = '\0';
        }
        next_tok(p);
        if (strcmp(name, "ALIGN") == 0 && accept(p, TK_LPAREN)) {
            uint64_t a = 0;
            if (!parse_expr(p, info, &a)) return 0;
            if (!expect(p, TK_RPAREN, ")")) return 0;
            *val = a; /* ALIGN(n) as value = n for attr purposes */
            return 1;
        }
        if (strcmp(name, "ADDR") == 0 && accept(p, TK_LPAREN)) {
            /* ADDR(sec) — skip section name, value unknown → 0 */
            if (p->tok == TK_IDENT || p->tok == TK_DOT) next_tok(p);
            if (!expect(p, TK_RPAREN, ")")) return 0;
            *val = 0;
            return 1;
        }
        if (strcmp(name, "FLAGS") == 0 && accept(p, TK_LPAREN)) {
            uint64_t f = 0;
            if (!parse_expr(p, info, &f)) return 0;
            if (!expect(p, TK_RPAREN, ")")) return 0;
            *val = f;
            return 1;
        }
        if (g_syms && sym_get(g_syms, name, val)) return 1;
        /* Unknown identifier: leave 0 (section names in patterns, etc.) */
        *val = 0;
        return 1;
    }
    if (accept(p, TK_LPAREN)) {
        if (!parse_expr(p, info, val)) return 0;
        return expect(p, TK_RPAREN, ")");
    }
    p_error(p, "expected expression");
    return 0;
}

static int parse_expr(Parser *p, LdScriptInfo *info, uint64_t *val) {
    if (!parse_primary(p, info, val)) return 0;
    while (p->tok == TK_PLUS || p->tok == TK_MINUS) {
        int op = p->tok;
        next_tok(p);
        uint64_t rhs = 0;
        if (!parse_primary(p, info, &rhs)) return 0;
        if (op == TK_PLUS) *val += rhs;
        else *val -= rhs;
        /* Detect AT(ADDR(.text) - KERNEL_VMA) style: ADDR then minus KERNEL_VMA */
        if (op == TK_MINUS && info->has_kernel_vma && rhs == info->kernel_vma)
            info->saw_at_addr_minus_kernel_vma = 1;
    }
    return 1;
}

static int parse_output_format(Parser *p, LdScriptInfo *info) {
    if (!expect(p, TK_LPAREN, "(")) return 0;
    /* format name may be ident with hyphens: elf64-x86-64 — our lexer
     * treats '-' as TK_MINUS, so gather tokens until ')'. */
    size_t n = 0;
    info->output_format[0] = '\0';
    while (p->tok != TK_RPAREN && p->tok != TK_EOF) {
        const char *piece = NULL;
        if (p->tok == TK_IDENT || p->tok == TK_NUMBER) {
            piece = p->text;
        } else if (p->tok == TK_MINUS) {
            piece = "-";
        } else if (p->tok == TK_DOT) {
            piece = ".";
        } else {
            piece = p->text[0] ? p->text : "?";
        }
        size_t pl = strlen(piece);
        if (n + pl < sizeof info->output_format) {
            memcpy(info->output_format + n, piece, pl);
            n += pl;
            info->output_format[n] = '\0';
        }
        next_tok(p);
    }
    return expect(p, TK_RPAREN, ")");
}

static int parse_entry(Parser *p, LdScriptInfo *info) {
    if (!expect(p, TK_LPAREN, "(")) return 0;
    if (p->tok != TK_IDENT) {
        p_error(p, "ENTRY expects symbol");
        return 0;
    }
    {
        size_t lim = sizeof info->entry - 1, n = 0;
        while (p->text[n] && n < lim) { info->entry[n] = p->text[n]; n++; }
        info->entry[n] = '\0';
    }
    next_tok(p);
    return expect(p, TK_RPAREN, ")");
}

static int parse_phdrs(Parser *p, LdScriptInfo *info) {
    if (!expect(p, TK_LBRACE, "{")) return 0;
    info->has_phdrs = 1;
    while (p->tok != TK_RBRACE && p->tok != TK_EOF) {
        if (p->tok != TK_IDENT) {
            p_error(p, "PHDRS: expected name");
            return 0;
        }
        char pname[64];
        {
            size_t lim = sizeof pname - 1, n = 0;
            while (p->text[n] && n < lim) { pname[n] = p->text[n]; n++; }
            pname[n] = '\0';
        }
        next_tok(p);
        /* type: PT_LOAD or ident */
        if (p->tok != TK_IDENT) {
            p_error(p, "PHDRS: expected type");
            return 0;
        }
        next_tok(p); /* skip PT_LOAD */
        int flags = -1;
        if (p->tok == TK_IDENT && strcmp(p->text, "FLAGS") == 0) {
            next_tok(p);
            if (!expect(p, TK_LPAREN, "(")) return 0;
            uint64_t f = 0;
            if (!parse_expr(p, info, &f)) return 0;
            if (!expect(p, TK_RPAREN, ")")) return 0;
            flags = (int)f;
        }
        if (!expect(p, TK_SEMI, ";")) return 0;
        if (strcmp(pname, "text") == 0) info->text_phdr_flags = flags;
        else if (strcmp(pname, "data") == 0) info->data_phdr_flags = flags;
    }
    return expect(p, TK_RBRACE, "}");
}

/* Skip / record one output-section or location-counter statement inside SECTIONS. */
static int parse_sections_stmt(Parser *p, LdScriptInfo *info) {
    /* `. = expr;` or `. += expr;` */
    if (p->tok == TK_DOT) {
        next_tok(p);
        if (accept(p, TK_EQ)) {
            uint64_t v = 0;
            if (!parse_expr(p, info, &v)) return 0;
            return expect(p, TK_SEMI, ";");
        }
        if (accept(p, TK_PLUSEQ)) {
            /* Remember if RHS references KERNEL_VMA */
            size_t save = p->pos;
            int save_tok = p->tok;
            char save_text[128];
            uint64_t save_num = p->num;
            {
                size_t n = 0, lim = sizeof save_text - 1;
                while (p->text[n] && n < lim) { save_text[n] = p->text[n]; n++; }
                save_text[n] = '\0';
            }
            if (p->tok == TK_IDENT && strcmp(p->text, "KERNEL_VMA") == 0)
                info->saw_dot_plus_kernel_vma = 1;
            uint64_t v = 0;
            if (!parse_expr(p, info, &v)) return 0;
            (void)save; (void)save_tok; (void)save_num;
            if (info->has_kernel_vma && v == info->kernel_vma)
                info->saw_dot_plus_kernel_vma = 1;
            return expect(p, TK_SEMI, ";");
        }
        p_error(p, "bad location-counter statement");
        return 0;
    }

    /* `sym = .;` or `sym = expr;` at SECTIONS scope (boundary symbols) */
    if (p->tok == TK_IDENT) {
        char name[128];
        {
            size_t lim = sizeof name - 1, n = 0;
            while (p->text[n] && n < lim) { name[n] = p->text[n]; n++; }
            name[n] = '\0';
        }
        /* Could be output section name (.text) — those start with '.' and
         * are TK_IDENT in our lexer ("\.text"). Or bare assign. */
        next_tok(p);
        if (accept(p, TK_EQ)) {
            uint64_t v = 0;
            if (!parse_expr(p, info, &v)) return 0;
            if (g_syms) sym_set(g_syms, name, v);
            return expect(p, TK_SEMI, ";");
        }
        /* Output section: name [ALIGN(n)] : [AT(expr)] { ... } [:phdr] */
        if (strcmp(name, ".boot") == 0) info->saw_boot_section = 1;

        /* Optional ALIGN(n) before ':' */
        if (p->tok == TK_IDENT && strcmp(p->text, "ALIGN") == 0) {
            next_tok(p);
            if (!expect(p, TK_LPAREN, "(")) return 0;
            uint64_t a = 0;
            if (!parse_expr(p, info, &a)) return 0;
            if (!expect(p, TK_RPAREN, ")")) return 0;
            (void)a;
        }
        if (!expect(p, TK_COLON, ":")) return 0;

        /* Optional AT(expr) */
        if (p->tok == TK_IDENT && strcmp(p->text, "AT") == 0) {
            next_tok(p);
            if (!expect(p, TK_LPAREN, "(")) return 0;
            uint64_t atv = 0;
            /* Look ahead for ADDR(...) - KERNEL_VMA pattern via parse_expr */
            if (!parse_expr(p, info, &atv)) return 0;
            if (!expect(p, TK_RPAREN, ")")) return 0;
            (void)atv;
        }

        if (!expect(p, TK_LBRACE, "{")) return 0;
        while (p->tok != TK_RBRACE && p->tok != TK_EOF) {
            /* KEEP(*(pat)) */
            if (p->tok == TK_IDENT && strcmp(p->text, "KEEP") == 0) {
                next_tok(p);
                if (!expect(p, TK_LPAREN, "(")) return 0;
                if (!expect(p, TK_STAR, "*")) return 0;
                if (!expect(p, TK_LPAREN, "(")) return 0;
                /* patterns: .multiboot or list */
                while (p->tok != TK_RPAREN && p->tok != TK_EOF) {
                    if (p->tok == TK_IDENT) {
                        if (strcmp(p->text, ".multiboot") == 0)
                            info->saw_keep_multiboot = 1;
                        next_tok(p);
                    } else if (p->tok == TK_COMMA) {
                        next_tok(p);
                    } else {
                        next_tok(p);
                    }
                }
                if (!expect(p, TK_RPAREN, ")")) return 0;
                if (!expect(p, TK_RPAREN, ")")) return 0;
                continue;
            }
            /* *(pat list) or *(COMMON) */
            if (accept(p, TK_STAR)) {
                if (!expect(p, TK_LPAREN, "(")) return 0;
                while (p->tok != TK_RPAREN && p->tok != TK_EOF) {
                    if (p->tok == TK_IDENT || p->tok == TK_NUMBER ||
                        p->tok == TK_DOT || p->tok == TK_COMMA)
                        next_tok(p);
                    else
                        next_tok(p);
                }
                if (!expect(p, TK_RPAREN, ")")) return 0;
                continue;
            }
            /* sym = .; */
            if (p->tok == TK_IDENT) {
                char sn[128];
        {
            size_t lim = sizeof sn - 1, n = 0;
            while (p->text[n] && n < lim) { sn[n] = p->text[n]; n++; }
            sn[n] = '\0';
        }
                next_tok(p);
                if (!expect(p, TK_EQ, "=")) return 0;
                uint64_t v = 0;
                if (!parse_expr(p, info, &v)) return 0;
                if (!expect(p, TK_SEMI, ";")) return 0;
                if (g_syms) sym_set(g_syms, sn, v);
                continue;
            }
            /* `. += expr;` inside section (rare) */
            if (p->tok == TK_DOT) {
                next_tok(p);
                if (accept(p, TK_PLUSEQ)) {
                    uint64_t v = 0;
                    if (!parse_expr(p, info, &v)) return 0;
                    if (!expect(p, TK_SEMI, ";")) return 0;
                    continue;
                }
                if (accept(p, TK_EQ)) {
                    uint64_t v = 0;
                    if (!parse_expr(p, info, &v)) return 0;
                    if (!expect(p, TK_SEMI, ";")) return 0;
                    continue;
                }
            }
            p_error(p, "unsupported statement inside output section");
            return 0;
        }
        if (!expect(p, TK_RBRACE, "}")) return 0;
        /* Optional :phdr */
        if (accept(p, TK_COLON)) {
            if (p->tok == TK_IDENT) next_tok(p);
            else {
                p_error(p, "expected phdr name after ':'");
                return 0;
            }
        }
        return 1;
    }

    p_error(p, "unsupported SECTIONS statement");
    return 0;
}

static int parse_sections(Parser *p, LdScriptInfo *info) {
    if (!expect(p, TK_LBRACE, "{")) return 0;
    while (p->tok != TK_RBRACE && p->tok != TK_EOF) {
        if (!parse_sections_stmt(p, info)) return 0;
    }
    return expect(p, TK_RBRACE, "}");
}

static void classify(LdScriptInfo *info) {
    if (info->has_kernel_vma || info->saw_dot_plus_kernel_vma ||
        info->saw_at_addr_minus_kernel_vma || info->saw_boot_section ||
        info->saw_keep_multiboot) {
        info->kind = LDSCRIPT_KIND_KERNEL_HH;
        return;
    }
    if (info->has_user_vma || info->has_phdrs) {
        info->kind = LDSCRIPT_KIND_USER;
        return;
    }
    info->kind = LDSCRIPT_KIND_UNKNOWN;
}

int ldscript_parse_string(const char *src, const char *name, LdScriptInfo *out) {
    if (!src || !out) return -1;
    memset(out, 0, sizeof *out);
    out->text_phdr_flags = -1;
    out->data_phdr_flags = -1;

    SymTab st;
    memset(&st, 0, sizeof st);
    g_syms = &st;

    Parser p;
    memset(&p, 0, sizeof p);
    p.src = src;
    p.name = name ? name : "<script>";
    p.line = 1;
    next_tok(&p);

    while (p.tok != TK_EOF) {
        if (p.tok != TK_IDENT) {
            p_error(&p, "expected top-level command");
            g_syms = NULL;
            return -1;
        }
        char cmd[64];
        {
            size_t lim = sizeof cmd - 1, n = 0;
            while (p.text[n] && n < lim) { cmd[n] = p.text[n]; n++; }
            cmd[n] = '\0';
        }
        next_tok(&p);

        if (strcmp(cmd, "OUTPUT_FORMAT") == 0) {
            if (!parse_output_format(&p, out)) { g_syms = NULL; return -1; }
        } else if (strcmp(cmd, "ENTRY") == 0) {
            if (!parse_entry(&p, out)) { g_syms = NULL; return -1; }
        } else if (strcmp(cmd, "PHDRS") == 0) {
            if (!parse_phdrs(&p, out)) { g_syms = NULL; return -1; }
        } else if (strcmp(cmd, "SECTIONS") == 0) {
            if (!parse_sections(&p, out)) { g_syms = NULL; return -1; }
        } else if (accept(&p, TK_EQ)) {
            /* NAME = expr; */
            uint64_t v = 0;
            if (!parse_expr(&p, out, &v)) { g_syms = NULL; return -1; }
            if (!expect(&p, TK_SEMI, ";")) { g_syms = NULL; return -1; }
            sym_set(&st, cmd, v);
            if (strcmp(cmd, "KERNEL_VMA") == 0) {
                out->kernel_vma = v;
                out->has_kernel_vma = 1;
            } else if (strcmp(cmd, "KERNEL_LMA") == 0) {
                out->kernel_lma = v;
                out->has_kernel_lma = 1;
            } else if (strcmp(cmd, "USER_VMA") == 0) {
                out->user_vma = v;
                out->has_user_vma = 1;
            }
        } else {
            char buf[160];
            snprintf(buf, sizeof buf, "unsupported command '%s'", cmd);
            p_error(&p, buf);
            g_syms = NULL;
            return -1;
        }
    }

    classify(out);
    g_syms = NULL;
    return 0;
}

int ldscript_parse_file(const char *path, LdScriptInfo *out) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "fakecc: cannot open linker script %s\n", path);
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long n = ftell(f);
    if (n < 0) { fclose(f); return -1; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return -1; }
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return -1; }
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[got] = '\0';
    int rc = ldscript_parse_string(buf, path, out);
    free(buf);
    return rc;
}

int ldscript_configure_link(const LdScriptInfo *info, int *out_kernel_multiboot) {
    if (!info || !out_kernel_multiboot) return -1;
    *out_kernel_multiboot = 0;

    if (info->kind == LDSCRIPT_KIND_KERNEL_HH) {
        /* Existing link_kernel_multiboot hardcodes these constants; reject
         * scripts that ask for a different higher-half layout. */
        const uint64_t expect_vma = 0xFFFFFFFF80000000ULL;
        const uint64_t expect_lma = 0x00100000ULL;
        if (info->has_kernel_vma && info->kernel_vma != expect_vma) {
            fprintf(stderr,
                    "fakecc: -T kernel script KERNEL_VMA=0x%llx unsupported "
                    "(only 0xFFFFFFFF80000000)\n",
                    (unsigned long long)info->kernel_vma);
            return -1;
        }
        if (info->has_kernel_lma && info->kernel_lma != expect_lma) {
            fprintf(stderr,
                    "fakecc: -T kernel script KERNEL_LMA=0x%llx unsupported "
                    "(only 0x100000)\n",
                    (unsigned long long)info->kernel_lma);
            return -1;
        }
        if (info->entry[0] && strcmp(info->entry, "_start") != 0) {
            fprintf(stderr,
                    "fakecc: -T kernel script ENTRY(%s) unsupported "
                    "(only _start)\n", info->entry);
            return -1;
        }
        *out_kernel_multiboot = 1;
        return 0;
    }

    if (info->kind == LDSCRIPT_KIND_USER) {
        uint64_t base = info->has_user_vma ? info->user_vma : 0x400000ULL;
        if (base == 0) {
            fprintf(stderr, "fakecc: -T user script USER_VMA must be non-zero\n");
            return -1;
        }
        /* Optional PHDR flag check: expect RX=5, RW=6 when present. */
        if (info->text_phdr_flags >= 0 && info->text_phdr_flags != 5) {
            fprintf(stderr,
                    "fakecc: -T user script text FLAGS(%d) unsupported (need 5)\n",
                    info->text_phdr_flags);
            return -1;
        }
        if (info->data_phdr_flags >= 0 && info->data_phdr_flags != 6) {
            fprintf(stderr,
                    "fakecc: -T user script data FLAGS(%d) unsupported (need 6)\n",
                    info->data_phdr_flags);
            return -1;
        }
        link_set_image_base(base);
        return 0;
    }

    fprintf(stderr,
            "fakecc: linker script not recognized as fakeos kernel "
            "(boot/linker.ld) or user (user/user.ld) shape\n");
    return -1;
}
