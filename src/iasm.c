/* Minimal AT&T x86-64 assembler for GNU inline asm templates (fakeos M1). */
#include "fakecc/iasm.h"
#include "fakecc/common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    R_RAX = 0, R_RCX = 1, R_RDX = 2, R_RBX = 3,
    R_RSP = 4, R_RBP = 5, R_RSI = 6, R_RDI = 7,
    R_R8 = 8, R_R9 = 9, R_R10 = 10, R_R11 = 11,
    R_R12 = 12, R_R13 = 13, R_R14 = 14, R_R15 = 15
};

static void eb(Buffer *b, uint8_t v) { buffer_append(b, (const char *)&v, 1); }
static void e32(Buffer *b, int32_t v) {
    uint32_t u = (uint32_t)v;
    eb(b, (uint8_t)(u)); eb(b, (uint8_t)(u >> 8));
    eb(b, (uint8_t)(u >> 16)); eb(b, (uint8_t)(u >> 24));
}

static void rex(Buffer *b, int w, int r, int x, int rm) {
    int R = (r >> 3) & 1, X = (x >> 3) & 1, B = (rm >> 3) & 1;
    if (w || R || X || B || r >= 8 || rm >= 8 || x >= 8)
        eb(b, (uint8_t)(0x40 | (w << 3) | (R << 2) | (X << 1) | B));
}

