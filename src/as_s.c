/* Gas-compatible .S assembler front-end (fakeos S3).
 *
 * Handles a small directive/macro subset, then hands AT&T instructions to
 * iasm_assemble_body.  Enough for io / usys / crt0 class stubs and for the
 * privileged ops used by later desc/sysentry migration.
 */
#include "fakecc/iasm.h"
#include "fakecc/emit.h"
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
    char *name;
    size_t off;
    int defined;
    int sect; /* 1=.rodata 2=.data 3=.bss */
} DataLab;

typedef struct {
    Macro *macros;
    int nmacros, cap;
    char **globals;
    int nglobals, gcap;
    char **externs;
    int nexterns, ecap;
    char *text;
    size_t tlen, tcap;
    int sect; /* 0=.text 1=.rodata 2=.data 3=.bss */
    EmitModule *out;
    const char *src_dir;
    DataLab *rlabs;
    int nrlab, rcap;
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
    for (int i = 0; i < st->nrlab; i++) free(st->rlabs[i].name);
    free(st->rlabs);
}

static size_t sect_len(AsState *st) {
    if (!st->out) return 0;
    if (st->sect == 1) return st->out->rodata.len;
    if (st->sect == 2) return st->out->data.len;
    if (st->sect == 3) return st->out->bss_size;
    return st->out->text.len;
}

static void sect_append_bytes(AsState *st, const void *bytes, size_t n) {
    if (!st->out || !n) return;
    if (st->sect == 1) {
        buffer_append(&st->out->rodata, bytes, n);
    } else if (st->sect == 2) {
        buffer_append(&st->out->data, bytes, n);
    } else if (st->sect == 3) {
        st->out->bss_size += n;
    } else {
        buffer_append(&st->out->text, bytes, n);
    }
}

static void sect_append_zeros(AsState *st, size_t n) {
    if (!st->out || !n) return;
    if (st->sect == 3) {
        st->out->bss_size += n;
        return;
    }
    char z = 0;
    for (size_t i = 0; i < n; i++)
        sect_append_bytes(st, &z, 1);
}

static void sect_align(AsState *st, size_t a) {
    if (!st->out || a <= 1) return;
    size_t cur = sect_len(st);
    while (cur % a) {
        sect_append_zeros(st, 1);
        cur = sect_len(st);
    }
}

static void define_data_lab(AsState *st, const char *name) {
    if (!st->out) return;
    size_t off = sect_len(st);
    for (int i = 0; i < st->nrlab; i++) {
        if (strcmp(st->rlabs[i].name, name) == 0) {
            st->rlabs[i].off = off;
            st->rlabs[i].defined = 1;
            st->rlabs[i].sect = st->sect;
            return;
        }
    }
    if (st->nrlab >= st->rcap) {
        int nc = st->rcap ? st->rcap * 2 : 8;
        st->rlabs = realloc(st->rlabs, (size_t)nc * sizeof(DataLab));
        if (!st->rlabs) exit(1);
        st->rcap = nc;
    }
    st->rlabs[st->nrlab].name = xdup(name);
    st->rlabs[st->nrlab].off = off;
    st->rlabs[st->nrlab].defined = 1;
    st->rlabs[st->nrlab].sect = st->sect;
    st->nrlab++;
}

/* Emit little-endian integer or R_X86_64_64 / absolute reloc against a symbol.
 * Returns 1 on success. */
