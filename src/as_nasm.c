/* fakecc NASM-syntax subset: real-mode / mixed-bits flat bin + ELF64.
 * Enough for fakeos arch/x86_64/{ap_tramp,boot}.asm.
 */
#include "fakecc/as_nasm.h"
#include "fakecc/common.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SECTS 32
#define MAX_SYMS 1024
#define MAX_RELOCS 1024
#define MAX_FIXUPS 2048

#define R64_64 1
#define R64_PC32 2
#define R64_32 10
#define R64_32S 11

typedef struct {
    char *name;
    Buffer bytes;
    size_t bss_size;
    int nobits, alloc, write, exec, align, shndx;
} Sect;

typedef struct {
    char *name;
    int sect;
    size_t off;
    int64_t absval;
    char *equ_expr; /* owned; kind==1 deferred expression text */
    int kind; /* 0 label 1 equ 2 extern */
    int is_global, defined;
} Sym;

typedef struct {
    int sect;
    size_t offset;
    int sym;
    int32_t addend;
    int type;
} Reloc;

typedef struct {
    int sect;
    size_t offset;
    int size;
    int64_t add;
    int sym;
    int pc_rel;
    int reloc_type;
    int far_seg; /* >=0 write u16 seg after value */
} Fixup;

typedef struct {
    NasmFormat fmt;
    const char *filename;
    int bits, default_rel, failed, lineno, pass;
    uint64_t org;
    int has_org;
    Sect sects[MAX_SECTS];
    int nsects, cur_sect;
    Sym syms[MAX_SYMS];
    int nsyms, last_non_local;
    Reloc relocs[MAX_RELOCS];
    int nrelocs;
    Fixup fixups[MAX_FIXUPS];
    int nfixups;
} As;

static void as_err(As *as, const char *msg) {
    fprintf(stderr, "%s:%d: %s\n",
            as->filename ? as->filename : "<asm>", as->lineno, msg);
    as->failed = 1;
}

static void eb(Buffer *b, uint8_t v) { buffer_append(b, (const char *)&v, 1); }

static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\r') p++;
    return p;
}
static int is_id_start(int c) { return isalpha(c) || c == '_' || c == '.' || c == '$'; }
static int is_id_cont(int c) { return isalnum(c) || c == '_' || c == '.' || c == '$'; }