static void modrm(Buffer *b, int mod, int reg, int rm) {
    eb(b, (uint8_t)(((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7)));
}

static int parse_reg_name(const char *s, int *width_out) {
    static const struct { const char *n; int r; int w; } T[] = {
        {"rax",0,8},{"eax",0,4},{"ax",0,2},{"al",0,1},{"ah",0,1},
        {"rcx",1,8},{"ecx",1,4},{"cx",1,2},{"cl",1,1},{"ch",1,1},
        {"rdx",2,8},{"edx",2,4},{"dx",2,2},{"dl",2,1},{"dh",2,1},
        {"rbx",3,8},{"ebx",3,4},{"bx",3,2},{"bl",3,1},{"bh",3,1},
        {"rsp",4,8},{"esp",4,4},{"sp",4,2},{"spl",4,1},
        {"rbp",5,8},{"ebp",5,4},{"bp",5,2},{"bpl",5,1},
        {"rsi",6,8},{"esi",6,4},{"si",6,2},{"sil",6,1},
        {"rdi",7,8},{"edi",7,4},{"di",7,2},{"dil",7,1},
        {"r8",8,8},{"r8d",8,4},{"r8w",8,2},{"r8b",8,1},
        {"r9",9,8},{"r9d",9,4},{"r9w",9,2},{"r9b",9,1},
        {"r10",10,8},{"r10d",10,4},{"r10w",10,2},{"r10b",10,1},
        {"r11",11,8},{"r11d",11,4},{"r11w",11,2},{"r11b",11,1},
        {"r12",12,8},{"r12d",12,4},{"r12w",12,2},{"r12b",12,1},
        {"r13",13,8},{"r13d",13,4},{"r13w",13,2},{"r13b",13,1},
        {"r14",14,8},{"r14d",14,4},{"r14w",14,2},{"r14b",14,1},
        {"r15",15,8},{"r15d",15,4},{"r15w",15,2},{"r15b",15,1},
    };
    for (size_t i = 0; i < sizeof T / sizeof T[0]; i++) {
        size_t n = strlen(T[i].n);
        if (strncmp(s, T[i].n, n) == 0 && !isalnum((unsigned char)s[n]) && s[n] != '_') {
            if (width_out) *width_out = T[i].w;
            return T[i].r;
        }
    }
    return -1;
}

/* XMM0..XMM15 — used by movdqa (sysentry/desc save/restore). */
static int parse_xmm_name(const char *s, const char **end_out) {
    if (!((s[0] == 'x' || s[0] == 'X') &&
          (s[1] == 'm' || s[1] == 'M') &&
          (s[2] == 'm' || s[2] == 'M')))
        return -1;
    s += 3;
    if (!isdigit((unsigned char)*s)) return -1;
    int n = 0;
    while (isdigit((unsigned char)*s)) {
        n = n * 10 + (*s - '0');
        s++;
    }
    if (n < 0 || n > 15) return -1;
    if (isalnum((unsigned char)*s) || *s == '_') return -1;
    if (end_out) *end_out = s;
    return n;
}

/* CR0..CR8 — mov %crN / mov to %crN (desc helpers). */
static int parse_cr_name(const char *s, const char **end_out) {
    if (!((s[0] == 'c' || s[0] == 'C') && (s[1] == 'r' || s[1] == 'R')))
        return -1;
    s += 2;
    if (!isdigit((unsigned char)*s)) return -1;
    int n = 0;
    while (isdigit((unsigned char)*s)) {
        n = n * 10 + (*s - '0');
        s++;
    }
    if (n < 0 || n > 8) return -1;
    if (isalnum((unsigned char)*s) || *s == '_') return -1;
    if (end_out) *end_out = s;
    return n;
}

/* Segment regs for mov %ax,%ds etc. Encoding: ES=0 CS=1 SS=2 DS=3 FS=4 GS=5. */
static int parse_sreg_name(const char *s, const char **end_out) {
    static const struct { const char *n; int r; } T[] = {
        {"es",0},{"cs",1},{"ss",2},{"ds",3},{"fs",4},{"gs",5},
    };
    for (size_t i = 0; i < sizeof T / sizeof T[0]; i++) {
        size_t n = strlen(T[i].n);
        if (strncmp(s, T[i].n, n) == 0 && !isalnum((unsigned char)s[n]) && s[n] != '_') {
            if (end_out) *end_out = s + n;
            return T[i].r;
        }
    }
    return -1;
}

/* Skip whitespace. */
static const char *skip_ws(const char *p) {
    while (*p && isspace((unsigned char)*p) && *p != '\n') p++;
    return p;
}

/* Local label table for jumps within one template. */
typedef struct {
    char *name;
    size_t off;
    int defined;
} LocLab;

typedef struct {
    size_t patch_off;
    char *name;
    int is_call; /* 1=call(E8), 0=jmp/jcc, 2=RIP-rel displ32 (lea/mov/lgdt) */
    int jcc_op;  /* 0xEB short jmp, 0xE9 near, or 0x0F8x jcc */
} LocFix;

#define MAX_LOCLAB 256
#define MAX_LOCFIX 512

static int is_ident_start(int c) {
    return isalpha(c) || c == '_' || c == '.';
}
static int is_ident(int c) {
    return isalnum(c) || c == '_' || c == '.';
}

static char *dup_ident(const char **pp) {
    const char *p = *pp;
    if (!is_ident_start((unsigned char)*p)) return NULL;
    const char *s = p;
    p++;
    while (is_ident((unsigned char)*p)) p++;
    size_t n = (size_t)(p - s);
    char *o = malloc(n + 1);
    if (!o) return NULL;
    memcpy(o, s, n);
    o[n] = 0;
    *pp = p;
    return o;
}

static int parse_imm(const char **pp, int64_t *out) {
    const char *p = skip_ws(*pp);
    int neg = 0;
    if (*p == '-') { neg = 1; p++; }
    if (*p == '$') p++;
    p = skip_ws(p);
    if (!isdigit((unsigned char)*p) && !(p[0]=='0' && (p[1]=='x'||p[1]=='X')))
        return 0;
    char *end = NULL;
    int64_t v = strtoll(p, &end, 0);
    if (end == p) return 0;
    if (neg) v = -v;
    *out = v;
    *pp = end;
    return 1;
}

/* Expand %N / %bN / %% in template into a concrete AT&T string. */
static char *expand_template(const char *tmpl, const IAsmOperand *ops, int nops) {
    size_t cap = strlen(tmpl) * 4 + 64;
    char *out = malloc(cap);
    if (!out) return NULL;
    size_t len = 0;
    const char *p = tmpl;
    while (*p) {
        if (p[0] == '%' && p[1] == '%') {
            if (len + 1 >= cap) { cap *= 2; out = realloc(out, cap); }
            out[len++] = '%';
            p += 2;
            continue;
        }
        if (p[0] == '%' && (isdigit((unsigned char)p[1]) ||
                            ((p[1]=='b'||p[1]=='w'||p[1]=='k'||p[1]=='q'||
                              p[1]=='h'||p[1]=='c') && isdigit((unsigned char)p[2])))) {
            int size_ovr = 0; /* 0=default, 1/2/4/8 */
            p++;
            if (*p == 'b') { size_ovr = 1; p++; }
            else if (*p == 'w') { size_ovr = 2; p++; }
            else if (*p == 'k') { size_ovr = 4; p++; }
            else if (*p == 'q') { size_ovr = 8; p++; }
            else if (*p == 'h' || *p == 'c') { p++; } /* ignore rarely used */
            int idx = 0;
            while (isdigit((unsigned char)*p)) {
                idx = idx * 10 + (*p - '0');
                p++;
            }
            if (idx < 0 || idx >= nops) {
                free(out);
                return NULL;
            }
            const IAsmOperand *op = &ops[idx];
            int w = size_ovr ? size_ovr : (op->width > 0 ? op->width : 8);
            char buf[64];
            buf[0] = 0;
            if (op->kind == 1) {
                snprintf(buf, sizeof buf, "$%lld", (long long)op->imm);
            } else if (op->kind == 2) {
                /* memory */
                static const char *rn64[] = {
                    "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
                    "r8","r9","r10","r11","r12","r13","r14","r15"
                };
                snprintf(buf, sizeof buf, "(%s)", rn64[op->reg & 15]);
            } else if (op->kind == 3 && op->sym) {
                snprintf(buf, sizeof buf, "%s", op->sym);
            } else {
                /* register */
                static const char *r8[] = {
                    "al","cl","dl","bl","spl","bpl","sil","dil",
                    "r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"
                };
                static const char *r16[] = {
                    "ax","cx","dx","bx","sp","bp","si","di",
                    "r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"
                };
                static const char *r32[] = {
                    "eax","ecx","edx","ebx","esp","ebp","esi","edi",
                    "r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"
                };
                static const char *r64[] = {
                    "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
                    "r8","r9","r10","r11","r12","r13","r14","r15"
                };
                const char *nm = r64[op->reg & 15];
                if (w == 1) nm = r8[op->reg & 15];
                else if (w == 2) nm = r16[op->reg & 15];
                else if (w == 4) nm = r32[op->reg & 15];
                snprintf(buf, sizeof buf, "%%%s", nm);
            }
            size_t bl = strlen(buf);
            if (len + bl + 1 >= cap) { cap = (len + bl + 1) * 2; out = realloc(out, cap); }
            memcpy(out + len, buf, bl);
            len += bl;
            continue;
        }
        if (len + 1 >= cap) { cap *= 2; out = realloc(out, cap); }
        out[len++] = *p++;
    }
    out[len] = 0;
    return out;
}

static int lookup_lab(LocLab *labs, int n, const char *name) {
    for (int i = 0; i < n; i++)
        if (strcmp(labs[i].name, name) == 0) return i;
    return -1;
}

static void emit_mov_rr(Buffer *b, int dst, int src, int w) {
    if (w == 1) {
        rex(b, 0, src, 0, dst);
        eb(b, 0x88); modrm(b, 3, src, dst);
    } else if (w == 2) {
        eb(b, 0x66);
        rex(b, 0, src, 0, dst);
        eb(b, 0x89); modrm(b, 3, src, dst);
    } else if (w == 4) {
        rex(b, 0, src, 0, dst);
        eb(b, 0x89); modrm(b, 3, src, dst);
    } else {
        rex(b, 1, src, 0, dst);
        eb(b, 0x89); modrm(b, 3, src, dst);
    }
}

static void emit_mov_ri(Buffer *b, int dst, int64_t imm, int w) {
    if (w == 1) {
        rex(b, 0, 0, 0, dst);
        eb(b, (uint8_t)(0xB0 + (dst & 7)));
        eb(b, (uint8_t)imm);
    } else if (w == 2) {
        eb(b, 0x66);
        rex(b, 0, 0, 0, dst);
        eb(b, (uint8_t)(0xB8 + (dst & 7)));
        eb(b, (uint8_t)imm); eb(b, (uint8_t)(imm >> 8));
    } else if (w == 4) {
        rex(b, 0, 0, 0, dst);
        eb(b, (uint8_t)(0xB8 + (dst & 7)));
        e32(b, (int32_t)imm);
    } else {
        /* 64-bit: use movabs if needed, else mov r32 + zero-extend via 32-bit */
        if (imm == (int64_t)(int32_t)imm) {
            rex(b, 1, 0, 0, dst);
            eb(b, 0xC7); modrm(b, 3, 0, dst);
            e32(b, (int32_t)imm);
        } else if (imm == (int64_t)(uint32_t)imm) {
            rex(b, 0, 0, 0, dst);
            eb(b, (uint8_t)(0xB8 + (dst & 7)));
            e32(b, (int32_t)imm);
        } else {
            rex(b, 1, 0, 0, dst);
            eb(b, (uint8_t)(0xB8 + (dst & 7)));
            for (int i = 0; i < 8; i++) eb(b, (uint8_t)(imm >> (8 * i)));
        }
    }
}


static void modrm_mem(Buffer *b, int reg, int base, int64_t disp) {
    int use_sib = ((base & 7) == 4);
    if (disp == 0 && (base & 7) != 5) {
        if (use_sib) { modrm(b, 0, reg, 4); eb(b, 0x24); }
        else modrm(b, 0, reg, base);
    } else if (disp == (int64_t)(int8_t)disp) {
        if (use_sib) { modrm(b, 1, reg, 4); eb(b, 0x24); }
        else modrm(b, 1, reg, base);
        eb(b, (uint8_t)disp);
    } else {
        if (use_sib) { modrm(b, 2, reg, 4); eb(b, 0x24); }
        else modrm(b, 2, reg, base);
        e32(b, (int32_t)disp);
    }
}

static void emit_mov_rm(Buffer *b, int dst, int base, int64_t disp, int w) {
    /* mov dst, disp(base) */
    if (w == 1) {
        rex(b, 0, dst, 0, base);
        eb(b, 0x8A);
    } else if (w == 2) {
        eb(b, 0x66); rex(b, 0, dst, 0, base); eb(b, 0x8B);
    } else if (w == 4) {
        rex(b, 0, dst, 0, base); eb(b, 0x8B);
    } else {
        rex(b, 1, dst, 0, base); eb(b, 0x8B);
    }
    modrm_mem(b, dst, base, disp);
}

static void emit_mov_mr(Buffer *b, int base, int64_t disp, int src, int w) {
    if (w == 1) {
        rex(b, 0, src, 0, base); eb(b, 0x88);
    } else if (w == 2) {
        eb(b, 0x66); rex(b, 0, src, 0, base); eb(b, 0x89);
    } else if (w == 4) {
        rex(b, 0, src, 0, base); eb(b, 0x89);
    } else {
        rex(b, 1, src, 0, base); eb(b, 0x89);
    }
    modrm_mem(b, src, base, disp);
}

/* Absolute [disp32] ModRM+SIB used by %fs:disp / %gs:disp. */
static void emit_abs_disp32(Buffer *b, int reg) {
    modrm(b, 0, reg, 4); /* mod=00 rm=4 → SIB */
    eb(b, 0x25);         /* scale=0 index=4 base=5 → [disp32] */
}

static void emit_seg_override(Buffer *b, int sreg) {
    if (sreg == 4) eb(b, 0x64);      /* %fs */
    else if (sreg == 5) eb(b, 0x65); /* %gs */
}

/* mov dst, %fs/%gs:disp32 */
static void emit_mov_seg_abs_load(Buffer *b, int dst, int sreg, int64_t disp, int w) {
    emit_seg_override(b, sreg);
    if (w == 1) {
        rex(b, 0, dst, 0, 0); eb(b, 0x8A);
    } else if (w == 2) {
        eb(b, 0x66); rex(b, 0, dst, 0, 0); eb(b, 0x8B);
    } else if (w == 4) {
        rex(b, 0, dst, 0, 0); eb(b, 0x8B);
    } else {
        rex(b, 1, dst, 0, 0); eb(b, 0x8B);
    }
    emit_abs_disp32(b, dst);
    e32(b, (int32_t)disp);
}

/* mov %fs/%gs:disp32, src */
static void emit_mov_seg_abs_store(Buffer *b, int src, int sreg, int64_t disp, int w) {
    emit_seg_override(b, sreg);
    if (w == 1) {
        rex(b, 0, src, 0, 0); eb(b, 0x88);
    } else if (w == 2) {
        eb(b, 0x66); rex(b, 0, src, 0, 0); eb(b, 0x89);
    } else if (w == 4) {
        rex(b, 0, src, 0, 0); eb(b, 0x89);
    } else {
        rex(b, 1, src, 0, 0); eb(b, 0x89);
    }
    emit_abs_disp32(b, src);
    e32(b, (int32_t)disp);
}

/* Parse a single operand token into kind/reg/imm/sym. Consumes from *pp.
 * kind 0=reg 1=imm 2=mem(reg[+disp in *imm]) 3=symbol 4=sym(%rip) 5=xmm
 * 6=cr 7=sreg 8=seg:disp32 (absolute mem with FS/GS override; *reg=sreg). */
static int parse_operand(const char **pp, int *kind, int *reg, int64_t *imm,
                         char **sym, int *width) {
    const char *p = skip_ws(*pp);
    *kind = -1; *reg = 0; *imm = 0; *sym = NULL; *width = 8;

    /* AT&T indirection star: call/jmp *%rax, *0(%rdi), *sym(%rip) */
    if (*p == '*') p = skip_ws(p + 1);

    /* Peek: disp(%reg) or (%reg) */
    {
        const char *q = p;
        int64_t disp = 0;
        int has_disp = 0;
        if (*q == '(') {
            /* bare (%reg) */
        } else if (*q == '-' || *q == '+' || isdigit((unsigned char)*q)) {
            const char *save = q;
            if (!parse_imm(&q, &disp)) goto not_mem;
            q = skip_ws(q);
            if (*q != '(') goto not_mem;
            has_disp = 1;
            (void)save;
        } else {
            goto not_mem;
        }
        if (*q != '(') goto not_mem;
        q++;
        q = skip_ws(q);
        if (*q != '%') goto not_mem;
        q++;
        int w = 8;
        int r = parse_reg_name(q, &w);
        if (r < 0) goto not_mem;
        while (isalnum((unsigned char)*q) || *q == '_') q++;
        q = skip_ws(q);
        if (*q != ')') goto not_mem;
        q++;
        *kind = 2; *reg = r; *width = w; *imm = has_disp ? disp : 0;
        *pp = q;
        return 1;
    }
not_mem:
    if (*p == '$' || (*p == '-' && p[1] == '$') ||
        (isdigit((unsigned char)*p) || (*p == '-' && isdigit((unsigned char)p[1])))) {
        if (!parse_imm(&p, imm)) return 0;
        *kind = 1;
        *pp = p;
        return 1;
    }
    if (*p == '%') {
        p++;
        const char *xend = NULL;
        int xr = parse_xmm_name(p, &xend);
        if (xr >= 0) {
            *kind = 5; *reg = xr; *width = 16;
            *pp = xend;
            return 1;
        }
        int cr = parse_cr_name(p, &xend);
        if (cr >= 0) {
            *kind = 6; *reg = cr; *width = 8;
            *pp = xend;
            return 1;
        }
        int sr = parse_sreg_name(p, &xend);
        if (sr >= 0) {
            const char *q = skip_ws(xend);
            /* %gs:disp / %fs:disp — absolute disp32 with segment override */
            if (*q == ':') {
                int64_t disp = 0;
                q = skip_ws(q + 1);
                if (!parse_imm(&q, &disp)) return 0;
                *kind = 8; *reg = sr; *width = 8; *imm = disp;
                *pp = q;
                return 1;
            }
            *kind = 7; *reg = sr; *width = 2;
            *pp = xend;
            return 1;
        }
        int w = 8;
        int r = parse_reg_name(p, &w);
        if (r < 0) return 0;
        while (isalnum((unsigned char)*p) || *p == '_') p++;
        *kind = 0; *reg = r; *width = w;
        *pp = p;
        return 1;
    }
    if (is_ident_start((unsigned char)*p)) {
        char *id = dup_ident(&p);
        if (!id) return 0;
        const char *q = skip_ws(p);
        if (q[0] == '(' && q[1] == '%' &&
            (strncmp(q + 2, "rip)", 4) == 0 || strncmp(q + 2, "RIP)", 4) == 0)) {
            *kind = 4;
            *sym = id;
            *pp = q + 6;
            return 1;
        }
        *kind = 3; *sym = id;
        *pp = p;
        return 1;
    }
    return 0;
}

static int emit_unary_no_op(Buffer *b, const char *mn) {
    if (strcmp(mn, "cli") == 0) { eb(b, 0xFA); return 1; }
    if (strcmp(mn, "sti") == 0) { eb(b, 0xFB); return 1; }
    if (strcmp(mn, "hlt") == 0) { eb(b, 0xF4); return 1; }
    if (strcmp(mn, "nop") == 0) { eb(b, 0x90); return 1; }
    if (strcmp(mn, "ret") == 0 || strcmp(mn, "retq") == 0) { eb(b, 0xC3); return 1; }
    if (strcmp(mn, "leave") == 0) { eb(b, 0xC9); return 1; }
    if (strcmp(mn, "cld") == 0) { eb(b, 0xFC); return 1; }
    if (strcmp(mn, "std") == 0) { eb(b, 0xFD); return 1; }
    if (strcmp(mn, "cmc") == 0) { eb(b, 0xF5); return 1; }
    if (strcmp(mn, "clc") == 0) { eb(b, 0xF8); return 1; }
    if (strcmp(mn, "stc") == 0) { eb(b, 0xF9); return 1; }
    if (strcmp(mn, "pause") == 0) { eb(b, 0xF3); eb(b, 0x90); return 1; }
    if (strcmp(mn, "syscall") == 0) { eb(b, 0x0F); eb(b, 0x05); return 1; }
    if (strcmp(mn, "sysret") == 0 || strcmp(mn, "sysretq") == 0) {
        eb(b, 0x48); eb(b, 0x0F); eb(b, 0x07); return 1;
    }
    if (strcmp(mn, "iret") == 0 || strcmp(mn, "iretq") == 0) {
        eb(b, 0x48); eb(b, 0xCF); return 1;
    }
    if (strcmp(mn, "retfq") == 0 || strcmp(mn, "lretq") == 0 ||
        strcmp(mn, "lret") == 0) {
        /* REX.W far return: pops RIP then CS (gdt_flush) */
        eb(b, 0x48); eb(b, 0xCB); return 1;
    }
    if (strcmp(mn, "pushfq") == 0 || strcmp(mn, "pushf") == 0) { eb(b, 0x9C); return 1; }
    if (strcmp(mn, "popfq") == 0 || strcmp(mn, "popf") == 0) { eb(b, 0x9D); return 1; }
    if (strcmp(mn, "cqto") == 0 || strcmp(mn, "cqo") == 0) { eb(b, 0x48); eb(b, 0x99); return 1; }
    if (strcmp(mn, "cltd") == 0 || strcmp(mn, "cdq") == 0) { eb(b, 0x99); return 1; }
    if (strcmp(mn, "cwtl") == 0 || strcmp(mn, "cwde") == 0) { eb(b, 0x98); return 1; }
    if (strcmp(mn, "cltq") == 0 || strcmp(mn, "cdqe") == 0) { eb(b, 0x48); eb(b, 0x98); return 1; }
    if (strcmp(mn, "ud2") == 0) { eb(b, 0x0F); eb(b, 0x0B); return 1; }
    if (strcmp(mn, "int3") == 0) { eb(b, 0xCC); return 1; }
    if (strcmp(mn, "rdmsr") == 0) { eb(b, 0x0F); eb(b, 0x32); return 1; }
    if (strcmp(mn, "wrmsr") == 0) { eb(b, 0x0F); eb(b, 0x30); return 1; }
    return 0;
}

static int jcc_opcode(const char *mn) {
    /* return 0x80+cc for near jcc, or -1 */
    static const struct { const char *n; int cc; } T[] = {
        {"jo",0},{"jno",1},{"jb",2},{"jnae",2},{"jc",2},
        {"jnb",3},{"jae",3},{"jnc",3},
        {"je",4},{"jz",4},{"jne",5},{"jnz",5},
        {"jbe",6},{"jna",6},{"ja",7},{"jnbe",7},
        {"js",8},{"jns",9},{"jp",10},{"jpe",10},{"jnp",11},{"jpo",11},
        {"jl",12},{"jnge",12},{"jge",13},{"jnl",13},
        {"jle",14},{"jng",14},{"jg",15},{"jnle",15},
    };
    for (size_t i = 0; i < sizeof T / sizeof T[0]; i++)
        if (strcmp(mn, T[i].n) == 0) return 0x80 | T[i].cc;
    return -1;
}

/* Queue a RIP-relative displ32 patch (lea/mov/lgdt sym(%rip)).
 * Resolved against LocLab if present; else becomes R_X86_64_PC32. */
static int queue_rip_rel(LocFix *fixes, int *nfix, size_t patch_off, char *sym) {
    if (*nfix >= MAX_LOCFIX) { free(sym); return 0; }
    fixes[*nfix].patch_off = patch_off;
    fixes[*nfix].name = sym;
    fixes[*nfix].is_call = 2;
    fixes[*nfix].jcc_op = 0;
    (*nfix)++;
    return 1;
}

static int emit_line(Buffer *b, EmitModule *out, const char *line,
                     LocLab *labs, int *nlab, LocFix *fixes, int *nfix) {
    const char *p = skip_ws(line);
    if (!*p || *p == '#' || (p[0] == '/' && p[1] == '/')) return 1;

    /* label: */
    if (is_ident_start((unsigned char)*p)) {
        const char *save = p;
        char *id = dup_ident(&p);
        p = skip_ws(p);
        if (*p == ':') {
            int idx = lookup_lab(labs, *nlab, id);
            if (idx < 0) {
                if (*nlab >= MAX_LOCLAB) { free(id); return 0; }
                labs[*nlab].name = id;
                labs[*nlab].off = b->len;
                labs[*nlab].defined = 1;
                (*nlab)++;
            } else {
                labs[idx].off = b->len;
                labs[idx].defined = 1;
                free(id);
            }
            p++;
            return emit_line(b, out, p, labs, nlab, fixes, nfix);
        }
        free(id);
        p = save;
    }

    /* directives .byte / .long / .quad */
    if (*p == '.') {
        char *dir = dup_ident(&p);
        if (!dir) return 0;
        if (strcmp(dir, ".byte") == 0 || strcmp(dir, ".db") == 0) {
            free(dir);
            for (;;) {
                p = skip_ws(p);
                if (!*p) break;
                int64_t v = 0;
                if (!parse_imm(&p, &v)) return 0;
                eb(b, (uint8_t)v);
                p = skip_ws(p);
                if (*p == ',') { p++; continue; }
                break;
            }
            return 1;
        }
        if (strcmp(dir, ".long") == 0 || strcmp(dir, ".int") == 0) {
            free(dir);
            for (;;) {
                p = skip_ws(p);
                if (!*p) break;
                int64_t v = 0;
                if (!parse_imm(&p, &v)) return 0;
                e32(b, (int32_t)v);
                p = skip_ws(p);
                if (*p == ',') { p++; continue; }
                break;
            }
            return 1;
        }
        if (strcmp(dir, ".quad") == 0) {
            free(dir);
            for (;;) {
                p = skip_ws(p);
                if (!*p) break;
                int64_t v = 0;
                if (!parse_imm(&p, &v)) return 0;
                for (int i = 0; i < 8; i++) eb(b, (uint8_t)(v >> (8 * i)));
                p = skip_ws(p);
                if (*p == ',') { p++; continue; }
                break;
            }
            return 1;
        }
        /* unknown directive: ignore */
        free(dir);
        return 1;
    }

    char *mn = dup_ident(&p);
    if (!mn) return 1; /* empty */
    /* lowercase mnemonic */
    for (char *q = mn; *q; q++) *q = (char)tolower((unsigned char)*q);

    if (emit_unary_no_op(b, mn)) { free(mn); return 1; }

    p = skip_ws(p);

    /* push / pop */
    if (strcmp(mn, "push") == 0 || strcmp(mn, "pushq") == 0) {
        int k, r, w; int64_t imm; char *sym = NULL;
        if (!parse_operand(&p, &k, &r, &imm, &sym, &w)) { free(mn); return 0; }
        if (k == 0) {
            rex(b, 0, 0, 0, r);
            eb(b, (uint8_t)(0x50 + (r & 7)));
        } else if (k == 1) {
            if (imm == (int64_t)(int8_t)imm) {
                eb(b, 0x6A); eb(b, (uint8_t)imm);
            } else {
                eb(b, 0x68); e32(b, (int32_t)imm);
            }
        } else { free(sym); free(mn); return 0; }
        free(sym); free(mn); return 1;
    }
    if (strcmp(mn, "pop") == 0 || strcmp(mn, "popq") == 0) {
        int k, r, w; int64_t imm; char *sym = NULL;
        if (!parse_operand(&p, &k, &r, &imm, &sym, &w) || k != 0) {
            free(sym); free(mn); return 0;
        }
        rex(b, 0, 0, 0, r);
        eb(b, (uint8_t)(0x58 + (r & 7)));
        free(sym); free(mn); return 1;
    }

    /* shl/shr/sal/sar $imm, %reg  and  div/idiv %reg */
    if (strcmp(mn, "shl") == 0 || strcmp(mn, "sal") == 0 ||
        strcmp(mn, "shr") == 0 || strcmp(mn, "sar") == 0 ||
        strcmp(mn, "shlq") == 0 || strcmp(mn, "shrq") == 0) {
        int k1, r1, w1, k2, r2, w2;
        int64_t imm1 = 0, imm2 = 0;
        char *sym1 = NULL, *sym2 = NULL;
        if (!parse_operand(&p, &k1, &r1, &imm1, &sym1, &w1)) { free(mn); return 0; }
        p = skip_ws(p);
        if (*p != ',') { free(sym1); free(mn); return 0; }
        p++;
        if (!parse_operand(&p, &k2, &r2, &imm2, &sym2, &w2)) {
            free(sym1); free(mn); return 0;
        }
        int ext = 4; /* shl/sal */
        if (strcmp(mn, "shr") == 0 || strcmp(mn, "shrq") == 0) ext = 5;
        else if (strcmp(mn, "sar") == 0) ext = 7;
        if (k1 == 1 && k2 == 0) {
            int w = w2;
            if (w == 4) rex(b, 0, 0, 0, r2);
            else rex(b, 1, 0, 0, r2);
            if (imm1 == 1) {
                eb(b, 0xD1); modrm(b, 3, ext, r2);
            } else {
                eb(b, 0xC1); modrm(b, 3, ext, r2); eb(b, (uint8_t)imm1);
            }
            free(sym1); free(sym2); free(mn); return 1;
        }
        free(sym1); free(sym2); free(mn); return 0;
    }
    if (strcmp(mn, "div") == 0 || strcmp(mn, "idiv") == 0 ||
        strcmp(mn, "divq") == 0 || strcmp(mn, "idivq") == 0 ||
        strcmp(mn, "divl") == 0 || strcmp(mn, "idivl") == 0) {
        int k, r, w; int64_t imm; char *sym = NULL;
        if (!parse_operand(&p, &k, &r, &imm, &sym, &w) || k != 0) {
            free(sym); free(mn); return 0;
        }
        int ext = (mn[0] == 'i') ? 7 : 6;
        int use64 = !(strcmp(mn, "divl") == 0 || strcmp(mn, "idivl") == 0);
        if (w == 4) use64 = 0;
        if (use64) rex(b, 1, 0, 0, r);
        else rex(b, 0, 0, 0, r);
        eb(b, 0xF7); modrm(b, 3, ext, r);
        free(sym); free(mn); return 1;
    }

    /* jmp / call / jcc */
    int jcc = jcc_opcode(mn);
    if (strcmp(mn, "jmp") == 0 || strcmp(mn, "call") == 0 || jcc >= 0) {
        int is_call = (strcmp(mn, "call") == 0);
        int k, r, w; int64_t imm; char *sym = NULL;
        if (!parse_operand(&p, &k, &r, &imm, &sym, &w)) { free(mn); return 0; }
        if (k == 0) {
            /* jmp/call *%reg — use FF /4 or /2 */
            rex(b, 0, 0, 0, r);
            eb(b, 0xFF);
            modrm(b, 3, is_call ? 2 : 4, r);
            free(sym); free(mn); return 1;
        }
        if (k == 3 && sym) {
            /* local or external */
            int li = lookup_lab(labs, *nlab, sym);
            if (li >= 0 && labs[li].defined && !is_call && jcc < 0) {
                /* backward near jmp */
                eb(b, 0xE9);
                int32_t rel = (int32_t)((int64_t)labs[li].off - (int64_t)(b->len + 4));
                e32(b, rel);
                free(sym); free(mn); return 1;
            }
            if (li >= 0 && labs[li].defined && jcc >= 0) {
                eb(b, 0x0F); eb(b, (uint8_t)jcc);
                int32_t rel = (int32_t)((int64_t)labs[li].off - (int64_t)(b->len + 4));
                e32(b, rel);
                free(sym); free(mn); return 1;
            }
            /* forward local or external */
            if (*nfix >= MAX_LOCFIX) { free(sym); free(mn); return 0; }
            if (jcc >= 0) {
                eb(b, 0x0F); eb(b, (uint8_t)jcc);
                fixes[*nfix].patch_off = b->len;
                fixes[*nfix].name = sym;
                fixes[*nfix].is_call = 0;
                fixes[*nfix].jcc_op = jcc;
                e32(b, 0);
            } else if (is_call) {
                eb(b, 0xE8);
                fixes[*nfix].patch_off = b->len;
                fixes[*nfix].name = sym;
                fixes[*nfix].is_call = 1;
                fixes[*nfix].jcc_op = 0;
                e32(b, 0);
            } else {
                eb(b, 0xE9);
                fixes[*nfix].patch_off = b->len;
                fixes[*nfix].name = sym;
                fixes[*nfix].is_call = 0;
                fixes[*nfix].jcc_op = 0;
                e32(b, 0);
            }
            (*nfix)++;
            free(mn); return 1;
        }
        free(sym); free(mn); return 0;
    }

    /* one-operand privileged / system */
    if (strcmp(mn, "lgdt") == 0 || strcmp(mn, "lidt") == 0 ||
        strcmp(mn, "ltr") == 0 || strcmp(mn, "invlpg") == 0) {
        int k, r, w; int64_t imm; char *sym = NULL;
        if (!parse_operand(&p, &k, &r, &imm, &sym, &w)) { free(mn); return 0; }
        int ext = 2;
        if (strcmp(mn, "lidt") == 0) ext = 3;
        else if (strcmp(mn, "ltr") == 0) ext = 3;
        else if (strcmp(mn, "invlpg") == 0) ext = 7;
        if (strcmp(mn, "ltr") == 0) {
            if (k == 0) {
                eb(b, 0x0F); eb(b, 0x00); modrm(b, 3, 3, r);
                free(sym); free(mn); return 1;
            }
            if (k == 2) {
                rex(b, 0, 0, 0, r); eb(b, 0x0F); eb(b, 0x00);
                if ((r & 7) == 4) { modrm(b, 0, 3, 4); eb(b, 0x24); }
                else modrm(b, 0, 3, r);
                free(sym); free(mn); return 1;
            }
        } else if (k == 2) {
            /* lgdt/lidt/invlpg m */
            rex(b, 0, 0, 0, r); eb(b, 0x0F); eb(b, 0x01);
            if ((r & 7) == 4) { modrm(b, 0, ext, 4); eb(b, 0x24); }
            else if ((r & 7) == 5) { modrm(b, 1, ext, 5); eb(b, 0); }
            else modrm(b, 0, ext, r);
            free(sym); free(mn); return 1;
        } else if (k == 4 && sym &&
                   (strcmp(mn, "lgdt") == 0 || strcmp(mn, "lidt") == 0)) {
            /* lgdt/lidt sym(%rip) */
            eb(b, 0x0F); eb(b, 0x01); modrm(b, 0, ext, 5);
            size_t po = b->len; e32(b, 0);
            if (!queue_rip_rel(fixes, nfix, po, sym)) { free(mn); return 0; }
            free(mn); return 1;
        }
        free(sym); free(mn); return 0;
    }

    /* two-operand: mov, out, in, xchg, test, add, sub, xor, and, or, cmp */
    int k1, r1, w1, k2, r2, w2;
    int64_t imm1 = 0, imm2 = 0;
    char *sym1 = NULL, *sym2 = NULL;
    if (!parse_operand(&p, &k1, &r1, &imm1, &sym1, &w1)) { free(mn); return 0; }
    p = skip_ws(p);
    if (*p != ',') { free(sym1); free(mn); return 0; }
    p++;
    if (!parse_operand(&p, &k2, &r2, &imm2, &sym2, &w2)) {
        free(sym1); free(mn); return 0;
    }

    int handled = 0;
    if (strcmp(mn, "mov") == 0 || strcmp(mn, "movq") == 0 ||
        strcmp(mn, "movl") == 0 || strcmp(mn, "movw") == 0 ||
        strcmp(mn, "movb") == 0) {
        int w = 8;
        if (strcmp(mn, "movb") == 0) w = 1;
        else if (strcmp(mn, "movw") == 0) w = 2;
        else if (strcmp(mn, "movl") == 0) w = 4;
        else if (strcmp(mn, "movq") == 0) w = 8;
        else {
            /* infer from operands */
            if (k1 == 0) w = w1;
            else if (k2 == 0) w = w2;
            else if (k1 == 2) w = w1;
            else if (k2 == 2) w = w2;
        }
        /* AT&T: mov src, dst */
        if (k1 == 0 && k2 == 0) { emit_mov_rr(b, r2, r1, w); handled = 1; }
        else if (k1 == 1 && k2 == 0) { emit_mov_ri(b, r2, imm1, w); handled = 1; }
        else if (k1 == 0 && k2 == 2) { emit_mov_mr(b, r2, imm2, r1, w); handled = 1; }
        else if (k1 == 2 && k2 == 0) { emit_mov_rm(b, r2, r1, imm1, w); handled = 1; }
        else if (k1 == 1 && k2 == 2) {
            /* mov $imm, (reg) */
            if (w == 1) {
                rex(b, 0, 0, 0, r2); eb(b, 0xC6);
                if ((r2 & 7) == 4) { modrm(b, 0, 0, 4); eb(b, 0x24); }
                else if ((r2 & 7) == 5) { modrm(b, 1, 0, 5); eb(b, 0); }
                else modrm(b, 0, 0, r2);
                eb(b, (uint8_t)imm1);
            } else if (w == 4) {
                rex(b, 0, 0, 0, r2); eb(b, 0xC7);
                if ((r2 & 7) == 4) { modrm(b, 0, 0, 4); eb(b, 0x24); }
                else if ((r2 & 7) == 5) { modrm(b, 1, 0, 5); eb(b, 0); }
                else modrm(b, 0, 0, r2);
                e32(b, (int32_t)imm1);
            } else {
                rex(b, 1, 0, 0, r2); eb(b, 0xC7);
                if ((r2 & 7) == 4) { modrm(b, 0, 0, 4); eb(b, 0x24); }
                else if ((r2 & 7) == 5) { modrm(b, 1, 0, 5); eb(b, 0); }
                else modrm(b, 0, 0, r2);
                e32(b, (int32_t)imm1);
            }
            handled = 1;
        } else if (k1 == 4 && k2 == 0 && sym1) {
            /* mov sym(%rip), %reg */
            int rw = (strcmp(mn, "movl") == 0) ? 4 : 8;
            if (strcmp(mn, "movb") == 0) rw = 1;
            else if (strcmp(mn, "movw") == 0) rw = 2;
            rex(b, rw == 8, r2, 0, 0); eb(b, rw == 1 ? 0x8A : 0x8B); modrm(b, 0, r2, 5);
            size_t po = b->len; e32(b, 0);
            if (!queue_rip_rel(fixes, nfix, po, sym1)) { free(sym2); free(mn); return 0; }
            sym1 = NULL;
            handled = 1;
        } else if (k1 == 0 && k2 == 4 && sym2) {
            /* mov %reg, sym(%rip) */
            int rw = (strcmp(mn, "movl") == 0) ? 4 : 8;
            if (strcmp(mn, "movb") == 0) rw = 1;
            else if (strcmp(mn, "movw") == 0) rw = 2;
            rex(b, rw == 8, r1, 0, 0); eb(b, rw == 1 ? 0x88 : 0x89); modrm(b, 0, r1, 5);
            size_t po = b->len; e32(b, 0);
            if (!queue_rip_rel(fixes, nfix, po, sym2)) { free(sym1); free(mn); return 0; }
            sym2 = NULL;
            handled = 1;
        } else if ((k1 == 6 && k2 == 0) || (k1 == 0 && k2 == 6)) {
            /* mov %crN, %r64  (0F 20 /r)  or  mov %r64, %crN (0F 22 /r) */
            int cr, gp, load;
            if (k1 == 6) { cr = r1; gp = r2; load = 1; }
            else { cr = r2; gp = r1; load = 0; }
            rex(b, 0, cr, 0, gp); /* REX.R/B for high regs; W not required */
            /* Intel SDM: MOV from/to CR uses REX.W optional; encode 0F 20/22 */
            eb(b, 0x0F); eb(b, load ? 0x20 : 0x22);
            modrm(b, 3, cr, gp);
            handled = 1;
        } else if (k1 == 8 && k2 == 0) {
            /* mov %gs:disp / %fs:disp, %reg */
            emit_mov_seg_abs_load(b, r2, r1, imm1, w);
            handled = 1;
        } else if (k1 == 0 && k2 == 8) {
            /* mov %reg, %gs:disp / %fs:disp */
            emit_mov_seg_abs_store(b, r1, r2, imm2, w);
            handled = 1;
        } else if (k1 == 0 && k2 == 7) {
            /* mov %r16/%r32/%r64, %sreg — 8E /r */
            eb(b, 0x8E); modrm(b, 3, r2, r1);
            handled = 1;
        } else if (k1 == 7 && k2 == 0) {
            /* mov %sreg, %reg — 8C /r */
            eb(b, 0x8C); modrm(b, 3, r1, r2);
            handled = 1;
        }
    } else if (strcmp(mn, "out") == 0 || strcmp(mn, "outb") == 0 ||
               strcmp(mn, "outw") == 0 || strcmp(mn, "outl") == 0) {
        /* out src, port — AT&T: out %al, %dx  or out %al, $imm */
        int w = 1;
        if (strcmp(mn, "outw") == 0) w = 2;
        else if (strcmp(mn, "outl") == 0) w = 4;
        else if (k1 == 0) w = w1;
        if (k2 == 0 && r2 == R_RDX) {
            if (w == 1) eb(b, 0xEE);
            else if (w == 2) { eb(b, 0x66); eb(b, 0xEF); }
            else eb(b, 0xEF);
            handled = 1;
        } else if (k2 == 1) {
            if (w == 1) { eb(b, 0xE6); eb(b, (uint8_t)imm2); }
            else if (w == 2) { eb(b, 0x66); eb(b, 0xE7); eb(b, (uint8_t)imm2); }
            else { eb(b, 0xE7); eb(b, (uint8_t)imm2); }
            handled = 1;
        }
    } else if (strcmp(mn, "in") == 0 || strcmp(mn, "inb") == 0 ||
               strcmp(mn, "inw") == 0 || strcmp(mn, "inl") == 0) {
        /* in port, dst — AT&T: in %dx, %al */
        int w = 1;
        if (strcmp(mn, "inw") == 0) w = 2;
        else if (strcmp(mn, "inl") == 0) w = 4;
        else if (k2 == 0) w = w2;
        if (k1 == 0 && r1 == R_RDX) {
            if (w == 1) eb(b, 0xEC);
            else if (w == 2) { eb(b, 0x66); eb(b, 0xED); }
            else eb(b, 0xED);
            handled = 1;
        } else if (k1 == 1) {
            if (w == 1) { eb(b, 0xE4); eb(b, (uint8_t)imm1); }
            else if (w == 2) { eb(b, 0x66); eb(b, 0xE5); eb(b, (uint8_t)imm1); }
            else { eb(b, 0xE5); eb(b, (uint8_t)imm1); }
            handled = 1;
        }
    } else if (strcmp(mn, "xchg") == 0) {
        /* xchg src, dst */
        if (k1 == 0 && k2 == 2) {
            int w = w1;
            if (w == 4) { rex(b, 0, r1, 0, r2); eb(b, 0x87); }
            else { rex(b, 1, r1, 0, r2); eb(b, 0x87); }
            if ((r2 & 7) == 4) { modrm(b, 0, r1, 4); eb(b, 0x24); }
            else if ((r2 & 7) == 5) { modrm(b, 1, r1, 5); eb(b, 0); }
            else modrm(b, 0, r1, r2);
            handled = 1;
        } else if (k1 == 2 && k2 == 0) {
            int w = w2;
            if (w == 4) { rex(b, 0, r2, 0, r1); eb(b, 0x87); }
            else { rex(b, 1, r2, 0, r1); eb(b, 0x87); }
            if ((r1 & 7) == 4) { modrm(b, 0, r2, 4); eb(b, 0x24); }
            else if ((r1 & 7) == 5) { modrm(b, 1, r2, 5); eb(b, 0); }
            else modrm(b, 0, r2, r1);
            handled = 1;
        } else if (k1 == 0 && k2 == 0) {
            rex(b, 1, r1, 0, r2); eb(b, 0x87); modrm(b, 3, r1, r2);
            handled = 1;
        }
    } else if (strcmp(mn, "test") == 0) {
        if (k1 == 0 && k2 == 0) {
            int w = (w1 < w2) ? w1 : w2;
            if (w == 4) { rex(b, 0, r1, 0, r2); eb(b, 0x85); }
            else { rex(b, 1, r1, 0, r2); eb(b, 0x85); }
            modrm(b, 3, r1, r2);
            handled = 1;
        }
    } else if (strcmp(mn, "xor") == 0 || strcmp(mn, "xorl") == 0 ||
               strcmp(mn, "xorq") == 0) {
        if (k1 == 0 && k2 == 0) {
            int w = 8;
            if (strcmp(mn, "xorl") == 0) w = 4;
            else if (strcmp(mn, "xorq") == 0) w = 8;
            else w = (w1 < w2) ? w1 : w2;
            if (w <= 4) { rex(b, 0, r1, 0, r2); eb(b, 0x31); }
            else { rex(b, 1, r1, 0, r2); eb(b, 0x31); }
            modrm(b, 3, r1, r2);
            handled = 1;
        }
    } else if (strcmp(mn, "and") == 0 || strcmp(mn, "or") == 0 ||
               strcmp(mn, "add") == 0 || strcmp(mn, "sub") == 0 ||
               strcmp(mn, "cmp") == 0) {
        uint8_t op = 0x01; /* add */
        if (strcmp(mn, "or") == 0) op = 0x09;
        else if (strcmp(mn, "and") == 0) op = 0x21;
        else if (strcmp(mn, "sub") == 0) op = 0x29;
        else if (strcmp(mn, "cmp") == 0) op = 0x39;
        if (k1 == 0 && k2 == 0) {
            rex(b, 1, r1, 0, r2); eb(b, op); modrm(b, 3, r1, r2);
            handled = 1;
        } else if (k1 == 1 && k2 == 0) {
            int ext = 0;
            if (strcmp(mn, "or") == 0) ext = 1;
            else if (strcmp(mn, "and") == 0) ext = 4;
            else if (strcmp(mn, "sub") == 0) ext = 5;
            else if (strcmp(mn, "cmp") == 0) ext = 7;
            rex(b, 1, 0, 0, r2);
            if (imm1 == (int64_t)(int8_t)imm1) {
                eb(b, 0x83); modrm(b, 3, ext, r2); eb(b, (uint8_t)imm1);
            } else {
                eb(b, 0x81); modrm(b, 3, ext, r2); e32(b, (int32_t)imm1);
            }
            handled = 1;
        }
    } else if (strcmp(mn, "lea") == 0) {
        if (k1 == 2 && k2 == 0) {
            rex(b, 1, r2, 0, r1); eb(b, 0x8D);
            modrm_mem(b, r2, r1, imm1);
            handled = 1;
        } else if (k1 == 4 && k2 == 0 && sym1) {
            /* lea sym(%rip), %reg — resolve local labels; else PC32 reloc */
            rex(b, 1, r2, 0, 0); eb(b, 0x8D); modrm(b, 0, r2, 5);
            size_t po = b->len; e32(b, 0);
            if (!queue_rip_rel(fixes, nfix, po, sym1)) { free(sym2); free(mn); return 0; }
            sym1 = NULL;
            handled = 1;
        }
    } else if (strcmp(mn, "lgdt") == 0 || strcmp(mn, "lidt") == 0) {
        /* lgdt/lidt m16&64 — AT&T: lgdt (%rdi) */
        int ext = (strcmp(mn, "lgdt") == 0) ? 2 : 3;
        if (k1 == 2) {
            rex(b, 0, 0, 0, r1); eb(b, 0x0F); eb(b, 0x01);
            if ((r1 & 7) == 4) { modrm(b, 0, ext, 4); eb(b, 0x24); }
            else if ((r1 & 7) == 5) { modrm(b, 1, ext, 5); eb(b, 0); }
            else modrm(b, 0, ext, r1);
            handled = 1;
        }
    } else if (strcmp(mn, "ltr") == 0) {
        if (k1 == 0) {
            /* ltr r16 */
            eb(b, 0x0F); eb(b, 0x00); modrm(b, 3, 3, r1);
            handled = 1;
        } else if (k1 == 2) {
            rex(b, 0, 0, 0, r1); eb(b, 0x0F); eb(b, 0x00);
            if ((r1 & 7) == 4) { modrm(b, 0, 3, 4); eb(b, 0x24); }
            else modrm(b, 0, 3, r1);
            handled = 1;
        }
    } else if (strcmp(mn, "invlpg") == 0) {
        if (k1 == 2) {
            rex(b, 0, 0, 0, r1); eb(b, 0x0F); eb(b, 0x01);
            if ((r1 & 7) == 4) { modrm(b, 0, 7, 4); eb(b, 0x24); }
            else modrm(b, 0, 7, r1);
            handled = 1;
        }
    } else if (strcmp(mn, "movdqa") == 0) {
        /* AT&T movdqa %xmmN, disp(%reg)  -> 66 0F 7F /r (store)
         *       movdqa disp(%reg), %xmmN -> 66 0F 6F /r (load)
         *       movdqa %xmmA, %xmmB      -> 66 0F 6F /r (xmm<-xmm) */
        if (k1 == 5 && k2 == 2) {
            eb(b, 0x66);
            rex(b, 0, r1, 0, r2);
            eb(b, 0x0F); eb(b, 0x7F);
            modrm_mem(b, r1, r2, imm2);
            handled = 1;
        } else if (k1 == 2 && k2 == 5) {
            eb(b, 0x66);
            rex(b, 0, r2, 0, r1);
            eb(b, 0x0F); eb(b, 0x6F);
            modrm_mem(b, r2, r1, imm1);
            handled = 1;
        } else if (k1 == 5 && k2 == 5) {
            eb(b, 0x66);
            rex(b, 0, r2, 0, r1);
            eb(b, 0x0F); eb(b, 0x6F);
            modrm(b, 3, r2, r1);
            handled = 1;
        }
    }

    free(sym1); free(sym2); free(mn);
    /* Unknown two-op mnemonic: silently skip (compile corpus). */
    (void)handled;
    return 1;
}

int iasm_emit_x86(Buffer *text, EmitModule *out,
                  const char *template,
                  const IAsmOperand *ops, int nops,
                  const char *file, int line, int col) {
    (void)file; (void)line; (void)col;
    if (!template || !*template) return 0;
    char *expanded = expand_template(template, ops, nops);
    if (!expanded) return -1;

    LocLab labs[MAX_LOCLAB];
    LocFix fixes[MAX_LOCFIX];
    int nlab = 0, nfix = 0;
    memset(labs, 0, sizeof labs);
    memset(fixes, 0, sizeof fixes);

    /* First pass: walk lines. Labels defined as we go; forward refs in fixes. */
    char *buf = expanded;
    while (*buf) {
        char *eol = buf;
        while (*eol && *eol != '\n' && *eol != ';') eol++;
        char save = *eol;
        *eol = 0;
        if (!emit_line(text, out, buf, labs, &nlab, fixes, &nfix)) {
            /* keep going — unsupported line */
        }
        *eol = save;
        if (save == 0) break;
        buf = eol + 1;
    }

    /* Resolve local forward refs; external → reloc */
    for (int i = 0; i < nfix; i++) {
        int li = lookup_lab(labs, nlab, fixes[i].name);
        if (li >= 0 && labs[li].defined) {
            int32_t rel = (int32_t)((int64_t)labs[li].off -
                                    (int64_t)(fixes[i].patch_off + 4));
            uint8_t *p = (uint8_t *)text->data + fixes[i].patch_off;
            p[0] = (uint8_t)rel;
            p[1] = (uint8_t)(rel >> 8);
            p[2] = (uint8_t)(rel >> 16);
            p[3] = (uint8_t)(rel >> 24);
        } else if (out) {
            int sym = emit_module_add_undefined(out, fixes[i].name);
            uint32_t rty = (fixes[i].is_call == 1) ? R_X86_64_PLT32 : R_X86_64_PC32;
            emit_module_add_reloc(out, fixes[i].patch_off, rty, sym, -4);
        }
        free(fixes[i].name);
    }
    for (int i = 0; i < nlab; i++) free(labs[i].name);
    free(expanded);
    return 0;
}


/* Assemble a multi-line AT&T body (standalone .S / file asm).  Labels listed
 * in `globals` become STB_GLOBAL STT_FUNC at their text offsets; `externs`
 * become SHN_UNDEF.  Returns 0 on success. */
int iasm_assemble_body(EmitModule *out, const char *body,
                       const char **globals, int nglobals,
                       const char **externs, int nexterns) {
    if (!out || !body) return -1;
    size_t text0 = out->text.len;

    LocLab labs[MAX_LOCLAB];
    LocFix fixes[MAX_LOCFIX];
    int nlab = 0, nfix = 0;
    memset(labs, 0, sizeof labs);
    memset(fixes, 0, sizeof fixes);

    size_t blen = strlen(body);
    char *copy = malloc(blen + 1);
    if (!copy) return -1;
    memcpy(copy, body, blen + 1);
    if (!copy) return -1;
    char *buf = copy;
    while (*buf) {
        char *eol = buf;
        while (*eol && *eol != '\n') eol++;
        char save = *eol;
        *eol = 0;
        /* Allow ';' comments like gas */
        char *sc = strchr(buf, '#');
        if (sc) *sc = 0;
        (void)emit_line(&out->text, out, buf, labs, &nlab, fixes, &nfix);
        *eol = save;
        if (save == 0) break;
        buf = eol + 1;
    }

    for (int i = 0; i < nfix; i++) {
        int li = lookup_lab(labs, nlab, fixes[i].name);
        if (li >= 0 && labs[li].defined) {
            int32_t rel = (int32_t)((int64_t)labs[li].off -
                                    (int64_t)(fixes[i].patch_off + 4));
            uint8_t *p = (uint8_t *)out->text.data + fixes[i].patch_off;
            p[0] = (uint8_t)rel;
            p[1] = (uint8_t)(rel >> 8);
            p[2] = (uint8_t)(rel >> 16);
            p[3] = (uint8_t)(rel >> 24);
        } else {
            int sym = emit_module_add_undefined(out, fixes[i].name);
            uint32_t rty = (fixes[i].is_call == 1) ? R_X86_64_PLT32 : R_X86_64_PC32;
            emit_module_add_reloc(out, fixes[i].patch_off, rty, sym, -4);
        }
        free(fixes[i].name);
    }

    for (int g = 0; g < nglobals; g++) {
        const char *gn = globals[g];
        if (!gn || !gn[0]) continue;
        int li = lookup_lab(labs, nlab, gn);
        if (li < 0 || !labs[li].defined) continue;
        size_t off = labs[li].off;
        /* size: until next defined label or end */
        size_t end = out->text.len;
        for (int j = 0; j < nlab; j++) {
            if (!labs[j].defined) continue;
            if (labs[j].off > off && labs[j].off < end)
                end = labs[j].off;
        }
        size_t sz = end >= off ? end - off : 0;
        emit_module_add_symbol(out, gn, /*STB_GLOBAL*/1, /*STT_FUNC*/2,
                               SECT_TEXT, off - text0, sz);
    }
    for (int e = 0; e < nexterns; e++) {
        if (externs[e] && externs[e][0])
            emit_module_add_undefined(out, externs[e]);
    }
    for (int i = 0; i < nlab; i++) free(labs[i].name);
    free(copy);
    (void)text0;
    return 0;
}