static int emit_data_value(AsState *st, const char *tok, size_t width,
                           const char *file, int line) {
    const char *p = skip_ws(tok);
    if (!*p) {
        as_fail(file, line, "empty data value");
        return 0;
    }
    /* Optional leading '$' (gas immediate style in data is uncommon but harmless). */
    if (*p == '$') p++;

    /* Integer literal? */
    if (*p == '-' || *p == '+' || isdigit((unsigned char)*p)) {
        char *end = NULL;
        long long v = strtoll(p, &end, 0);
        if (end == p) {
            as_fail(file, line, "bad integer in data directive");
            return 0;
        }
        unsigned char buf[8];
        memset(buf, 0, sizeof buf);
        for (size_t i = 0; i < width && i < 8; i++)
            buf[i] = (unsigned char)((unsigned long long)v >> (8 * i));
        if (st->sect == 3) {
            /* Initialized value in .bss is odd; treat as reservation. */
            sect_append_zeros(st, width);
        } else {
            sect_append_bytes(st, buf, width);
        }
        return 1;
    }

    /* Symbol [+/- addend] — emit zeros + reloc (data/rodata only). */
    if (!(isalpha((unsigned char)*p) || *p == '_' || *p == '.')) {
        as_fail(file, line, "expected integer or symbol in data directive");
        return 0;
    }
    const char *s = p;
    while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '.')) p++;
    char id[128];
    size_t n = (size_t)(p - s);
    if (!n || n >= sizeof id) {
        as_fail(file, line, "bad symbol in data directive");
        return 0;
    }
    memcpy(id, s, n); id[n] = 0;
    int32_t addend = 0;
    p = skip_ws(p);
    if (*p == '+' || *p == '-') {
        char *end = NULL;
        long long adj = strtoll(p, &end, 0);
        if (end == p) {
            as_fail(file, line, "bad symbol addend");
            return 0;
        }
        addend = (int32_t)adj;
        p = end;
    }
    (void)p;
    if (st->sect != 1 && st->sect != 2) {
        as_fail(file, line, "symbol data value only in .data/.rodata");
        return 0;
    }
    if (width != 8 && width != 4) {
        as_fail(file, line, "symbol reloc needs .quad/.long");
        return 0;
    }
    size_t off = sect_len(st);
    sect_append_zeros(st, width);
    int sym = emit_module_add_undefined(st->out, id);
    /* R_X86_64_64=1 for 8-byte; for .long use same absolute family (linker
     * width follows reloc site size via emit path — store as 64-bit type and
     * 4-byte slot is still patched low 32 on hosts that honor size). */
    uint32_t rty = R_X86_64_64;
    emit_module_add_data_reloc(st->out, off, rty, sym, addend);
    if (st->sect == 1 && st->out->num_data_relocs > 0)
        st->out->data_relocs[st->out->num_data_relocs - 1].shndx = SECT_RODATA;
    return 1;
}

static int parse_data_directive(AsState *st, const char *lp,
                                const char *file, int line) {
    size_t width = 0;
    const char *args = NULL;
    int is_zero = 0;
    if (starts(lp, ".quad") || starts(lp, "dq")) {
        width = 8;
        args = skip_ws(lp + (starts(lp, ".quad") ? 5 : 2));
    } else if (starts(lp, ".long") || starts(lp, ".int") || starts(lp, "dd")) {
        width = 4;
        args = skip_ws(lp + (starts(lp, ".long") ? 5 : starts(lp, ".int") ? 4 : 2));
    } else if (starts(lp, ".word") || starts(lp, ".short") || starts(lp, "dw")) {
        width = 2;
        args = skip_ws(lp + (starts(lp, ".word") ? 5 : starts(lp, ".short") ? 6 : 2));
    } else if (starts(lp, ".byte") || starts(lp, "db")) {
        width = 1;
        args = skip_ws(lp + (starts(lp, ".byte") ? 5 : 2));
    } else if (starts(lp, ".zero") || starts(lp, ".space")) {
        is_zero = 1;
        args = skip_ws(lp + (starts(lp, ".zero") ? 5 : 6));
    } else {
        return 0; /* not a data directive */
    }

    if (is_zero) {
        char *end = NULL;
        long n = strtol(args, &end, 0);
        if (n < 0) n = 0;
        if (n > 1 << 20) {
            as_fail(file, line, ".zero/.space too large");
            return -1;
        }
        sect_append_zeros(st, (size_t)n);
        return 1;
    }

    if (!args || !*args) {
        as_fail(file, line, "data directive needs a value");
        return -1;
    }
    while (*args) {
        args = skip_ws(args);
        if (*args == ',') { args++; continue; }
        if (!*args) break;
        /* one value up to next comma */
        const char *s = args;
        int depth = 0;
        while (*args) {
            if (*args == '(') depth++;
            else if (*args == ')') { if (depth) depth--; }
            else if (*args == ',' && depth == 0) break;
            args++;
        }
        char tmp[256];
        size_t n = (size_t)(args - s);
        if (n >= sizeof tmp) n = sizeof tmp - 1;
        memcpy(tmp, s, n); tmp[n] = 0;
        /* trim trailing space */
        while (n > 0 && isspace((unsigned char)tmp[n - 1])) tmp[--n] = 0;
        if (!emit_data_value(st, tmp, width, file, line))
            return -1;
        if (*args == ',') args++;
    }
    return 1;
}