static char *dup_range(const char *s, const char *e) {
    size_t n = (size_t)(e - s);
    char *o = (char *)xmalloc(n + 1);
    memcpy(o, s, n);
    o[n] = 0;
    return o;
}
static char *dup_ident(const char **pp) {
    const char *p = *pp;
    if (!is_id_start((unsigned char)*p)) return NULL;
    const char *s = p++;
    while (is_id_cont((unsigned char)*p)) p++;
    *pp = p;
    return dup_range(s, p);
}
static void str_tolower(char *s) {
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

static int find_sym(As *as, const char *n) {
    for (int i = 0; i < as->nsyms; i++)
        if (strcmp(as->syms[i].name, n) == 0) return i;
    return -1;
}
static int add_sym(As *as, const char *n) {
    int i = find_sym(as, n);
    if (i >= 0) return i;
    if (as->nsyms >= MAX_SYMS) { as_err(as, "too many symbols"); return -1; }
    i = as->nsyms++;
    memset(&as->syms[i], 0, sizeof(as->syms[i]));
    as->syms[i].name = xstrdup(n);
    as->syms[i].sect = -1;
    return i;
}
static int find_sect(As *as, const char *n) {
    for (int i = 0; i < as->nsects; i++)
        if (strcmp(as->sects[i].name, n) == 0) return i;
    return -1;
}
static int add_sect(As *as, const char *n) {
    int i = find_sect(as, n);
    if (i >= 0) return i;
    if (as->nsects >= MAX_SECTS) { as_err(as, "too many sections"); return -1; }
    i = as->nsects++;
    memset(&as->sects[i], 0, sizeof(as->sects[i]));
    as->sects[i].name = xstrdup(n);
    buffer_init(&as->sects[i].bytes);
    as->sects[i].alloc = 1;
    as->sects[i].align = 1;
    if (strstr(n, "bss")) { as->sects[i].nobits = 1; as->sects[i].write = 1; }
    if (strstr(n, ".data")) as->sects[i].write = 1;
    if (strcmp(n, ".text") == 0 || strstr(n, ".text")) {
        as->sects[i].exec = 1;
        if (strcmp(n, ".text") == 0) as->sects[i].align = 16;
    }
    if (strstr(n, "GNU-stack")) {
        as->sects[i].alloc = 0; as->sects[i].write = 0; as->sects[i].exec = 0;
    }
    return i;
}
static Sect *cur(As *as) {
    if (as->cur_sect < 0) as->cur_sect = add_sect(as, ".text");
    return &as->sects[as->cur_sect];
}
static size_t cur_off(As *as) {
    Sect *s = cur(as);
    return s->nobits ? s->bss_size : s->bytes.len;
}
static void cur_eb(As *as, uint8_t v) {
    Sect *s = cur(as);
    if (s->nobits) { as_err(as, "emit into nobits"); return; }
    eb(&s->bytes, v);
}
static void do_align(As *as, int a, int use_nop) {
    if (a <= 1) return;
    Sect *s = cur(as);
    if (s->align < a) s->align = a;
    size_t off = cur_off(as);
    size_t pad = (size_t)((a - (int)(off % (size_t)a)) % a);
    if (s->nobits) { s->bss_size += pad; return; }
    uint8_t fill = use_nop ? 0x90 : 0x00;
    for (size_t i = 0; i < pad; i++) cur_eb(as, fill);
}

static void add_fixup(As *as, size_t off, int size, int64_t add, int sym,
                       int pc_rel, int rtype, int far_seg) {
    if (as->pass != 2) return;
    if (as->nfixups >= MAX_FIXUPS) { as_err(as, "too many fixups"); return; }
    Fixup *f = &as->fixups[as->nfixups++];
    f->sect = as->cur_sect;
    f->offset = off;
    f->size = size;
    f->add = add;
    f->sym = sym;
    f->pc_rel = pc_rel;
    f->reloc_type = rtype;
    f->far_seg = far_seg;
}
static void add_reloc(As *as, int sect, size_t offset, int sym, int32_t addend, int type) {
    if (as->nrelocs >= MAX_RELOCS) { as_err(as, "too many relocs"); return; }
    Reloc *r = &as->relocs[as->nrelocs++];
    r->sect = sect; r->offset = offset; r->sym = sym; r->addend = addend; r->type = type;
}

/* Forward decls for expr — implemented below via including continued file */

typedef struct { As *as; const char *p; int allow_undef; } ExprCtx;
static int parse_expr(ExprCtx *c, int64_t *out, int *out_sym);

static void qual_label(As *as, const char *id, char *buf, size_t n) {
    if (id[0] == '.' && as->last_non_local >= 0)
        snprintf(buf, n, "%s%s", as->syms[as->last_non_local].name, id);
    else
        snprintf(buf, n, "%s", id);
}

static int parse_primary(ExprCtx *c, int64_t *out, int *out_sym) {
    c->p = skip_ws(c->p); *out_sym = -1;
    if (*c->p == '(') {
        c->p++;
        if (!parse_expr(c, out, out_sym)) return 0;
        c->p = skip_ws(c->p);
        if (*c->p != ')') { as_err(c->as, "')'"); return 0; }
        c->p++; return 1;
    }
    if (c->p[0]=='$' && c->p[1]=='$') {
        c->p += 2;
        *out = (c->as->fmt==NASM_FMT_BIN && c->as->has_org) ? (int64_t)c->as->org : 0;
        return 1;
    }
    if (*c->p=='$' && !is_id_cont((unsigned char)c->p[1])) {
        c->p++;
        *out = (int64_t)cur_off(c->as);
        if (c->as->fmt==NASM_FMT_BIN && c->as->has_org) *out += (int64_t)c->as->org;
        return 1;
    }
    if (isdigit((unsigned char)*c->p)) {
        char *end=NULL;
        *out = (c->p[0]=='0' && (c->p[1]=='x'||c->p[1]=='X'))
            ? (int64_t)strtoull(c->p,&end,16) : (int64_t)strtoll(c->p,&end,0);
        if (end==c->p) return 0;
        c->p=end; return 1;
    }
    if (is_id_start((unsigned char)*c->p)) {
        char *id=dup_ident(&c->p); char nm[256];
        qual_label(c->as, id, nm, sizeof nm);
        int si=find_sym(c->as, nm); if (si<0) si=find_sym(c->as, id);
        free(id);
        if (si<0) { if (c->allow_undef){*out=0;return 1;} as_err(c->as,"undef"); return 0; }
        Sym *sy=&c->as->syms[si];
        if (sy->kind==1) { *out=sy->absval; return 1; }
        if (sy->kind==2) { *out=0; *out_sym=si; return 1; }
        if (sy->defined && c->as->fmt==NASM_FMT_BIN) {
            *out=(int64_t)sy->off+(c->as->has_org?(int64_t)c->as->org:0); return 1;
        }
        if (sy->defined || c->allow_undef) { *out=(int64_t)sy->off; *out_sym=si; return 1; }
        as_err(c->as,"undef"); return 0;
    }
    as_err(c->as,"expr"); return 0;
}
static int parse_unary(ExprCtx *c, int64_t *out, int *out_sym) {
    c->p=skip_ws(c->p);
    if (*c->p=='-'||*c->p=='~'||*c->p=='+') {
        char op=*c->p++; int64_t v; int s;
        if (!parse_unary(c,&v,&s)) return 0;
        if (s>=0) { as_err(c->as,"unary reloc"); return 0; }
        *out = op=='-'?-v:op=='~'?~v:v; *out_sym=-1; return 1;
    }
    return parse_primary(c,out,out_sym);
}
static int parse_mul(ExprCtx *c, int64_t *out, int *out_sym) {
    if (!parse_unary(c,out,out_sym)) return 0;
    for(;;){
        c->p=skip_ws(c->p); int sh=0; char op=0;
        if(*c->p=='*'||*c->p=='/') op=*c->p++;
        else if(c->p[0]=='<'&&c->p[1]=='<'){sh=1;c->p+=2;}
        else if(c->p[0]=='>'&&c->p[1]=='>'){sh=2;c->p+=2;}
        else break;
        int64_t r; int rs; if(!parse_unary(c,&r,&rs)) return 0;
        if(*out_sym>=0||rs>=0){as_err(c->as,"reloc */"); return 0;}
        if(sh==1)*out<<=(int)(r&63); else if(sh==2)*out>>=(int)(r&63);
        else if(op=='*')*out*=r; else *out=r?(*out/r):0;
    }
    return 1;
}
static int parse_add(ExprCtx *c, int64_t *out, int *out_sym) {
    if (!parse_mul(c,out,out_sym)) return 0;
    for(;;){
        c->p=skip_ws(c->p); if(*c->p!='+'&&*c->p!='-') break;
        char op=*c->p++; int64_t r; int rs; if(!parse_mul(c,&r,&rs)) return 0;
        if(op=='+'){ if(*out_sym>=0&&rs>=0){as_err(c->as,"+rel");return 0;} *out+=r; if(rs>=0)*out_sym=rs; }
        else {
            if(rs>=0&&*out_sym>=0){
                Sym *a=&c->as->syms[*out_sym],*b=&c->as->syms[rs];
                if(a->defined&&b->defined&&a->sect>=0&&a->sect==b->sect){
                    *out=(*out-(int64_t)a->off)+(int64_t)a->off - ((r-(int64_t)b->off)+(int64_t)b->off);
                    *out_sym=-1;
                } else { as_err(c->as,"symdiff"); return 0; }
            } else if(rs>=0){ as_err(c->as,"-rel"); return 0; }
            else *out-=r;
        }
    }
    return 1;
}
static int parse_band(ExprCtx *c,int64_t *out,int *out_sym){
    if(!parse_add(c,out,out_sym))return 0;
    for(;;){c->p=skip_ws(c->p);if(*c->p!='&')break;c->p++;
        int64_t r;int rs;if(!parse_add(c,&r,&rs))return 0;
        if(*out_sym>=0||rs>=0)return 0;
        *out&=r;} return 1;
}
static int parse_bxor(ExprCtx *c,int64_t *out,int *out_sym){
    if(!parse_band(c,out,out_sym))return 0;
    for(;;){c->p=skip_ws(c->p);if(*c->p!='^')break;c->p++;
        int64_t r;int rs;if(!parse_band(c,&r,&rs))return 0;
        if(*out_sym>=0||rs>=0)return 0;
        *out^=r;} return 1;
}
static int parse_expr(ExprCtx *c,int64_t *out,int *out_sym){
    if(!parse_bxor(c,out,out_sym))return 0;
    for(;;){c->p=skip_ws(c->p);if(*c->p!='|')break;c->p++;
        int64_t r;int rs;if(!parse_bxor(c,&r,&rs))return 0;
        if(*out_sym>=0||rs>=0)return 0;
        *out|=r;} return 1;
}
static int eval_ex(As *as,const char **pp,int64_t *out,int *out_sym,int au){
    ExprCtx c={as,*pp,au}; if(!parse_expr(&c,out,out_sym))return 0; *pp=c.p; return 1;
}

typedef enum { OK_NONE=0, OK_REG, OK_IMM, OK_MEM, OK_FAR, OK_CR } OpKind;
typedef struct {
    OpKind kind;
    int reg, width, is_seg;
    int64_t imm; int imm_sym;
    int has_base, base, has_index, index, scale;
    int64_t disp; int disp_sym; int abs_mem;
    int far_seg; int64_t far_off; int far_off_sym;
} Op;

static int gp_reg(const char *n, int *reg, int *width) {
    static const struct { const char *n; int r,w; } T[] = {
        {"al",0,1},{"cl",1,1},{"dl",2,1},{"bl",3,1},{"ah",4,1},{"ch",5,1},{"dh",6,1},{"bh",7,1},
        {"ax",0,2},{"cx",1,2},{"dx",2,2},{"bx",3,2},{"sp",4,2},{"bp",5,2},{"si",6,2},{"di",7,2},
        {"eax",0,4},{"ecx",1,4},{"edx",2,4},{"ebx",3,4},{"esp",4,4},{"ebp",5,4},{"esi",6,4},{"edi",7,4},
        {"rax",0,8},{"rcx",1,8},{"rdx",2,8},{"rbx",3,8},{"rsp",4,8},{"rbp",5,8},{"rsi",6,8},{"rdi",7,8},
        {"r8",8,8},{"r9",9,8},{"r10",10,8},{"r11",11,8},{"r12",12,8},{"r13",13,8},{"r14",14,8},{"r15",15,8},
        {"r8d",8,4},{"r9d",9,4},{"r10d",10,4},{"r11d",11,4},{"r12d",12,4},{"r13d",13,4},{"r14d",14,4},{"r15d",15,4},
        {"spl",4,1},{"bpl",5,1},{"sil",6,1},{"dil",7,1},
    };
    for (size_t i=0;i<sizeof T/sizeof T[0];i++)
        if (!strcmp(n,T[i].n)) { *reg=T[i].r; *width=T[i].w; return 1; }
    return 0;
}
static int seg_reg(const char *n, int *s) {
    static const char *S[]={"es","cs","ss","ds","fs","gs"};
    for (int i=0;i<6;i++) if (!strcmp(n,S[i])) { *s=i; return 1; }
    return 0;
}
static int cr_reg(const char *n, int *c) {
    if (n[0]=='c'&&n[1]=='r'&&n[2]>='0'&&n[2]<='7'&&!n[3]) { *c=n[2]-'0'; return 1; }
    return 0;
}

static void emit_rex(As *as, int w, int r, int x, int b) {
    int need = w || (r&8) || (x&8) || (b&8);
    if (as->bits != 64 && !need) return;
    if (!need) return;
    cur_eb(as, (uint8_t)(0x40 | (w?8:0) | ((r&8)?4:0) | ((x&8)?2:0) | ((b&8)?1:0)));
}
static void maybe_opsz(As *as, int width) {
    if (as->bits==16 && width==4) cur_eb(as,0x66);
    else if ((as->bits==32 || as->bits==64) && width==2) cur_eb(as,0x66);
}


static void rex_for_rm(As *as, int rex_w, int regfld, Op *rm) {
    if (as->bits != 64 && !rex_w) return;
    int r = regfld, x = 0, b = 0;
    if (rm->kind == OK_REG && !rm->is_seg) b = rm->reg;
    else if (rm->kind == OK_MEM) {
        if (rm->has_base) b = rm->base;
        if (rm->has_index) x = rm->index;
    }
    emit_rex(as, rex_w, r, x, b);
}
static int emit_rm(As *as, int regfld, Op *rm, int rex_w) {
    if (rm->kind == OK_REG && !rm->is_seg) {
        (void)rex_w;
        cur_eb(as, (uint8_t)(0xC0 | ((regfld&7)<<3) | (rm->reg&7)));
        return 1;
    }
    if (rm->kind != OK_MEM) { as_err(as, "r/m"); return 0; }
    int64_t disp=rm->disp; int ds=rm->disp_sym; int rf=regfld&7;

    if (rm->abs_mem || (!rm->has_base && !rm->has_index)) {
        if (as->bits==16) {
            cur_eb(as, (uint8_t)((rf<<3)|6));
            size_t off=cur_off(as); cur_eb(as,0);cur_eb(as,0);
            add_fixup(as,off,2,disp,ds,0,0,-1);
            return 1;
        }
        if (as->bits==64) {
            (void)rex_w;
            cur_eb(as, (uint8_t)((rf<<3)|4));
            cur_eb(as, 0x25);
            size_t off=cur_off(as); for(int i=0;i<4;i++) cur_eb(as,0);
            add_fixup(as,off,4,disp,ds,0, as->fmt==NASM_FMT_ELF64?R64_32S:0, -1);
            return 1;
        }
        cur_eb(as, (uint8_t)((rf<<3)|5));
        size_t off=cur_off(as); for(int i=0;i<4;i++) cur_eb(as,0);
        add_fixup(as,off,4,disp,ds,0, as->fmt==NASM_FMT_ELF64?R64_32:0, -1);
        return 1;
    }

    int base = rm->has_base ? rm->base : 5;
    int index = rm->has_index ? rm->index : 4;
    int scale = rm->scale ? rm->scale : 1;
    int ss = scale==2?1:scale==4?2:scale==8?3:0;
    int use_sib = rm->has_index || (rm->has_base && (base&7)==4) || !rm->has_base;
    int has_disp = (disp!=0)||(ds>=0);
    if (rm->has_base && (base&7)==5 && !has_disp) has_disp=1;
    int mod = !has_disp ? 0 : (ds<0 && disp==(int64_t)(int8_t)disp) ? 1 : 2;

    (void)rex_w;

    if (use_sib) {
        if (!rm->has_base) mod = 0; /* [disp32 + index*scale] requires mod=00 base=5 */
        cur_eb(as, (uint8_t)((mod<<6)|(rf<<3)|4));
        cur_eb(as, (uint8_t)((ss<<6)|((index&7)<<3)|(rm->has_base?(base&7):5)));
        if (!rm->has_base) {
            size_t off=cur_off(as); for(int i=0;i<4;i++) cur_eb(as,0);
            add_fixup(as,off,4,disp,ds,0,as->fmt==NASM_FMT_ELF64?R64_32:0,-1);
            return 1;
        }
    } else {
        cur_eb(as, (uint8_t)((mod<<6)|(rf<<3)|(base&7)));
    }
    if (mod==1) {
        size_t off=cur_off(as); cur_eb(as,0);
        add_fixup(as,off,1,disp,ds,0,0,-1);
    } else if (mod==2) {
        size_t off=cur_off(as); for(int i=0;i<4;i++) cur_eb(as,0);
        add_fixup(as,off,4,disp,ds,0, (as->fmt==NASM_FMT_ELF64&&ds>=0)?R64_32:0, -1);
    }
    return 1;
}

static int parse_mem(As *as, const char **pp, Op *o) {
    const char *p = skip_ws(*pp);
    if (*p != '[') return 0;
    p++;
    memset(o, 0, sizeof(*o));
    o->kind = OK_MEM;
    o->scale = 1;
    o->disp_sym = o->imm_sym = o->far_off_sym = -1;
    p = skip_ws(p);

    /* Grammar (subset):
     *   [ reg ]
     *   [ reg + disp ]
     *   [ reg + reg * scale (+ disp) ]
     *   [ symbol ]
     *   [ symbol + reg * scale (+ disp) ]
     *   [ expr ]  (absolute)
     */
    char *id = dup_ident(&p);
    if (id) {
        str_tolower(id);
        int r, w;
        if (gp_reg(id, &r, &w)) {
            o->has_base = 1;
            o->base = r;
            free(id);
            p = skip_ws(p);
            for (;;) {
                p = skip_ws(p);
                if (*p != '+' && *p != '-') break;
                int sign = (*p == '-') ? -1 : 1;
                p++;
                p = skip_ws(p);
                const char *s2 = p;
                char *id2 = dup_ident(&p);
                if (id2) {
                    str_tolower(id2);
                    int r2, w2;
                    if (sign > 0 && gp_reg(id2, &r2, &w2)) {
                        o->has_index = 1;
                        o->index = r2;
                        free(id2);
                        p = skip_ws(p);
                        if (*p == '*') {
                            p++;
                            p = skip_ws(p);
                            char *end = NULL;
                            long sc = strtol(p, &end, 0);
                            if (end == p) return 0;
                            p = end;
                            o->scale = (int)sc;
                        }
                        continue;
                    }
                    free(id2);
                    p = s2;
                }
                int64_t d; int ds;
                if (!eval_ex(as, &p, &d, &ds, 1)) return 0;
                o->disp += sign * d;
                if (ds >= 0) o->disp_sym = ds;
            }
        } else {
            /* identifier is a symbol (displacement), optional + reg*scale.
             * Do NOT eval_ex the whole [...] — that would eat "+ecx*8". */
            char nm[256];
            qual_label(as, id, nm, sizeof nm);
            int si = find_sym(as, nm);
            if (si < 0) si = find_sym(as, id);
            if (si < 0) si = add_sym(as, nm[0]?nm:id);
            free(id);
            o->disp = as->syms[si].defined ? (int64_t)as->syms[si].off : 0;
            o->disp_sym = si;
            p = skip_ws(p);
            for (;;) {
                p = skip_ws(p);
                if (*p != '+' && *p != '-') break;
                int sign = (*p == '-') ? -1 : 1;
                p++;
                p = skip_ws(p);
                const char *s2 = p;
                char *id2 = dup_ident(&p);
                if (id2) {
                    str_tolower(id2);
                    int r2, w2;
                    if (sign > 0 && gp_reg(id2, &r2, &w2)) {
                        o->has_index = 1;
                        o->index = r2;
                        free(id2);
                        p = skip_ws(p);
                        if (*p == '*') {
                            p++;
                            p = skip_ws(p);
                            char *end = NULL;
                            long sc = strtol(p, &end, 0);
                            if (end == p) return 0;
                            p = end;
                            o->scale = (int)sc;
                        }
                        continue;
                    }
                    free(id2);
                    p = s2;
                }
                int64_t d2; int ds2;
                if (!eval_ex(as, &p, &d2, &ds2, 1)) return 0;
                o->disp += sign * d2;
            }
            if (!o->has_index) o->abs_mem = 1;
        }
    } else {
        int64_t d; int ds;
        if (!eval_ex(as, &p, &d, &ds, 1)) return 0;
        o->abs_mem = 1;
        o->disp = d;
        o->disp_sym = ds;
    }
    p = skip_ws(p);
    if (*p != ']') { as_err(as, "expected ']'"); return 0; }
    p++;
    *pp = p;
    return 1;
}

static int parse_op(As *as, const char **pp, Op *o) {
    const char *p=skip_ws(*pp);
    memset(o,0,sizeof(*o)); o->imm_sym=o->disp_sym=o->far_off_sym=-1;
    if (*p=='[') { if(!parse_mem(as,&p,o)) return 0; *pp=p; return 1; }
    if (!strncmp(p,"byte",4)||!strncmp(p,"word",4)||!strncmp(p,"dword",5)||!strncmp(p,"qword",5)) {
        int w = !strncmp(p,"byte",4)?1:!strncmp(p,"qword",5)?8:!strncmp(p,"dword",5)?4:2;
        p += (w==4||w==8)?5:4; p=skip_ws(p);
        if (*p=='[') { if(!parse_mem(as,&p,o)) return 0; o->width=w; *pp=p; return 1; }
    }
    /* far: expr:expr */
    {
        const char *s=p; int64_t a; int asy;
        if (eval_ex(as,&p,&a,&asy,1)) {
            p=skip_ws(p);
            if (*p==':') {
                p++; int64_t b; int bsy;
                if (!eval_ex(as,&p,&b,&bsy,1)) return 0;
                o->kind=OK_FAR; o->far_seg=(int)a; o->far_off=b; o->far_off_sym=bsy;
                *pp=p; return 1;
            }
            p=s;
        } else p=s;
    }
    {
        const char *s=p; char *id=dup_ident(&p);
        if (id) {
            str_tolower(id); int r,w,sg,cr;
            if (gp_reg(id,&r,&w)){o->kind=OK_REG;o->reg=r;o->width=w;free(id);*pp=p;return 1;}
            if (seg_reg(id,&sg)){o->kind=OK_REG;o->is_seg=1;o->reg=sg;o->width=2;free(id);*pp=p;return 1;}
            if (cr_reg(id,&cr)){o->kind=OK_CR;o->reg=cr;o->width=4;free(id);*pp=p;return 1;}
            free(id); p=s;
        }
    }
    {
        int64_t v; int sy;
        if (!eval_ex(as,&p,&v,&sy,1)) return 0;
        o->kind=OK_IMM; o->imm=v; o->imm_sym=sy;
        o->width = as->bits==16?2:4;
        *pp=p; return 1;
    }
}

static int emit_imm(As *as, int64_t v, int sym, int size, int rtype) {
    size_t off = cur_off(as);
    for (int i = 0; i < size; i++) cur_eb(as, 0);
    add_fixup(as, off, size, v, sym, 0, rtype, -1);
    return 1;
}

static int emit_mov(As *as, Op *dst, Op *src) {
    /* mov sreg, r16 */
    if (dst->kind==OK_REG && dst->is_seg && src->kind==OK_REG && !src->is_seg) {
        cur_eb(as, 0x8E);
        cur_eb(as, (uint8_t)(0xC0 | ((dst->reg&7)<<3) | (src->reg&7)));
        return 1;
    }
    /* mov cr, r / mov r, cr */
    if (dst->kind==OK_CR && src->kind==OK_REG) {
        cur_eb(as,0x0F); cur_eb(as,0x22);
        cur_eb(as, (uint8_t)(0xC0|((dst->reg&7)<<3)|(src->reg&7)));
        return 1;
    }
    if (src->kind==OK_CR && dst->kind==OK_REG) {
        cur_eb(as,0x0F); cur_eb(as,0x20);
        cur_eb(as, (uint8_t)(0xC0|((src->reg&7)<<3)|(dst->reg&7)));
        return 1;
    }
    /* mov reg, imm */
    if (dst->kind==OK_REG && !dst->is_seg && src->kind==OK_IMM) {
        int w=dst->width; maybe_opsz(as,w);
        if (w==8) {
            emit_rex(as,1,0,0,dst->reg);
            cur_eb(as,(uint8_t)(0xB8+(dst->reg&7)));
            return emit_imm(as,src->imm,src->imm_sym,8,
                as->fmt==NASM_FMT_ELF64&&src->imm_sym>=0?R64_64:0);
        }
        if (dst->reg>=8) emit_rex(as,0,0,0,dst->reg);
        if (w==1) { cur_eb(as,(uint8_t)(0xB0+(dst->reg&7))); return emit_imm(as,src->imm,src->imm_sym,1,0); }
        cur_eb(as,(uint8_t)(0xB8+(dst->reg&7)));
        return emit_imm(as,src->imm,src->imm_sym,w==2?2:4,
            as->fmt==NASM_FMT_ELF64&&src->imm_sym>=0?R64_32:0);
    }
    /* mov eax/ax, moffs (not 64-bit) */
    if (dst->kind==OK_REG && dst->reg==0 && !dst->is_seg && src->kind==OK_MEM &&
        src->abs_mem && as->bits!=64) {
        int w=dst->width; maybe_opsz(as,w);
        cur_eb(as, w==1?0xA0:0xA1);
        int sz=as->bits==16?2:4;
        size_t off=cur_off(as); for(int i=0;i<sz;i++) cur_eb(as,0);
        add_fixup(as,off,sz,src->disp,src->disp_sym,0,
            as->fmt==NASM_FMT_ELF64?R64_32:0,-1);
        return 1;
    }
    /* mov moffs, eax/ax */
    if (src->kind==OK_REG && src->reg==0 && !src->is_seg && dst->kind==OK_MEM &&
        dst->abs_mem && as->bits!=64) {
        int w=src->width; maybe_opsz(as,w);
        cur_eb(as, w==1?0xA2:0xA3);
        int sz=as->bits==16?2:4;
        size_t off=cur_off(as); for(int i=0;i<sz;i++) cur_eb(as,0);
        add_fixup(as,off,sz,dst->disp,dst->disp_sym,0,
            as->fmt==NASM_FMT_ELF64?R64_32:0,-1);
        return 1;
    }
    /* mov r, r/m */
    if (dst->kind==OK_REG && !dst->is_seg && (src->kind==OK_REG||src->kind==OK_MEM)) {
        int w=dst->width; maybe_opsz(as,w);
        rex_for_rm(as, w==8, dst->reg, src);
        if (w==1) cur_eb(as,0x8A); else cur_eb(as,0x8B);
        return emit_rm(as, dst->reg, src, w==8);
    }
    /* mov r/m, r */
    if (src->kind==OK_REG && !src->is_seg && dst->kind==OK_MEM) {
        int w=src->width; maybe_opsz(as,w);
        rex_for_rm(as, w==8, src->reg, dst);
        if (w==1) cur_eb(as,0x88); else cur_eb(as,0x89);
        return emit_rm(as, src->reg, dst, w==8);
    }
    /* mov r/m, imm */
    if ((dst->kind==OK_MEM||(dst->kind==OK_REG&&!dst->is_seg)) && src->kind==OK_IMM) {
        int w = dst->width ? dst->width : (as->bits==16?2:4);
        maybe_opsz(as,w);
        if (w==1){ rex_for_rm(as,0,0,dst); cur_eb(as,0xC6); if(!emit_rm(as,0,dst,0))return 0; return emit_imm(as,src->imm,src->imm_sym,1,0); }
        rex_for_rm(as,w==8,0,dst); cur_eb(as,0xC7);
        if(!emit_rm(as,0,dst,w==8)) return 0;
        return emit_imm(as,src->imm,src->imm_sym,w==2?2:4,
            as->fmt==NASM_FMT_ELF64?R64_32:0);
    }
    as_err(as,"bad mov"); return 0;
}

static int emit_alu_imm(As *as, int ext, Op *dst, Op *imm) {
    int w = dst->kind==OK_REG ? dst->width : (dst->width?dst->width:4);
    maybe_opsz(as,w);
    int rex_w = (w==8);
    int64_t v=imm->imm;
    int fits8 = (imm->imm_sym<0 && v==(int64_t)(int8_t)v && w!=1);
    /* eax shortcuts */
    if (dst->kind==OK_REG && dst->reg==0 && !fits8 && w!=1) {
        uint8_t op=0;
        if (ext==1) op=0x0D; else if (ext==4) op=0x25;
        else if (ext==6) op=0x35; else if (ext==0) op=0x05;
        if (op) {
            if (rex_w) emit_rex(as,1,0,0,0);
            cur_eb(as,op);
            return emit_imm(as,v,imm->imm_sym,w==2?2:4,0);
        }
    }
    if (w==1) {
        rex_for_rm(as,0,ext,dst); cur_eb(as,0x80); if(!emit_rm(as,ext,dst,0))return 0;
        return emit_imm(as,v,imm->imm_sym,1,0);
    }
    if (fits8) {
        rex_for_rm(as,rex_w,ext,dst); cur_eb(as,0x83); if(!emit_rm(as,ext,dst,rex_w))return 0;
        return emit_imm(as,v,-1,1,0);
    }
    rex_for_rm(as,rex_w,ext,dst); cur_eb(as,0x81); if(!emit_rm(as,ext,dst,rex_w))return 0;
    return emit_imm(as,v,imm->imm_sym,w==2?2:4,0);
}

static int emit_alu_rr(As *as, uint8_t opc32, Op *dst, Op *src) {
    /* opc32: 32-bit form for r/m,r (xor=0x31). r,r/m is opc32+2. */
    int w = dst->width ? dst->width : src->width;
    maybe_opsz(as,w);
    if (src->kind==OK_REG && (dst->kind==OK_REG||dst->kind==OK_MEM)) {
        uint8_t opc = (w==1) ? (uint8_t)(opc32-1) : opc32;
        rex_for_rm(as, w==8, src->reg, dst);
        cur_eb(as, opc);
        return emit_rm(as, src->reg, dst, w==8);
    }
    if (dst->kind==OK_REG && src->kind==OK_MEM) {
        uint8_t opc = (w==1) ? (uint8_t)(opc32+1) : (uint8_t)(opc32+2);
        rex_for_rm(as, w==8, dst->reg, src);
        cur_eb(as, opc);
        return emit_rm(as, dst->reg, src, w==8);
    }
    as_err(as,"bad alu rr"); return 0;
}

static int emit_jmp_call(As *as, const char *mn, Op *tgt) {
    int is_call = !strcmp(mn,"call");
    if (tgt->kind==OK_FAR) {
        /* EA ptr16:16 (bits16) or ptr16:32 (bits32) */
        cur_eb(as, 0xEA);
        int offsz = as->bits==16 ? 2 : 4;
        size_t off=cur_off(as);
        for(int i=0;i<offsz;i++) cur_eb(as,0);
        cur_eb(as, (uint8_t)(tgt->far_seg & 0xff));
        cur_eb(as, (uint8_t)((tgt->far_seg >> 8) & 0xff));
        add_fixup(as, off, offsz, tgt->far_off, tgt->far_off_sym, 0,
            as->fmt==NASM_FMT_ELF64 ? R64_32 : 0, -1);
        return 1;
    }
    if (tgt->kind==OK_REG) {
        rex_for_rm(as, 0, is_call?2:4, tgt);
        cur_eb(as, 0xFF);
        return emit_rm(as, is_call?2:4, tgt, 0);
    }
    if (tgt->kind==OK_IMM) {
        int use_short = 0;
        if (!is_call && tgt->imm_sym >= 0 && as->syms[tgt->imm_sym].defined &&
            as->syms[tgt->imm_sym].sect == as->cur_sect && as->syms[tgt->imm_sym].kind == 0) {
            int64_t rel = (int64_t)as->syms[tgt->imm_sym].off - ((int64_t)cur_off(as) + 2);
            if (rel == (int64_t)(int8_t)rel) use_short = 1;
        }
        if (use_short) {
            cur_eb(as, 0xEB);
            size_t off = cur_off(as); cur_eb(as, 0);
            add_fixup(as, off, 1, 0, tgt->imm_sym, 1, 0, -1);
            return 1;
        }
        cur_eb(as, is_call ? 0xE8 : 0xE9);
        size_t off = cur_off(as);
        for (int i = 0; i < 4; i++) cur_eb(as, 0);
        if (tgt->imm_sym >= 0 && as->fmt == NASM_FMT_ELF64 &&
            (!as->syms[tgt->imm_sym].defined || as->syms[tgt->imm_sym].kind == 2 ||
             as->syms[tgt->imm_sym].sect != as->cur_sect)) {
            add_fixup(as, off, 4, -4, tgt->imm_sym, 0, R64_PC32, -1);
        } else {
            add_fixup(as, off, 4, 0, tgt->imm_sym, 1, 0, -1);
        }
        return 1;
    }
    /* short jmp label — try imm with small delta in pass2 via E9 first;
       for jne use separate path */
    as_err(as,"bad jmp"); return 0;
}

static int jcc_cc(const char *mn) {
    static const struct { const char *n; int cc; } T[]={
        {"jo",0},{"jno",1},{"jb",2},{"jc",2},{"jnae",2},{"jnb",3},{"jae",3},{"jnc",3},
        {"je",4},{"jz",4},{"jne",5},{"jnz",5},{"jbe",6},{"jna",6},{"ja",7},{"jnbe",7},
        {"js",8},{"jns",9},{"jp",10},{"jpe",10},{"jnp",11},{"jpo",11},
        {"jl",12},{"jnge",12},{"jge",13},{"jnl",13},{"jle",14},{"jng",14},{"jg",15},{"jnle",15},
    };
    for (size_t i=0;i<sizeof T/sizeof T[0];i++) if(!strcmp(mn,T[i].n)) return T[i].cc;
    return -1;
}

static int emit_jcc(As *as, const char *mn, Op *tgt) {
    int cc=jcc_cc(mn); if (cc<0) return 0;
    /* short jcc */
    cur_eb(as, (uint8_t)(0x70|cc));
    size_t off=cur_off(as); cur_eb(as,0);
    add_fixup(as, off, 1, tgt->imm, tgt->imm_sym, 1, 0, -1);
    return 1;
}

static int emit_unary_mn(As *as, const char *mn) {
    if (!strcmp(mn,"cli")){cur_eb(as,0xFA);return 1;}
    if (!strcmp(mn,"sti")){cur_eb(as,0xFB);return 1;}
    if (!strcmp(mn,"hlt")){cur_eb(as,0xF4);return 1;}
    if (!strcmp(mn,"nop")){cur_eb(as,0x90);return 1;}
    if (!strcmp(mn,"cld")){cur_eb(as,0xFC);return 1;}
    if (!strcmp(mn,"std")){cur_eb(as,0xFD);return 1;}
    if (!strcmp(mn,"rdmsr")){cur_eb(as,0x0F);cur_eb(as,0x32);return 1;}
    if (!strcmp(mn,"wrmsr")){cur_eb(as,0x0F);cur_eb(as,0x30);return 1;}
    return 0;
}

static int assemble_instr(As *as, char *mn, const char *args) {
    str_tolower(mn);
    if (emit_unary_mn(as, mn)) return 1;
    Op a, b; memset(&a,0,sizeof a); memset(&b,0,sizeof b);
    const char *p=args;
    int na=0;
    if (*skip_ws(p)) {
        if (!parse_op(as,&p,&a)) return 0;
        na=1; p=skip_ws(p);
        if (*p==',') { p++; if (!parse_op(as,&p,&b)) return 0; na=2; }
    }
    if (!strcmp(mn,"mov") && na==2) return emit_mov(as,&a,&b);
    if (!strcmp(mn,"xor") && na==2) {
        if (b.kind==OK_IMM) return emit_alu_imm(as,6,&a,&b);
        return emit_alu_rr(as,0x31,&a,&b);
    }
    if (!strcmp(mn,"or") && na==2) {
        if (b.kind==OK_IMM) return emit_alu_imm(as,1,&a,&b);
        return emit_alu_rr(as,0x09,&a,&b);
    }
    if (!strcmp(mn,"and") && na==2) {
        if (b.kind==OK_IMM) return emit_alu_imm(as,4,&a,&b);
        return emit_alu_rr(as,0x21,&a,&b);
    }
    if (!strcmp(mn,"add") && na==2) {
        if (b.kind==OK_IMM) return emit_alu_imm(as,0,&a,&b);
        return emit_alu_rr(as,0x01,&a,&b);
    }
    if (!strcmp(mn,"sub") && na==2) {
        if (b.kind==OK_IMM) return emit_alu_imm(as,5,&a,&b);
        return emit_alu_rr(as,0x29,&a,&b);
    }
    if (!strcmp(mn,"cmp") && na==2) {
        if (b.kind==OK_IMM) return emit_alu_imm(as,7,&a,&b);
        return emit_alu_rr(as,0x39,&a,&b);
    }
    if (!strcmp(mn,"inc") && na==1) {
        int w=a.width; maybe_opsz(as,w);
        if (as->bits!=64 && a.kind==OK_REG && w!=1) {
            cur_eb(as,(uint8_t)(0x40+(a.reg&7))); return 1;
        }
        rex_for_rm(as,w==8,0,&a); cur_eb(as, w==1?0xFE:0xFF);
        return emit_rm(as,0,&a,w==8);
    }
    if (!strcmp(mn,"mul") && na==1) {
        int w=a.width?a.width:4; maybe_opsz(as,w);
        rex_for_rm(as,w==8,4,&a); cur_eb(as,w==1?0xF6:0xF7);
        return emit_rm(as,4,&a,w==8);
    }
    if (!strcmp(mn,"lgdt") && na==1) {
        rex_for_rm(as,0,2,&a); cur_eb(as,0x0F); cur_eb(as,0x01);
        return emit_rm(as,2,&a,0);
    }
    if (!strcmp(mn,"lidt") && na==1) {
        rex_for_rm(as,0,3,&a); cur_eb(as,0x0F); cur_eb(as,0x01);
        return emit_rm(as,3,&a,0);
    }
    if ((!strcmp(mn,"jmp")||!strcmp(mn,"call")) && na==1)
        return emit_jmp_call(as,mn,&a);
    if (jcc_cc(mn)>=0 && na==1) return emit_jcc(as,mn,&a);
    as_err(as,"unknown instr"); return 0;
}


static int resolve_equs(As *as) {
    /* Iterate until stable (equ may reference equ). */
    for (int round=0; round<20; round++) {
        int changed=0;
        for (int i=0;i<as->nsyms;i++) {
            Sym *sy=&as->syms[i];
            if (sy->kind!=1 || !sy->equ_expr) continue;
            int64_t v; int s;
            const char *ep=sy->equ_expr;
            /* temporarily mark undefined so we don't use stale value */
            int64_t old=sy->absval; int od=sy->defined;
            sy->defined=0;
            if (!eval_ex(as,&ep,&v,&s,1) || s>=0) {
                sy->absval=old; sy->defined=od; continue;
            }
            if (!od || old!=v) changed=1;
            sy->absval=v; sy->defined=1;
        }
        if (!changed) break;
    }
    for (int i=0;i<as->nsyms;i++) {
        if (as->syms[i].kind==1 && !as->syms[i].defined) {
            as->lineno=0;
            fprintf(stderr, "%s: unresolved equ '%s'\n",
                    as->filename?as->filename:"<asm>", as->syms[i].name);
            as->failed=1; return 0;
        }
    }
    return 1;
}
static void define_label(As *as, const char *raw) {
    char nm[256];
    if (raw[0]=='.' && as->last_non_local>=0)
        snprintf(nm,sizeof nm,"%s%s", as->syms[as->last_non_local].name, raw);
    else
        snprintf(nm,sizeof nm,"%s", raw);
    int si=add_sym(as,nm);
    if (si<0) return;
    as->syms[si].sect=as->cur_sect;
    as->syms[si].off=cur_off(as);
    as->syms[si].defined=1;
    as->syms[si].kind=0;
    if (raw[0]!='.') as->last_non_local=si;
}

static void reset_sects(As *as) {
    for (int i=0;i<as->nsects;i++) {
        buffer_free(&as->sects[i].bytes);
        buffer_init(&as->sects[i].bytes);
        as->sects[i].bss_size=0;
    }
    as->nfixups=0;
    as->nrelocs=0;
}

static int process_line(As *as, char *line) {
    char *semi=strchr(line,';'); if (semi) *semi=0;
    char *hash=strchr(line,'#'); /* don't strip # in char consts — none here */
    (void)hash;
    const char *p=skip_ws(line);
    if (!*p) return 1;

    /* [bits N] or BITS N */
    if (*p=='[') {
        p++; p=skip_ws(p);
        char *id=dup_ident(&p); if(!id) return 0;
        str_tolower(id);
        if (!strcmp(id,"bits")) {
            p=skip_ws(p); int64_t v; int s;
            if (!eval_ex(as,&p,&v,&s,1)) { free(id); return 0; }
            as->bits=(int)v;
        }
        free(id);
        return 1;
    }

    /* label: optional + rest */
    if (is_id_start((unsigned char)*p)) {
        const char *s=p; char *id=dup_ident(&p);
        p=skip_ws(p);
        if (*p==':') {
            p++;
            define_label(as, id);
            free(id);
            p=skip_ws(p);
            if (!*p) return 1;
            /* fall through with rest */
        } else {
            /* could be EQU: name equ expr */
            const char *p2=p;
            char *kw=dup_ident(&p2);
            if (kw) {
                str_tolower(kw);
                if (!strcmp(kw,"equ")) {
                    p2=skip_ws(p2);
                    int si=add_sym(as,id);
                    as->syms[si].kind=1; as->syms[si].sect=-1;
                    free(as->syms[si].equ_expr);
                    as->syms[si].equ_expr=xstrdup(p2);
                    /* try immediate eval; may fail on forward refs */
                    int64_t v; int sy;
                    const char *ep=p2;
                    if (eval_ex(as,&ep,&v,&sy,1) && sy<0) {
                        as->syms[si].absval=v; as->syms[si].defined=1;
                    } else {
                        as->syms[si].defined=0;
                    }
                    free(id); free(kw); return 1;
                }
                free(kw);
            }
            /* instruction starting with mnemonic */
            p=s; free(id);
            char *mn=dup_ident(&p);
            if (!mn) return 0;
            str_tolower(mn);
            /* directives */
            if (!strcmp(mn,"bits")) {
                p=skip_ws(p); int64_t v; int sy;
                if (!eval_ex(as,&p,&v,&sy,1)){free(mn);return 0;}
                as->bits=(int)v; free(mn); return 1;
            }
            if (!strcmp(mn,"org")) {
                p=skip_ws(p); int64_t v; int sy;
                if (!eval_ex(as,&p,&v,&sy,1)){free(mn);return 0;}
                as->org=(uint64_t)v; as->has_org=1; free(mn); return 1;
            }
            if (!strcmp(mn,"section") || !strcmp(mn,"segment")) {
                p=skip_ws(p);
                const char *sn0=p;
                while (*p && (isalnum((unsigned char)*p) || *p=='_' || *p=='.' ||
                              *p=='-' || *p=='$')) p++;
                if (p==sn0) { free(mn); return 0; }
                char *sn=dup_range(sn0,p);
                int si=add_sect(as,sn);
                /* flags */
                for (;;) {
                    p=skip_ws(p); if(!*p) break;
                    char *fl=dup_ident(&p); if(!fl) break;
                    str_tolower(fl);
                    if (!strcmp(fl,"nobits")) as->sects[si].nobits=1;
                    else if (!strcmp(fl,"progbits")) as->sects[si].nobits=0;
                    else if (!strcmp(fl,"alloc")) as->sects[si].alloc=1;
                    else if (!strcmp(fl,"noalloc")) as->sects[si].alloc=0;
                    else if (!strcmp(fl,"write")) as->sects[si].write=1;
                    else if (!strcmp(fl,"nowrite")) as->sects[si].write=0;
                    else if (!strcmp(fl,"exec")) as->sects[si].exec=1;
                    else if (!strcmp(fl,"noexec")) as->sects[si].exec=0;
                    free(fl);
                }
                as->cur_sect=si; free(sn); free(mn); return 1;
            }
            if (!strcmp(mn,"global") || !strcmp(mn,"globl")) {
                for(;;){
                    p=skip_ws(p); if(!*p) break;
                    char *n=dup_ident(&p); if(!n) break;
                    int si=add_sym(as,n); as->syms[si].is_global=1; free(n);
                    p=skip_ws(p); if(*p==',') p++;
                }
                free(mn); return 1;
            }
            if (!strcmp(mn,"extern")) {
                for(;;){
                    p=skip_ws(p); if(!*p) break;
                    char *n=dup_ident(&p); if(!n) break;
                    int si=add_sym(as,n); as->syms[si].kind=2; as->syms[si].defined=0; free(n);
                    p=skip_ws(p); if(*p==',') p++;
                }
                free(mn); return 1;
            }
            if (!strcmp(mn,"default")) {
                p=skip_ws(p); char *m=dup_ident(&p);
                if (m){ str_tolower(m); as->default_rel=!strcmp(m,"rel"); free(m); }
                free(mn); return 1;
            }
            if (!strcmp(mn,"align")) {
                p=skip_ws(p); int64_t v; int sy;
                if (!eval_ex(as,&p,&v,&sy,1)){free(mn);return 0;}
                do_align(as,(int)v, !cur(as)->nobits);
                free(mn); return 1;
            }
            if (!strcmp(mn,"alignb")) {
                p=skip_ws(p); int64_t v; int sy;
                if (!eval_ex(as,&p,&v,&sy,1)){free(mn);return 0;}
                do_align(as,(int)v, 0);
                free(mn); return 1;
            }
            if (!strcmp(mn,"times")) {
                p=skip_ws(p); int64_t cnt; int sy;
                if (!eval_ex(as,&p,&cnt,&sy,as->pass==1)){free(mn);return 0;}
                p=skip_ws(p);
                /* body: usually `db 0` */
                char body[256];
                snprintf(body,sizeof body,"%s",p);
                free(mn);
                for (int64_t i=0;i<cnt;i++) {
                    char tmp[256]; snprintf(tmp,sizeof tmp,"%s",body);
                    if (!process_line(as,tmp)) return 0;
                }
                return 1;
            }
            if (!strcmp(mn,"db")||!strcmp(mn,"dw")||!strcmp(mn,"dd")||!strcmp(mn,"dq")) {
                int sz = mn[1]=='b'?1:mn[1]=='w'?2:mn[1]=='d'?4:8;
                free(mn);
                for(;;){
                    p=skip_ws(p); if(!*p) break;
                    int64_t v; int sy;
                    if (!eval_ex(as,&p,&v,&sy,as->pass==1)) return 0;
                    size_t off=cur_off(as);
                    for(int i=0;i<sz;i++) cur_eb(as,0);
                    add_fixup(as,off,sz,v,sy,0,
                        as->fmt==NASM_FMT_ELF64&&sy>=0?(sz==8?R64_64:R64_32):0,-1);
                    p=skip_ws(p); if(*p==',') p++; else break;
                }
                return 1;
            }
            if (!strcmp(mn,"resb")||!strcmp(mn,"resw")||!strcmp(mn,"resd")||!strcmp(mn,"resq")) {
                int sz = mn[3]=='b'?1:mn[3]=='w'?2:mn[3]=='d'?4:8;
                free(mn);
                p=skip_ws(p); int64_t n; int sy;
                if (!eval_ex(as,&p,&n,&sy,1)) return 0;
                Sect *s=cur(as);
                if (!s->nobits) {
                    for (int64_t i=0;i<n*sz;i++) cur_eb(as,0);
                } else s->bss_size += (size_t)(n*sz);
                return 1;
            }
            /* real instruction */
            if (!assemble_instr(as, mn, p)) { free(mn); return 0; }
            free(mn); return 1;
        }
        /* after label: rest of line */
        if (*p) {
            char rest[512]; snprintf(rest,sizeof rest,"%s",p);
            return process_line(as, rest);
        }
        return 1;
    }
    as_err(as,"bad line"); return 0;
}

static void apply_fixups(As *as) {
    for (int i = 0; i < as->nfixups; i++) {
        Fixup *f = &as->fixups[i];
        Sect *s = &as->sects[f->sect];
        int64_t val = f->add;
        int sym = f->sym;

        if (as->fmt == NASM_FMT_BIN) {
            if (sym >= 0) {
                Sym *sy = &as->syms[sym];
                if (f->pc_rel) {
                    val = (int64_t)sy->off - ((int64_t)f->offset + f->size);
                } else if (sy->kind == 1) {
                    val = sy->absval;
                } else if (sy->defined) {
                    /* f->add is section-relative (off[+const]) from expr */
                    val = (as->has_org ? (int64_t)as->org : 0) + f->add;
                }
            } else {
                val = f->add;
            }
        } else {
            /* ELF64 */
            if (sym >= 0) {
                Sym *sy = &as->syms[sym];
                int rtype = f->reloc_type;
                if (f->pc_rel && !rtype) {
                    val = (int64_t)sy->off - ((int64_t)f->offset + f->size);
                } else if (rtype == R64_PC32) {
                    add_reloc(as, f->sect, f->offset, sym, (int32_t)f->add, rtype);
                    val = 0;
                } else {
                    if (!rtype)
                        rtype = (f->size == 8) ? R64_64 :
                                (as->bits == 64 ? R64_32S : R64_32);
                    int32_t addend = (int32_t)(f->add - (int64_t)sy->off);
                    add_reloc(as, f->sect, f->offset, sym, addend, rtype);
                    val = 0;
                }
            } else {
                val = f->add;
            }
        }

        if (s->nobits) continue;
        if (f->offset + (size_t)f->size > s->bytes.len) continue;
        for (int k = 0; k < f->size; k++)
            s->bytes.data[f->offset + (size_t)k] = (char)((val >> (8 * k)) & 0xff);
    }
}

/* ---- writers ---- */
static int write_bin(As *as, const char *path) {
    /* Concatenate progbits sections in order; for ap_tramp only .text */
    FILE *fp=fopen(path,"wb");
    if (!fp) { perror(path); return -1; }
    for (int i=0;i<as->nsects;i++) {
        Sect *s=&as->sects[i];
        if (s->nobits || !s->alloc) continue;
        if (s->bytes.len && fwrite(s->bytes.data,1,s->bytes.len,fp)!=s->bytes.len) {
            fclose(fp); return -1;
        }
    }
    fclose(fp); return 0;
}

static void buf_u8(Buffer *b, uint8_t v){eb(b,v);}
static void buf_u16(Buffer *b, uint16_t v){uint8_t d[2]={(uint8_t)v,(uint8_t)(v>>8)};buffer_append(b,(const char*)d,2);}
static void buf_u32(Buffer *b, uint32_t v){uint8_t d[4]={(uint8_t)v,(uint8_t)(v>>8),(uint8_t)(v>>16),(uint8_t)(v>>24)};buffer_append(b,(const char*)d,4);}
static void buf_u64(Buffer *b, uint64_t v){for(int i=0;i<8;i++) buf_u8(b,(uint8_t)(v>>(8*i)));}
static void buf_z(Buffer *b, size_t n){for(size_t i=0;i<n;i++) buf_u8(b,0);}

static int write_elf64(As *as, const char *path) {
    /* Build ET_REL with custom sections, symtab, rela */
    Buffer out; buffer_init(&out);
    Buffer shstr; buffer_init(&shstr); buf_u8(&shstr,0);
    Buffer strtab; buffer_init(&strtab); buf_u8(&strtab,0);
    Buffer symtab; buffer_init(&symtab);
    /* null symbol */
    buf_z(&symtab, 24);

    int nprog=0;
    /* Assign shndx: 1.. for each sect */
    int shndx=1;
    uint32_t shname_offs[MAX_SECTS];
    for (int i=0;i<as->nsects;i++) {
        shname_offs[i]=(uint32_t)shstr.len;
        buffer_append(&shstr, as->sects[i].name, strlen(as->sects[i].name)+1);
        as->sects[i].shndx=shndx;
        shndx++;
    }
    uint32_t name_symtab=(uint32_t)shstr.len; buffer_append(&shstr,".symtab",8);
    uint32_t name_strtab=(uint32_t)shstr.len; buffer_append(&shstr,".strtab",8);
    uint32_t name_shstr=(uint32_t)shstr.len; buffer_append(&shstr,".shstrtab",10);

    /* rela section names */
    uint32_t rela_names[MAX_SECTS];
    int has_rela[MAX_SECTS];
    memset(has_rela,0,sizeof has_rela);
    for (int i=0;i<as->nrelocs;i++) has_rela[as->relocs[i].sect]=1;
    for (int i=0;i<as->nsects;i++) {
        if (!has_rela[i]) { rela_names[i]=0; continue; }
        char tmp[128]; snprintf(tmp,sizeof tmp,".rela%s", as->sects[i].name);
        rela_names[i]=(uint32_t)shstr.len;
        buffer_append(&shstr,tmp,strlen(tmp)+1);
    }

    /* FILE symbol */
    {
        uint32_t no=(uint32_t)strtab.len;
        const char *fn=as->filename?as->filename:"a.asm";
        buffer_append(&strtab,fn,strlen(fn)+1);
        buf_u32(&symtab,no); buf_u8(&symtab,4); /* STB_LOCAL<<4 | STT_FILE */
        buf_u8(&symtab,0); buf_u16(&symtab,0xfff1); /* SHN_ABS */
        buf_u64(&symtab,0); buf_u64(&symtab,0);
    }
    /* section symbols */
    int sect_sym_index[MAX_SECTS];
    for (int i=0;i<as->nsects;i++) {
        sect_sym_index[i] = (int)(symtab.len/24);
        buf_u32(&symtab,0); buf_u8(&symtab,3); buf_u8(&symtab,0);
        buf_u16(&symtab,(uint16_t)as->sects[i].shndx);
        buf_u64(&symtab,0); buf_u64(&symtab,0);
    }
    /* user symbols */
    int sym_index[MAX_SYMS];
    memset(sym_index,0xff,sizeof sym_index);
    int first_global = (int)(symtab.len/24);
    /* locals first */
    for (int pass=0; pass<2; pass++) {
        if (pass==1) first_global=(int)(symtab.len/24);
        for (int i=0;i<as->nsyms;i++) {
            Sym *sy=&as->syms[i];
            int is_g = sy->is_global || sy->kind==2;
            if (pass==0 && is_g) continue;
            if (pass==1 && !is_g) continue;
            uint32_t no=(uint32_t)strtab.len;
            buffer_append(&strtab,sy->name,strlen(sy->name)+1);
            sym_index[i]=(int)(symtab.len/24);
            uint8_t bind = is_g ? 1 : 0; /* GLOBAL/LOCAL */
            uint8_t type = 0;
            uint16_t ndx = 0;
            uint64_t val=0;
            if (sy->kind==1) { ndx=0xfff1; val=(uint64_t)sy->absval; }
            else if (sy->kind==2) { ndx=0; }
            else if (sy->defined) { ndx=(uint16_t)as->sects[sy->sect].shndx; val=sy->off; }
            buf_u32(&symtab,no);
            buf_u8(&symtab,(uint8_t)((bind<<4)|type));
            buf_u8(&symtab,0);
            buf_u16(&symtab,ndx);
            buf_u64(&symtab,val);
            buf_u64(&symtab,0);
        }
    }

    /* remap reloc sym indices; if we used label syms we're good.
       Also convert fixups that need section symbols — already added in apply. */

    Buffer rela_bufs[MAX_SECTS];
    for (int i=0;i<as->nsects;i++) buffer_init(&rela_bufs[i]);
    for (int i=0;i<as->nrelocs;i++) {
        Reloc *r=&as->relocs[i];
        int si = sym_index[r->sym];
        if (si<0) si = sect_sym_index[as->syms[r->sym].sect >=0 ? as->syms[r->sym].sect : 0];
        buf_u64(&rela_bufs[r->sect], r->offset);
        buf_u64(&rela_bufs[r->sect], ((uint64_t)(uint32_t)si << 32) | (uint32_t)r->type);
        buf_u64(&rela_bufs[r->sect], (uint64_t)(int64_t)r->addend);
    }

    /* layout: ehdr | section data | shstrtab | symtab | strtab | rela* | shdrs */
    buf_z(&out, 64); /* ehdr filled later */

    size_t sect_off[MAX_SECTS];
    size_t sect_sz[MAX_SECTS];
    for (int i=0;i<as->nsects;i++) {
        Sect *s=&as->sects[i];
        /* align */
        while (out.len % 16) buf_u8(&out,0);
        sect_off[i]=out.len;
        if (s->nobits) { sect_sz[i]=s->bss_size; }
        else {
            sect_sz[i]=s->bytes.len;
            if (s->bytes.len) buffer_append(&out,s->bytes.data,s->bytes.len);
        }
    }
    while (out.len % 8) buf_u8(&out,0);
    size_t off_shstr=out.len; buffer_append(&out,shstr.data,shstr.len);
    while (out.len % 8) buf_u8(&out,0);
    size_t off_sym=out.len; buffer_append(&out,symtab.data,symtab.len);
    while (out.len % 8) buf_u8(&out,0);
    size_t off_str=out.len; buffer_append(&out,strtab.data,strtab.len);

    size_t off_rela[MAX_SECTS];
    for (int i=0;i<as->nsects;i++) {
        off_rela[i]=0;
        if (!has_rela[i]) continue;
        while (out.len % 8) buf_u8(&out,0);
        off_rela[i]=out.len;
        buffer_append(&out, rela_bufs[i].data, rela_bufs[i].len);
    }

    while (out.len % 8) buf_u8(&out,0);
    size_t off_shdrs=out.len;

    /* null shdr */
    buf_z(&out, 64);
    /* section shdrs */
    int cur_sh=1;
    for (int i=0;i<as->nsects;i++) {
        Sect *s=&as->sects[i];
        uint32_t type = s->nobits ? 8 : 1; /* NOBITS : PROGBITS */
        uint64_t flags=0;
        if (s->alloc) flags|=2;
        if (s->write) flags|=1;
        if (s->exec) flags|=4;
        buf_u32(&out, shname_offs[i]);
        buf_u32(&out, type);
        buf_u64(&out, flags);
        buf_u64(&out, 0);
        buf_u64(&out, s->nobits ? 0 : sect_off[i]);
        buf_u64(&out, sect_sz[i]);
        buf_u32(&out, 0); buf_u32(&out, 0);
        buf_u64(&out, (uint64_t)s->align);
        buf_u64(&out, 0);
        cur_sh++;
    }
    int symtab_shndx=cur_sh;
    buf_u32(&out, name_symtab); buf_u32(&out, 2); /* SYMTAB */
    buf_u64(&out, 0); buf_u64(&out, 0); buf_u64(&out, off_sym); buf_u64(&out, symtab.len);
    buf_u32(&out, cur_sh+1); /* link strtab */
    buf_u32(&out, (uint32_t)first_global);
    buf_u64(&out, 8); buf_u64(&out, 24);
    cur_sh++;
    int strtab_shndx=cur_sh;
    buf_u32(&out, name_strtab); buf_u32(&out, 3);
    buf_u64(&out,0);buf_u64(&out,0);buf_u64(&out,off_str);buf_u64(&out,strtab.len);
    buf_u32(&out,0);buf_u32(&out,0);buf_u64(&out,1);buf_u64(&out,0);
    cur_sh++;
    for (int i=0;i<as->nsects;i++) {
        if (!has_rela[i]) continue;
        buf_u32(&out, rela_names[i]); buf_u32(&out, 4); /* RELA */
        buf_u64(&out, 0); /* flags */
        buf_u64(&out, 0);
        buf_u64(&out, off_rela[i]);
        buf_u64(&out, rela_bufs[i].len);
        buf_u32(&out, (uint32_t)symtab_shndx);
        buf_u32(&out, (uint32_t)as->sects[i].shndx);
        buf_u64(&out, 8); buf_u64(&out, 24);
        cur_sh++;
    }
    int shstr_shndx=cur_sh;
    buf_u32(&out, name_shstr); buf_u32(&out, 3);
    buf_u64(&out,0);buf_u64(&out,0);buf_u64(&out,off_shstr);buf_u64(&out,shstr.len);
    buf_u32(&out,0);buf_u32(&out,0);buf_u64(&out,1);buf_u64(&out,0);
    cur_sh++;

    /* ELF header */
    uint8_t *h=(uint8_t*)out.data;
    memset(h,0,64);
    h[0]=0x7f;h[1]='E';h[2]='L';h[3]='F';
    h[4]=2;h[5]=1;h[6]=1;
    /* e_type ET_REL=1 */
    h[16]=1; h[17]=0;
    h[18]=0x3e; h[19]=0; /* EM_X86_64 */
    h[20]=1; /* version */
    /* e_shoff */
    uint64_t shoff=off_shdrs;
    memcpy(h+40,&shoff,8);
    h[54]=64; /* e_ehsize */
    h[58]=64; /* e_shentsize */
    uint16_t shnum=(uint16_t)cur_sh; memcpy(h+60,&shnum,2);
    uint16_t shstrndx=(uint16_t)shstr_shndx; memcpy(h+62,&shstrndx,2);

    FILE *fp=fopen(path,"wb");
    if (!fp) { buffer_free(&out); return -1; }
    fwrite(out.data,1,out.len,fp); fclose(fp);

    buffer_free(&out); buffer_free(&shstr); buffer_free(&strtab); buffer_free(&symtab);
    for (int i=0;i<as->nsects;i++) buffer_free(&rela_bufs[i]);
    (void)nprog;(void)strtab_shndx;
    return 0;
}

static int run_assemble(As *as, const char *src) {
    for (int pass=1; pass<=2; pass++) {
        as->pass=pass;
        as->bits = (as->fmt==NASM_FMT_BIN)?16:32;
        as->default_rel=0;
        as->cur_sect=-1;
        as->last_non_local=-1;
        as->failed=0;
        if (pass==1) {
            /* create default section */
            as->nsects=0; as->nsyms=0; /* keep? equ need both passes */
        }
        if (pass==2) {
            reset_sects(as);
            /* Keep pass-1 label offsets so early equ/expressions resolve; labels
             * are refreshed to the same offsets as we re-emit. */
            as->last_non_local=-1;
            as->cur_sect=-1;
        } else {
            as->nsects=0; as->nsyms=0;
        }
        as->lineno=0;
        const char *p=src;
        while (*p) {
            const char *e=p; while (*e && *e!='\n') e++;
            size_t n=(size_t)(e-p);
            char *line=(char*)xmalloc(n+1); memcpy(line,p,n); line[n]=0;
            as->lineno++;
            if (!process_line(as,line)) { free(line); return -1; }
            free(line);
            if (as->failed) return -1;
            p = (*e=='\n') ? e+1 : e;
        }
        if (!resolve_equs(as)) return -1;
        if (pass==2) apply_fixups(as);
    }
    return as->failed ? -1 : 0;
}

int is_nasm_input(const char *path) {
    size_t n=strlen(path);
    return (n>=4 && (!strcmp(path+n-4,".asm") || !strcmp(path+n-5,".nasm")));
}

int assemble_nasm(const char *src, const char *filename, NasmFormat fmt, const char *out_path) {
    As as;
    memset(&as,0,sizeof as);
    as.fmt=fmt; as.filename=filename; as.bits=16; as.cur_sect=-1;
    if (run_assemble(&as, src) != 0) return -1;
    int rc = (fmt==NASM_FMT_BIN) ? write_bin(&as,out_path) : write_elf64(&as,out_path);
    for (int i=0;i<as.nsects;i++) { buffer_free(&as.sects[i].bytes); free(as.sects[i].name); }
    for (int i=0;i<as.nsyms;i++) { free(as.syms[i].name); free(as.syms[i].equ_expr); }
    return rc;
}
