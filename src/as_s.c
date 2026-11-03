/* Gas-compatible .S assembler front-end (fakeos S3).
 *
 * Handles a small directive/macro subset, then hands AT&T instructions to
 * iasm_assemble_body.  Enough for io / usys / crt0 class stubs and for the
 * privileged ops used by later desc/sysentry migration.
 */
#include "fakecc/iasm.h"
#include "fakecc/common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    char **formals;
    int nformals;
    char *body; /* raw lines joined by \n */
} Macro;

typedef struct {
    Macro *macros;
    int nmacros, cap;
    char **globals;
    int nglobals, gcap;
    char **externs;
    int nexterns, ecap;
    char *text; /* expanded instruction stream */
    size_t tlen, tcap;
} AsState;

static void as_fail(const char *file, int line, const char *msg) {
    fprintf(stderr, "%s:%d: %s\n", file ? file : "<asm>", line, msg);
}

static char *xdup(const char *s) {
    size_t n = strlen(s);
    char *o = malloc(n + 1);
    if (!o) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    memcpy(o, s, n + 1);
    return o;
}

static void append_text(AsState *st, const char *line) {
    size_t n = strlen(line);
    if (st->tlen + n + 2 > st->tcap) {
        size_t nc = st->tcap ? st->tcap * 2 : 256;
        while (nc < st->tlen + n + 2) nc *= 2;
        st->text = realloc(st->text, nc);
        if (!st->text) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        st->tcap = nc;
    }
    memcpy(st->text + st->tlen, line, n);
    st->tlen += n;
    st->text[st->tlen++] = '\n';
    st->text[st->tlen] = 0;
}

static void add_str(char ***arr, int *n, int *cap, const char *s) {
    for (int i = 0; i < *n; i++)
        if (strcmp((*arr)[i], s) == 0) return;
    if (*n >= *cap) {
        int nc = *cap ? *cap * 2 : 8;
        *arr = realloc(*arr, (size_t)nc * sizeof(char *));
        if (!*arr) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        *cap = nc;
    }
    (*arr)[(*n)++] = xdup(s);
}

static const char *skip_ws(const char *p) {
    while (*p && isspace((unsigned char)*p) && *p != '\n') p++;
    return p;
}

static int starts(const char *p, const char *kw) {
    size_t n = strlen(kw);
    return strncmp(p, kw, n) == 0 &&
           (!p[n] || isspace((unsigned char)p[n]) || p[n] == ':' ||
            p[n] == ',' || p[n] == ';' || p[n] == '#');
}

/* Expand \arg and \() in macro body. */
static char *macro_subst(const Macro *m, char **actuals, int nact) {
    const char *b = m->body ? m->body : "";
    size_t cap = strlen(b) + 64, len = 0;
    char *out = malloc(cap);
    if (!out) exit(1);
    out[0] = 0;
    for (const char *p = b; *p; ) {
        if (*p == '\\') {
            p++;
            if (*p == '(' && p[1] == ')') { p += 2; continue; } /* gas empty concat */
            /* formal name */
            const char *s = p;
            while (*p && (isalnum((unsigned char)*p) || *p == '_')) p++;
            size_t nl = (size_t)(p - s);
            int ai = -1;
            for (int i = 0; i < m->nformals; i++) {
                if (strlen(m->formals[i]) == nl &&
                    strncmp(m->formals[i], s, nl) == 0) {
                    ai = i; break;
                }
            }
            const char *rep = (ai >= 0 && ai < nact) ? actuals[ai] : "";
            size_t rn = strlen(rep);
            while (len + rn + 1 > cap) {
                cap *= 2; out = realloc(out, cap); if (!out) exit(1);
            }
            memcpy(out + len, rep, rn); len += rn; out[len] = 0;
            continue;
        }
        if (len + 2 > cap) { cap *= 2; out = realloc(out, cap); if (!out) exit(1); }
        out[len++] = *p++;
        out[len] = 0;
    }
    return out;
}