static char *join_path(const char *dir, const char *rel) {
    if (!rel || !rel[0]) return xdup("");
    if (rel[0] == '/') return xdup(rel);
    if (!dir || !dir[0] || strcmp(dir, ".") == 0) return xdup(rel);
    size_t nd = strlen(dir), nr = strlen(rel);
    char *o = malloc(nd + nr + 2);
    if (!o) exit(1);
    memcpy(o, dir, nd);
    o[nd] = '/';
    memcpy(o + nd + 1, rel, nr + 1);
    return o;
}

static int do_incbin(AsState *st, const char *path_tok, const char *file, int line) {
    char pathbuf[512];
    const char *s = skip_ws(path_tok);
    if (*s == '"' || *s == '\'') {
        char q = *s++;
        size_t n = 0;
        while (*s && *s != q && n + 1 < sizeof pathbuf) pathbuf[n++] = *s++;
        pathbuf[n] = 0;
    } else {
        size_t n = 0;
        while (*s && !isspace((unsigned char)*s) && n + 1 < sizeof pathbuf)
            pathbuf[n++] = *s++;
        pathbuf[n] = 0;
    }
    if (!pathbuf[0]) {
        as_fail(file, line, ".incbin: empty path");
        return 0;
    }
    char *full = join_path(st->src_dir, pathbuf);
    FILE *fp = fopen(full, "rb");
    if (!fp) fp = fopen(pathbuf, "rb");
    if (!fp) {
        fprintf(stderr, "%s:%d: .incbin: cannot open '%s' (tried '%s')\n",
                file ? file : "<asm>", line, pathbuf, full);
        free(full);
        return 0;
    }
    free(full);
    /* .incbin always materializes into .rodata */
    st->sect = 1;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, fp)) > 0)
        buffer_append(&st->out->rodata, buf, n);
    fclose(fp);
    return 1;
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
    if (starts(lp, ".text")) { st->sect = 0; return; }
    if (starts(lp, ".data")) { st->sect = 2; return; }
    if (starts(lp, ".bss")) { st->sect = 3; return; }
    if (starts(lp, ".section")) {
        if (strstr(lp, ".rodata"))
            st->sect = 1;
        else if (strstr(lp, ".data"))
            st->sect = 2;
        else if (strstr(lp, ".bss"))
            st->sect = 3;
        else if (strstr(lp, ".text"))
            st->sect = 0;
        return;
    }
    if (starts(lp, ".align") || starts(lp, ".p2align")) {
        int is_p2 = starts(lp, ".p2align");
        lp = skip_ws(lp + (is_p2 ? 8 : 6));
        long a = strtol(lp, NULL, 0);
        if (is_p2) {
            if (a < 0) a = 0;
            if (a > 12) a = 12;
            a = 1L << a;
        }
        if (a <= 0) a = 1;
        if (st->sect != 0) sect_align(st, (size_t)a);
        return;
    }
    if (starts(lp, ".incbin")) {
        if (!do_incbin(st, skip_ws(lp + 7), "<asm>", 0))
            exit(1);
        return;
    }
    if (starts(lp, ".type") || starts(lp, ".size") || starts(lp, ".file") ||
        starts(lp, ".ident") || starts(lp, ".code64") || starts(lp, ".intel_syntax") ||
        starts(lp, ".att_syntax") || starts(lp, ".set") || starts(lp, ".equ"))
        return;

    /* .quad/.long/.byte/.zero (and dq/dd/db) — valid in any section */
    {
        int dr = parse_data_directive(st, lp, "<asm>", 0);
        if (dr < 0) exit(1);
        if (dr > 0) return;
    }

    /* label: in data / rodata / bss */
    if (st->sect != 0) {
        const char *q = lp;
        if ((isalpha((unsigned char)*q) || *q == '_' || *q == '.')) {
            const char *s = q;
            while (*q && (isalnum((unsigned char)*q) || *q == '_' || *q == '.')) q++;
            const char *r = skip_ws(q);
            if (*r == ':') {
                char id[128];
                size_t n = (size_t)(q - s);
                if (n >= sizeof id) n = sizeof id - 1;
                memcpy(id, s, n); id[n] = 0;
                define_data_lab(st, id);
                r++;
                r = skip_ws(r);
                if (*r) ingest_line(st, r); /* label + directive/insn */
                return;
            }
        }
        /* unknown non-text content */
        return;
    }

    append_text(st, lp);
}