static Macro *find_macro(AsState *st, const char *name) {
    for (int i = 0; i < st->nmacros; i++)
        if (strcmp(st->macros[i].name, name) == 0) return &st->macros[i];
    return NULL;
}

static void free_state(AsState *st) {
    for (int i = 0; i < st->nmacros; i++) {
        free(st->macros[i].name);
        for (int j = 0; j < st->macros[i].nformals; j++)
            free(st->macros[i].formals[j]);
        free(st->macros[i].formals);
        free(st->macros[i].body);
    }
    free(st->macros);
    for (int i = 0; i < st->nglobals; i++) free(st->globals[i]);
    free(st->globals);
    for (int i = 0; i < st->nexterns; i++) free(st->externs[i]);
    free(st->externs);
    free(st->text);
}


static void ingest_line(AsState *st, const char *line) {
    const char *lp = skip_ws(line);
    if (!*lp) return;
    if (starts(lp, ".globl") || starts(lp, ".global")) {
        if (starts(lp, ".global")) lp = skip_ws(lp + 7);
        else lp = skip_ws(lp + 6);
        while (*lp) {
            lp = skip_ws(lp);
            if (*lp == ',') { lp++; continue; }
            const char *s = lp;
            while (*lp && !isspace((unsigned char)*lp) && *lp != ',') lp++;
            char id[128];
            size_t n = (size_t)(lp - s);
            if (!n) break;
            if (n >= sizeof id) n = sizeof id - 1;
            memcpy(id, s, n); id[n] = 0;
            add_str(&st->globals, &st->nglobals, &st->gcap, id);
        }
        return;
    }
    if (starts(lp, ".extern")) {
        lp = skip_ws(lp + 7);
        while (*lp) {
            lp = skip_ws(lp);
            if (*lp == ',') { lp++; continue; }
            const char *s = lp;
            while (*lp && !isspace((unsigned char)*lp) && *lp != ',') lp++;
            char id[128];
            size_t n = (size_t)(lp - s);
            if (!n) break;
            if (n >= sizeof id) n = sizeof id - 1;
            memcpy(id, s, n); id[n] = 0;
            add_str(&st->externs, &st->nexterns, &st->ecap, id);
        }
        return;
    }
    if (starts(lp, ".text") || starts(lp, ".data") || starts(lp, ".bss") ||
        starts(lp, ".section") || starts(lp, ".align") || starts(lp, ".p2align") ||
        starts(lp, ".type") || starts(lp, ".size") || starts(lp, ".file") ||
        starts(lp, ".ident") || starts(lp, ".code64") || starts(lp, ".intel_syntax") ||
        starts(lp, ".att_syntax") || starts(lp, ".set") || starts(lp, ".equ"))
        return;
    append_text(st, lp);
}