int assemble_s_file(const char *src, const char *filename, EmitModule *out) {

    if (!src || !out) return -1;
    AsState st;
    memset(&st, 0, sizeof st);
    st.out = out;
    {
        static char dirbuf[512];
        dirbuf[0] = '.'; dirbuf[1] = 0;
        if (filename) {
            const char *slash = NULL;
            for (const char *q = filename; *q; q++)
                if (*q == '/') slash = q;
            if (slash) {
                size_t n = (size_t)(slash - filename);
                if (n >= sizeof dirbuf) n = sizeof dirbuf - 1;
                memcpy(dirbuf, filename, n);
                dirbuf[n] = 0;
            }
        }
        st.src_dir = dirbuf;
    }

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

        /* Section switches, align, incbin, data directives, ignored metas. */
        if (starts(lp, ".incbin") || starts(lp, ".text") || starts(lp, ".data") ||
            starts(lp, ".bss") || starts(lp, ".section") || starts(lp, ".align") ||
            starts(lp, ".p2align") || starts(lp, ".type") || starts(lp, ".size") ||
            starts(lp, ".file") || starts(lp, ".ident") || starts(lp, ".code64") ||
            starts(lp, ".intel_syntax") || starts(lp, ".att_syntax") ||
            starts(lp, ".set") || starts(lp, ".equ") ||
            starts(lp, ".quad") || starts(lp, ".long") || starts(lp, ".int") ||
            starts(lp, ".word") || starts(lp, ".short") || starts(lp, ".byte") ||
            starts(lp, ".zero") || starts(lp, ".space") ||
            starts(lp, "dq") || starts(lp, "dd") || starts(lp, "dw") ||
            starts(lp, "db")) {
            ingest_line(&st, lp);
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

        ingest_line(&st, lp);
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
    /* Register .data/.rodata/.bss labels as STT_OBJECT (S3 conduit / tables). */
    for (int i = 0; i < st.nrlab; i++) {
        if (!st.rlabs[i].defined) continue;
        int is_global = 0;
        for (int g = 0; g < st.nglobals; g++)
            if (strcmp(st.globals[g], st.rlabs[i].name) == 0) {
                is_global = 1; break;
            }
        uint8_t bind = is_global ? 1 : 0; /* STB_GLOBAL / LOCAL */
        size_t off = st.rlabs[i].off;
        int sect = st.rlabs[i].sect;
        size_t sect_end = 0;
        uint16_t shndx = SECT_RODATA;
        if (sect == 2) { sect_end = out->data.len; shndx = SECT_DATA; }
        else if (sect == 3) { sect_end = out->bss_size; shndx = SECT_BSS; }
        else { sect_end = out->rodata.len; shndx = SECT_RODATA; }
        size_t end = sect_end;
        for (int j = 0; j < st.nrlab; j++) {
            if (!st.rlabs[j].defined) continue;
            if (st.rlabs[j].sect != sect) continue;
            if (st.rlabs[j].off > off && st.rlabs[j].off < end)
                end = st.rlabs[j].off;
        }
        size_t sz = end >= off ? end - off : 0;
        emit_module_add_symbol(out, st.rlabs[i].name, bind, /*STT_OBJECT*/1,
                               shndx, off, sz);
    }
    free_state(&st);
    return rc;
}