int assemble_s_file(const char *src, const char *filename, EmitModule *out) {

    if (!src || !out) return -1;
    AsState st;
    memset(&st, 0, sizeof st);

    /* Line-oriented parse with .macro / .rept nesting. */
    const char *p = src;
    int lineno = 1;
    Macro *cur_macro = NULL;
    char *macro_acc = NULL;
    size_t mlen = 0, mcap = 0;
    int rept_count = 0;
    char *rept_acc = NULL;
    size_t rlen = 0, rcap = 0;
    int in_rept = 0;

    while (*p) {
        const char *line_start = p;
        while (*p && *p != '\n') p++;
        size_t llen = (size_t)(p - line_start);
        char *line = malloc(llen + 1);
        if (!line) exit(1);
        memcpy(line, line_start, llen);
        line[llen] = 0;
        if (*p == '\n') p++;

        /* strip C block comments (no nest); also // and # and ; */
        char *cut = strstr(line, "/*");
        if (cut) *cut = 0;
        for (char *c = line; *c; c++) {
            if (*c == '#' || (*c == '/' && c[1] == '/') || *c == ';') {
                *c = 0; break;
            }
        }
        const char *lp = skip_ws(line);

        if (cur_macro) {
            if (starts(lp, ".endm") || starts(lp, ".endmacro")) {
                cur_macro->body = macro_acc ? macro_acc : xdup("");
                macro_acc = NULL; mlen = mcap = 0;
                cur_macro = NULL;
            } else {
                size_t n = strlen(line);
                if (mlen + n + 2 > mcap) {
                    mcap = mcap ? mcap * 2 : 128;
                    while (mcap < mlen + n + 2) mcap *= 2;
                    macro_acc = realloc(macro_acc, mcap);
                    if (!macro_acc) exit(1);
                }
                memcpy(macro_acc + mlen, line, n);
                mlen += n;
                macro_acc[mlen++] = '\n';
                macro_acc[mlen] = 0;
            }
            free(line); lineno++; continue;
        }

        if (in_rept) {
            if (starts(lp, ".endr")) {
                for (int i = 0; i < rept_count; i++) {
                    char *rp = rept_acc ? xdup(rept_acc) : xdup("");
                    char *ep = rp;
                    while (*ep) {
                        char *eol = strchr(ep, '\n');
                        if (eol) {
                            *eol = 0;
                            if (*ep) ingest_line(&st, ep);
                            ep = eol + 1;
                        } else {
                            if (*ep) ingest_line(&st, ep);
                            break;
                        }
                    }
                    free(rp);
                }
                free(rept_acc); rept_acc = NULL; rlen = rcap = 0;
                in_rept = 0; rept_count = 0;
            } else {
                size_t n = strlen(line);
                if (rlen + n + 2 > rcap) {
                    rcap = rcap ? rcap * 2 : 128;
                    while (rcap < rlen + n + 2) rcap *= 2;
                    rept_acc = realloc(rept_acc, rcap);
                    if (!rept_acc) exit(1);
                }
                memcpy(rept_acc + rlen, line, n);
                rlen += n; rept_acc[rlen++] = '\n'; rept_acc[rlen] = 0;
            }
            free(line); lineno++; continue;
        }

        if (!*lp) { free(line); lineno++; continue; }

        if (starts(lp, ".macro")) {
            lp = skip_ws(lp + 6);
            const char *ns = lp;
            while (*lp && !isspace((unsigned char)*lp) && *lp != ',') lp++;
            char name[128];
            size_t nl = (size_t)(lp - ns);
            if (nl >= sizeof name) nl = sizeof name - 1;
            memcpy(name, ns, nl); name[nl] = 0;
            if (st.nmacros >= st.cap) {
                st.cap = st.cap ? st.cap * 2 : 4;
                st.macros = realloc(st.macros, (size_t)st.cap * sizeof(Macro));
                if (!st.macros) exit(1);
            }
            Macro *m = &st.macros[st.nmacros++];
            memset(m, 0, sizeof *m);
            m->name = xdup(name);
            /* formals: comma/space separated */
            while (*lp) {
                lp = skip_ws(lp);
                if (*lp == ',') { lp++; continue; }
                if (!*lp) break;
                const char *fs = lp;
                while (*lp && !isspace((unsigned char)*lp) && *lp != ',') lp++;
                char formal[64];
                size_t fl = (size_t)(lp - fs);
                if (fl >= sizeof formal) fl = sizeof formal - 1;
                memcpy(formal, fs, fl); formal[fl] = 0;
                m->formals = realloc(m->formals, (size_t)(m->nformals + 1) * sizeof(char *));
                m->formals[m->nformals++] = xdup(formal);
            }
            cur_macro = m;
            free(line); lineno++; continue;
        }

        if (starts(lp, ".rept")) {
            lp = skip_ws(lp + 5);
            rept_count = (int)strtol(lp, NULL, 0);
            if (rept_count < 0) rept_count = 0;
            if (rept_count > 4096) {
                as_fail(filename, lineno, ".rept count too large");
                free(line); free_state(&st); return -1;
            }
            in_rept = 1;
            free(line); lineno++; continue;
        }

        if (starts(lp, ".globl") || starts(lp, ".global")) {
            if (starts(lp, ".global")) lp = skip_ws(lp + 7);
            else lp = skip_ws(lp + 6);
            while (*lp) {
                lp = skip_ws(lp);
                if (*lp == ',') { lp++; continue; }
                const char *s = lp;
                while (*lp && !isspace((unsigned char)*lp) && *lp != ',') lp++;
                char id[128];
                size_t n = (size_t)(lp - s);
                if (!n) break;
                if (n >= sizeof id) n = sizeof id - 1;
                memcpy(id, s, n); id[n] = 0;
                add_str(&st.globals, &st.nglobals, &st.gcap, id);
            }
            free(line); lineno++; continue;
        }

        if (starts(lp, ".extern") || starts(lp, ".extrn")) {
            lp = skip_ws(lp + 7);
            while (*lp) {
                lp = skip_ws(lp);
                if (*lp == ',') { lp++; continue; }
                const char *s = lp;
                while (*lp && !isspace((unsigned char)*lp) && *lp != ',') lp++;
                char id[128];
                size_t n = (size_t)(lp - s);
                if (!n) break;
                if (n >= sizeof id) n = sizeof id - 1;
                memcpy(id, s, n); id[n] = 0;
                add_str(&st.externs, &st.nexterns, &st.ecap, id);
            }
            free(line); lineno++; continue;
        }

        /* Ignored / section switches — section content goes to .text for now
         * (S3 stubs are code-only). .bss/.data bytes via .zero/.quad still ok. */
        if (starts(lp, ".text") || starts(lp, ".data") || starts(lp, ".bss") ||
            starts(lp, ".section") || starts(lp, ".align") || starts(lp, ".p2align") ||
            starts(lp, ".type") || starts(lp, ".size") || starts(lp, ".file") ||
            starts(lp, ".ident") || starts(lp, ".code64") || starts(lp, ".intel_syntax") ||
            starts(lp, ".att_syntax") || starts(lp, ".set") || starts(lp, ".equ")) {
            free(line); lineno++; continue;
        }

        /* Macro invocation: NAME args... at start of line (not a label) */
        {
            const char *q = lp;
            char mname[128];
            int mi = 0;
            while (*q && (isalnum((unsigned char)*q) || *q == '_') && mi + 1 < (int)sizeof mname)
                mname[mi++] = *q++;
            mname[mi] = 0;
            q = skip_ws(q);
            Macro *m = mname[0] ? find_macro(&st, mname) : NULL;
            if (m && *q != ':') {
                /* parse actuals */
                char *acts[16];
                int na = 0;
                while (*q && na < 16) {
                    if (*q == ',') { q++; q = skip_ws(q); continue; }
                    const char *s = q;
                    while (*q && *q != ',') q++;
                    while (q > s && isspace((unsigned char)q[-1])) q--;
                    size_t n = (size_t)(q - s);
                    char *a = malloc(n + 1);
                    if (!a) exit(1);
                    memcpy(a, s, n); a[n] = 0;
                    /* trim leading space */
                    char *t = a;
                    while (*t && isspace((unsigned char)*t)) t++;
                    acts[na++] = xdup(t);
                    free(a);
                    q = skip_ws(q);
                    if (*q == ',') { q++; q = skip_ws(q); }
                }
                char *exp = macro_subst(m, acts, na);
                for (int i = 0; i < na; i++) free(acts[i]);
                char *ep = exp;
                while (*ep) {
                    char *eol = strchr(ep, '\n');
                    if (eol) {
                        *eol = 0;
                        if (*ep) ingest_line(&st, ep);
                        ep = eol + 1;
                    } else {
                        if (*ep) ingest_line(&st, ep);
                        break;
                    }
                }
                free(exp);
                free(line); lineno++; continue;
            }
        }

        append_text(&st, lp);
        free(line);
        lineno++;
    }

    if (cur_macro || in_rept) {
        as_fail(filename, lineno, "unclosed .macro / .rept");
        free_state(&st);
        return -1;
    }

    int rc = iasm_assemble_body(out, st.text ? st.text : "",
                                (const char **)st.globals, st.nglobals,
                                (const char **)st.externs, st.nexterns);
    free_state(&st);
    return rc;
}
