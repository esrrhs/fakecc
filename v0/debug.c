package main;

static int __fakecc_ctzll(unsigned long _v){int c;for(c=0;!(_v&1);c++)_v>>=1;return c;}
static void __fakecc_va_copy(void *dst, void *src){
    char *d = (char*)dst; char *s = (char*)src;
    for(int i = 0; i < 24; i++) d[i] = s[i];
}

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
struct SourceLoc {
    const char *file;
    int line;
    int col;
};typedef struct SourceLoc SourceLoc;
struct Buffer {
    char *data;
    size_t len;
    size_t cap;
};typedef struct Buffer Buffer;
void buffer_init(Buffer *b);
void buffer_free(Buffer *b);
void buffer_append(Buffer *b, const char *s, size_t n);
void buffer_appendf(Buffer *b, const char *fmt, ...);
char *xstrdup(const char *s);
void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
void die_at(const char *file, int line, int col, const char *fmt, ...);
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;
typedef int bool;
struct EmitSymbol {
    char *name;
    uint8_t binding;
    uint8_t type;
    uint16_t shndx;
    size_t value;
    size_t size;
};typedef struct EmitSymbol EmitSymbol;
struct EmitReloc {
    size_t offset;
    uint32_t type;
    uint32_t sym;
    int32_t addend;
};typedef struct EmitReloc EmitReloc;
enum DebugVarKind {
    DBG_VAR_PARAM = 0,
    DBG_VAR_LOCAL = 1,
    DBG_VAR_GLOBAL = 2
};typedef enum DebugVarKind DebugVarKind;
enum DebugTypeTag {
    DBG_TY_VOID = 0,
    DBG_TY_INT = 1,
    DBG_TY_FLOAT = 2,
    DBG_TY_PTR = 3,
    DBG_TY_ARRAY = 4,
    DBG_TY_STRUCT = 5,
    DBG_TY_BOOL = 6
};typedef enum DebugTypeTag DebugTypeTag;
enum DebugLocKind {
    DBG_LOC_NONE = 0,
    DBG_LOC_FBREG = 1,
    DBG_LOC_REG = 2,
    DBG_LOC_ADDR = 3
};typedef enum DebugLocKind DebugLocKind;
struct DebugLocRange {
    size_t pc_start;
    size_t pc_end;
    DebugLocKind loc_kind;
    int rbp_offset;
    int dwarf_reg;
};typedef struct DebugLocRange DebugLocRange;
struct DebugVar {
    char *name;
    char *file;
    int line;
    DebugVarKind kind;
    DebugTypeTag type_tag;
    int width;
    int is_unsigned;
    int array_len;
    DebugLocKind loc_kind;
    int rbp_offset;
    int dwarf_reg;
    char *sym_name;
    DebugLocRange *ranges;
    size_t num_ranges;
    size_t cap_ranges;
    int alloca_ssa;
    int param_idx;
};typedef struct DebugVar DebugVar;
struct DebugLineEntry {
    char *file;
    int line;
    int col;
    size_t pc_off;
};typedef struct DebugLineEntry DebugLineEntry;
struct DebugFunc {
    char *name;
    char *file;
    int line;
    size_t start_pc;
    size_t end_pc;
    size_t prologue_end_pc;
    size_t after_push_rbp_pc;
    size_t after_mov_rbp_pc;
    DebugVar *vars;
    size_t num_vars;
    size_t cap_vars;
};typedef struct DebugFunc DebugFunc;
struct EmitModule {
    Buffer text;
    Buffer rodata;
    Buffer data;
    size_t bss_size;
    EmitSymbol *syms;
    size_t num_syms;
    size_t cap_syms;
    EmitReloc *relocs;
    size_t num_relocs;
    size_t cap_relocs;
    EmitReloc *data_relocs;
    size_t num_data_relocs;
    size_t cap_data_relocs;
    char *dbg_tu_name;
    DebugLineEntry *dbg_lines;
    size_t num_dbg_lines;
    size_t cap_dbg_lines;
    DebugFunc *dbg_funcs;
    size_t num_dbg_funcs;
    size_t cap_dbg_funcs;
    DebugVar *dbg_globals;
    size_t num_dbg_globals;
    size_t cap_dbg_globals;
};typedef struct EmitModule EmitModule;
void emit_module_init(EmitModule *m);
void emit_module_free(EmitModule *m);
int emit_module_add_symbol(EmitModule *m, const char *name,
                            uint8_t binding, uint8_t type,
                            uint16_t shndx, size_t value, size_t size);
int emit_module_find_symbol(EmitModule *m, const char *name);
int emit_module_add_undefined(EmitModule *m, const char *name);
void emit_module_add_reloc(EmitModule *m, size_t offset, uint32_t type,
                           int sym, int32_t addend);
void emit_module_add_data_reloc(EmitModule *m, size_t offset, uint32_t type,
                                int sym, int32_t addend);
void emit_obj(const EmitModule *m, const char *path);
int emit_obj_read(const char *path, EmitModule *m);
void emit_link(EmitModule **mods, size_t n, const char *path,
               const char **needed, size_t num_needed, int nodefaultlibs,
               const char **lib_paths, size_t num_lib_paths,
               int want_debug);
void emit_elf(const EmitModule *m, const char *path);
void debug_emit_dwarf(const EmitModule *m, uint64_t text_base_vaddr,
                      Buffer *debug_abbrev, Buffer *debug_info,
                      Buffer *debug_str, Buffer *debug_line,
                      Buffer *debug_frame, Buffer *debug_loc);
void debug_serialize(const EmitModule *m, Buffer *out);
int debug_deserialize(EmitModule *m, const unsigned char *data, size_t len);
void emit_module_add_dbg_line(EmitModule *m, const char *file, int line,
                              int col, size_t pc_off);
int emit_module_add_dbg_func(EmitModule *m, const char *name,
                              const char *file, int line, size_t start_pc);
void emit_module_dbg_func_end(EmitModule *m, int func_idx, size_t end_pc,
                              size_t prologue_end_pc);
void emit_module_dbg_func_frame(EmitModule *m, int func_idx,
                                size_t after_push_rbp_pc,
                                size_t after_mov_rbp_pc);
void emit_module_add_dbg_var(EmitModule *m, int func_idx, const DebugVar *v);
void emit_module_add_dbg_global(EmitModule *m, const DebugVar *v);
void debug_var_release(DebugVar *v);
typedef struct FILE FILE;
extern FILE *stderr;
extern FILE *stdin;
extern FILE *stdout;
extern int fprintf(FILE *f, const char *fmt, ...);
extern int vfprintf(FILE *f, const char *fmt, va_list ap);
extern int printf(const char *fmt, ...);
extern int sprintf(char *buf, const char *fmt, ...);
extern int snprintf(char *buf, size_t n, const char *fmt, ...);
extern int fputs(const char *s, FILE *f);
extern int fputc(int c, FILE *f);
extern int fflush(FILE *f);
extern int puts(const char *s);
extern int putchar(int c);
extern FILE *fopen(const char *p, const char *m);
extern int fclose(FILE *f);
extern size_t fwrite(const void *p, size_t n, size_t m, FILE *f);
extern size_t fread(void *p, size_t n, size_t m, FILE *f);
extern void perror(const char *s);
extern int fileno(FILE *f);
extern int fseek(FILE *f, long off, int whence);
extern long ftell(FILE *f);
typedef long fpos_t;
extern void *malloc(size_t n);
extern void *realloc(void *p, size_t n);
extern void *calloc(size_t n, size_t m);
extern void free(void *p);
extern void exit(int code);
extern void abort(void);
extern int atoi(const char *s);
extern long atol(const char *s);
extern long strtol(const char *s, char **end, int base);
extern double strtod(const char *s, char **end);
extern long double strtold(const char *nptr, char **endptr);
extern void qsort(void *base, size_t n, size_t sz, int (*cmp)(const void*, const void*));
extern char *getenv(const char *name);
extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memmove(void *dst, const void *src, size_t n);
extern void *memset(void *dst, int c, size_t n);
extern int memcmp(const void *a, const void *b, size_t n);
extern size_t strlen(const char *s);
extern char *strdup(const char *s);
extern int strcmp(const char *a, const char *b);
extern int strncmp(const char *a, const char *b, size_t n);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern char *strstr(const char *a, const char *b);
extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, size_t n);
extern char *strerror(int n);
static void d_u8(Buffer *b, uint8_t v) { buffer_append(b, (const char *)&v, 1); }
static void d_u16(Buffer *b, uint16_t v) { buffer_append(b, (const char *)&v, 2); }
static void d_u32(Buffer *b, uint32_t v) { buffer_append(b, (const char *)&v, 4); }
static void d_u64(Buffer *b, uint64_t v) { buffer_append(b, (const char *)&v, 8); }
static void d_bytes(Buffer *b, const void *p, size_t n) {
    buffer_append(b, (const char *)p, n);
}
static void d_sleb(Buffer *b, int64_t v) {
    int more = 1;
    while (more) {
        uint8_t byte = (uint8_t)(v & 0x7f);
        v >>= 7;
        int sign = (byte & 0x40) != 0;
        if ((v == 0 && !sign) || (v == -1 && sign)) more = 0;
        else byte |= 0x80;
        d_u8(b, byte);
    }
}
static void d_uleb(Buffer *b, uint64_t v) {
    do {
        uint8_t byte = (uint8_t)(v & 0x7f);
        v >>= 7;
        if (v) byte |= 0x80;
        d_u8(b, byte);
    } while (v);
}
struct Strtab { Buffer buf; };typedef struct Strtab Strtab;
static void strtab_init(Strtab *s) { buffer_init(&s->buf); d_u8(&s->buf, 0); }
static void strtab_free(Strtab *s) { buffer_free(&s->buf); }
static uint32_t strtab_add(Strtab *s, const char *str) {
    if (!str) str = "";
    uint32_t off = (uint32_t)s->buf.len;
    d_bytes(&s->buf, str, strlen(str) + 1);
    return off;
}
void emit_module_add_dbg_line(EmitModule *m, const char *file, int line,
                              int col, size_t pc_off) {
    if (!file || line <= 0) return;
    if (m->num_dbg_lines > 0) {
        DebugLineEntry *last = &m->dbg_lines[m->num_dbg_lines - 1];
        if (last->pc_off == pc_off) {
            last->line = line;
            last->col = col;
            return;
        }
        if (last->line == line && last->col == col && last->file &&
            strcmp(last->file, file) == 0)
            return;
    }
    if (m->num_dbg_lines >= m->cap_dbg_lines) {
        size_t nc = m->cap_dbg_lines ? m->cap_dbg_lines * 2 : 16;
        m->dbg_lines = realloc(m->dbg_lines, nc * sizeof(DebugLineEntry));
        if (!m->dbg_lines) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->cap_dbg_lines = nc;
    }
    DebugLineEntry *e = &m->dbg_lines[m->num_dbg_lines++];
    e->file = xstrdup(file);
    e->line = line;
    e->col = col;
    e->pc_off = pc_off;
}
int emit_module_add_dbg_func(EmitModule *m, const char *name,
                             const char *file, int line, size_t start_pc) {
    if (m->num_dbg_funcs >= m->cap_dbg_funcs) {
        size_t nc = m->cap_dbg_funcs ? m->cap_dbg_funcs * 2 : 4;
        m->dbg_funcs = realloc(m->dbg_funcs, nc * sizeof(DebugFunc));
        if (!m->dbg_funcs) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->cap_dbg_funcs = nc;
    }
    DebugFunc *f = &m->dbg_funcs[m->num_dbg_funcs];
    memset(f, 0, sizeof(*f));
    f->name = xstrdup(name ? name : "");
    f->file = file ? xstrdup(file) : ((void*)0);
    f->line = line;
    f->start_pc = start_pc;
    f->end_pc = start_pc;
    f->prologue_end_pc = start_pc;
    return (int)m->num_dbg_funcs++;
}
void emit_module_dbg_func_end(EmitModule *m, int func_idx, size_t end_pc,
                              size_t prologue_end_pc) {
    if (func_idx < 0 || (size_t)func_idx >= m->num_dbg_funcs) return;
    m->dbg_funcs[func_idx].end_pc = end_pc;
    m->dbg_funcs[func_idx].prologue_end_pc = prologue_end_pc;
}
void emit_module_dbg_func_frame(EmitModule *m, int func_idx,
                                size_t after_push_rbp_pc,
                                size_t after_mov_rbp_pc) {
    if (func_idx < 0 || (size_t)func_idx >= m->num_dbg_funcs) return;
    m->dbg_funcs[func_idx].after_push_rbp_pc = after_push_rbp_pc;
    m->dbg_funcs[func_idx].after_mov_rbp_pc = after_mov_rbp_pc;
}
static void debug_var_copy(DebugVar *dst, const DebugVar *src) {
    memset(dst, 0, sizeof(*dst));
    dst->name = src->name ? xstrdup(src->name) : ((void*)0);
    dst->file = src->file ? xstrdup(src->file) : ((void*)0);
    dst->line = src->line;
    dst->kind = src->kind;
    dst->type_tag = src->type_tag;
    dst->width = src->width;
    dst->is_unsigned = src->is_unsigned;
    dst->array_len = src->array_len;
    dst->loc_kind = src->loc_kind;
    dst->rbp_offset = src->rbp_offset;
    dst->dwarf_reg = src->dwarf_reg;
    dst->sym_name = src->sym_name ? xstrdup(src->sym_name) : ((void*)0);
    dst->alloca_ssa = src->alloca_ssa;
    dst->param_idx = src->param_idx;
    if (src->num_ranges > 0) {
        dst->ranges = xmalloc(src->num_ranges * sizeof(DebugLocRange));
        memcpy(dst->ranges, src->ranges,
               src->num_ranges * sizeof(DebugLocRange));
        dst->num_ranges = src->num_ranges;
        dst->cap_ranges = src->num_ranges;
    }
}
void debug_var_release(DebugVar *v) {
    free(v->name);
    free(v->file);
    free(v->sym_name);
    free(v->ranges);
    v->name = ((void*)0);
    v->file = ((void*)0);
    v->sym_name = ((void*)0);
    v->ranges = ((void*)0);
    v->num_ranges = 0;
    v->cap_ranges = 0;
}
void emit_module_add_dbg_var(EmitModule *m, int func_idx, const DebugVar *v) {
    if (func_idx < 0 || (size_t)func_idx >= m->num_dbg_funcs) return;
    DebugFunc *f = &m->dbg_funcs[func_idx];
    if (f->num_vars >= f->cap_vars) {
        size_t nc = f->cap_vars ? f->cap_vars * 2 : 4;
        f->vars = realloc(f->vars, nc * sizeof(DebugVar));
        if (!f->vars) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        f->cap_vars = nc;
    }
    debug_var_copy(&f->vars[f->num_vars++], v);
}
void emit_module_add_dbg_global(EmitModule *m, const DebugVar *v) {
    if (m->num_dbg_globals >= m->cap_dbg_globals) {
        size_t nc = m->cap_dbg_globals ? m->cap_dbg_globals * 2 : 4;
        m->dbg_globals = realloc(m->dbg_globals, nc * sizeof(DebugVar));
        if (!m->dbg_globals) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->cap_dbg_globals = nc;
    }
    debug_var_copy(&m->dbg_globals[m->num_dbg_globals++], v);
}
static void ser_str(Buffer *b, const char *s) {
    if (!s) s = "";
    uint32_t n = (uint32_t)strlen(s);
    d_u32(b, n);
    d_bytes(b, s, n);
}
static int deser_str(const unsigned char **p, const unsigned char *end, char **out) {
    if (*p + 4 > end) return -1;
    uint32_t n = (uint32_t)((*p)[0] | ((*p)[1] << 8) | ((*p)[2] << 16) | ((*p)[3] << 24));
    *p += 4;
    if (*p + n > end) return -1;
    *out = malloc(n + 1);
    if (!*out) return -1;
    memcpy(*out, *p, n);
    (*out)[n] = '\0';
    *p += n;
    return 0;
}
static uint32_t rd32(const unsigned char **p) {
    uint32_t v = (uint32_t)((*p)[0] | ((*p)[1] << 8) | ((*p)[2] << 16) | ((*p)[3] << 24));
    *p += 4;
    return v;
}
static uint64_t rd64(const unsigned char **p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)(*p)[i] << (8 * i);
    *p += 8;
    return v;
}
static void ser_var(Buffer *b, const DebugVar *v) {
    ser_str(b, v->name);
    ser_str(b, v->file);
    d_u32(b, (uint32_t)v->line);
    d_u8(b, (uint8_t)v->kind);
    d_u8(b, (uint8_t)v->type_tag);
    d_u32(b, (uint32_t)v->width);
    d_u8(b, (uint8_t)v->is_unsigned);
    d_u32(b, (uint32_t)v->array_len);
    d_u8(b, (uint8_t)v->loc_kind);
    d_u32(b, (uint32_t)v->rbp_offset);
    d_u32(b, (uint32_t)v->dwarf_reg);
    ser_str(b, v->sym_name);
    d_u32(b, (uint32_t)v->alloca_ssa);
    d_u32(b, (uint32_t)v->param_idx);
    d_u32(b, (uint32_t)v->num_ranges);
    for (size_t i = 0; i < v->num_ranges; i++) {
        const DebugLocRange *r = &v->ranges[i];
        d_u64(b, (uint64_t)r->pc_start);
        d_u64(b, (uint64_t)r->pc_end);
        d_u8(b, (uint8_t)r->loc_kind);
        d_u32(b, (uint32_t)r->rbp_offset);
        d_u32(b, (uint32_t)r->dwarf_reg);
    }
}
static int deser_var(const unsigned char **p, const unsigned char *end, DebugVar *v) {
    memset(v, 0, sizeof(*v));
    if (deser_str(p, end, &v->name) < 0) return -1;
    if (deser_str(p, end, &v->file) < 0) return -1;
    if (*p + 4 > end) return -1;
    v->line = (int)rd32(p);
    if (*p + 1 > end) return -1;
    v->kind = (DebugVarKind)**p; (*p)++;
    if (*p + 1 > end) return -1;
    v->type_tag = (DebugTypeTag)**p; (*p)++;
    if (*p + 4 > end) return -1;
    v->width = (int)rd32(p);
    if (*p + 1 > end) return -1;
    v->is_unsigned = **p; (*p)++;
    if (*p + 4 > end) return -1;
    v->array_len = (int)rd32(p);
    if (*p + 1 > end) return -1;
    v->loc_kind = (DebugLocKind)**p; (*p)++;
    if (*p + 4 > end) return -1;
    v->rbp_offset = (int)rd32(p);
    if (*p + 4 > end) return -1;
    v->dwarf_reg = (int)rd32(p);
    if (deser_str(p, end, &v->sym_name) < 0) return -1;
    if (*p + 4 > end) return -1;
    v->alloca_ssa = (int)rd32(p);
    if (*p + 4 > end) return -1;
    v->param_idx = (int)rd32(p);
    if (*p + 4 > end) return -1;
    uint32_t nranges = rd32(p);
    if (nranges > 0) {
        v->ranges = xmalloc(nranges * sizeof(DebugLocRange));
        v->cap_ranges = nranges;
        for (uint32_t i = 0; i < nranges; i++) {
            if (*p + 25 > end) return -1;
            DebugLocRange *r = &v->ranges[i];
            r->pc_start = (size_t)rd64(p);
            r->pc_end = (size_t)rd64(p);
            r->loc_kind = (DebugLocKind)**p; (*p)++;
            r->rbp_offset = (int)rd32(p);
            r->dwarf_reg = (int)rd32(p);
        }
        v->num_ranges = nranges;
    }
    return 0;
}
void debug_serialize(const EmitModule *m, Buffer *out) {
    d_bytes(out, "FDBG", 4);
    d_u32(out, 1);
    ser_str(out, m->dbg_tu_name);
    d_u32(out, (uint32_t)m->num_dbg_lines);
    for (size_t i = 0; i < m->num_dbg_lines; i++) {
        const DebugLineEntry *e = &m->dbg_lines[i];
        ser_str(out, e->file);
        d_u32(out, (uint32_t)e->line);
        d_u32(out, (uint32_t)e->col);
        d_u64(out, (uint64_t)e->pc_off);
    }
    d_u32(out, (uint32_t)m->num_dbg_funcs);
    for (size_t i = 0; i < m->num_dbg_funcs; i++) {
        const DebugFunc *f = &m->dbg_funcs[i];
        ser_str(out, f->name);
        ser_str(out, f->file);
        d_u32(out, (uint32_t)f->line);
        d_u64(out, (uint64_t)f->start_pc);
        d_u64(out, (uint64_t)f->end_pc);
        d_u64(out, (uint64_t)f->prologue_end_pc);
        d_u32(out, (uint32_t)f->num_vars);
        for (size_t j = 0; j < f->num_vars; j++) ser_var(out, &f->vars[j]);
    }
    d_u32(out, (uint32_t)m->num_dbg_globals);
    for (size_t i = 0; i < m->num_dbg_globals; i++)
        ser_var(out, &m->dbg_globals[i]);
}
int debug_deserialize(EmitModule *m, const unsigned char *data, size_t len) {
    const unsigned char *p = data, *end = data + len;
    if (len < 8 || memcmp(p, "FDBG", 4) != 0) return -1;
    p += 4;
    if (rd32(&p) != 1) return -1;
    if (deser_str(&p, end, &m->dbg_tu_name) < 0) return -1;
    if (p + 4 > end) return -1;
    uint32_t nlines = rd32(&p);
    for (uint32_t i = 0; i < nlines; i++) {
        char *file = ((void*)0);
        if (deser_str(&p, end, &file) < 0) return -1;
        if (p + 16 > end) { free(file); return -1; }
        int line = (int)rd32(&p), col = (int)rd32(&p);
        uint64_t pc = rd64(&p);
        emit_module_add_dbg_line(m, file, line, col, (size_t)pc);
        free(file);
    }
    if (p + 4 > end) return -1;
    uint32_t nfuncs = rd32(&p);
    for (uint32_t i = 0; i < nfuncs; i++) {
        char *name = ((void*)0), *file = ((void*)0);
        if (deser_str(&p, end, &name) < 0) return -1;
        if (deser_str(&p, end, &file) < 0) { free(name); return -1; }
        if (p + 4 + 24 + 4 > end) { free(name); free(file); return -1; }
        int line = (int)rd32(&p);
        uint64_t start = rd64(&p), endpc = rd64(&p), prol = rd64(&p);
        uint32_t nvars = rd32(&p);
        int fi = emit_module_add_dbg_func(m, name, file, line, (size_t)start);
        emit_module_dbg_func_end(m, fi, (size_t)endpc, (size_t)prol);
        free(name); free(file);
        for (uint32_t j = 0; j < nvars; j++) {
            DebugVar v;
            if (deser_var(&p, end, &v) < 0) return -1;
            emit_module_add_dbg_var(m, fi, &v);
            debug_var_release(&v);
        }
    }
    if (p + 4 > end) return -1;
    uint32_t nglobs = rd32(&p);
    for (uint32_t i = 0; i < nglobs; i++) {
        DebugVar v;
        if (deser_var(&p, end, &v) < 0) return -1;
        emit_module_add_dbg_global(m, &v);
        debug_var_release(&v);
    }
    return 0;
}
static uint64_t resolve_sym_addr(const EmitModule *m, const char *name,
                                 uint64_t text_base) {
    (void)text_base;
    if (!name) return 0;
    for (size_t i = 0; i < m->num_syms; i++) {
        if (!m->syms[i].name || strcmp(m->syms[i].name, name) != 0) continue;
        if (m->syms[i].shndx == 0) continue;
        return m->syms[i].value;
    }
    return 0;
}
struct TypeDie {
    DebugTypeTag tag;
    int width;
    int is_unsigned;
    int array_len;
    uint32_t offset;
};typedef struct TypeDie TypeDie;
struct TypeDieCache { TypeDie *data; size_t len, cap; };typedef struct TypeDieCache TypeDieCache;
static uint32_t type_die_get(TypeDieCache *c, Buffer *info, DebugTypeTag tag,
                             int width, int is_unsigned, int array_len);
static uint32_t type_die_get(TypeDieCache *c, Buffer *info, DebugTypeTag tag,
                             int width, int is_unsigned, int array_len) {
    for (size_t i = 0; i < c->len; i++) {
        if (c->data[i].tag == tag && c->data[i].width == width &&
            c->data[i].is_unsigned == is_unsigned &&
            c->data[i].array_len == array_len)
            return c->data[i].offset;
    }
    if (c->len >= c->cap) {
        size_t nc = c->cap ? c->cap * 2 : 8;
        c->data = realloc(c->data, nc * sizeof(TypeDie));
        if (!c->data) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        c->cap = nc;
    }
    uint32_t pointee_off = 0, elem_off = 0;
    if (tag == DBG_TY_PTR)
        pointee_off = type_die_get(c, info, DBG_TY_INT, 4, 0, 0);
    else if (tag == DBG_TY_ARRAY) {
        int elem_w = array_len > 0 ? width / array_len : width;
        if (elem_w <= 0) elem_w = 4;
        elem_off = type_die_get(c, info, DBG_TY_INT, elem_w, is_unsigned, 0);
    }
    uint32_t off = (uint32_t)info->len;
    if (tag == DBG_TY_PTR) {
        d_uleb(info, 3);
        d_u32(info, pointee_off);
    } else if (tag == DBG_TY_ARRAY) {
        d_uleb(info, 4);
        d_u32(info, elem_off);
        d_uleb(info, 5);
        d_u32(info, (uint32_t)(array_len > 0 ? array_len : 1));
        d_u8(info, 0);
    } else if (tag == DBG_TY_STRUCT) {
        d_uleb(info, 6);
        d_u32(info, (uint32_t)(width > 0 ? width : 1));
    } else {
        d_uleb(info, 2);
        const char *nm = "int";
        uint8_t enc = 0x05;
        if (tag == DBG_TY_BOOL) { nm = "_Bool"; enc = 0x02; width = 1; }
        else if (tag == DBG_TY_FLOAT) {
            if (width == 4) { nm = "float"; enc = 0x04; }
            else if (width == 16) { nm = "long double"; enc = 0x04; }
            else { nm = "double"; enc = 0x04; width = 8; }
        } else if (tag == DBG_TY_VOID) { nm = "void"; enc = 0x05; width = 1; }
        else if (width == 1) {
            nm = is_unsigned ? "unsigned char" : "signed char";
            enc = is_unsigned ? 0x08 : 0x06;
        } else if (width == 2) {
            nm = is_unsigned ? "unsigned short" : "short";
            enc = is_unsigned ? 0x07 : 0x05;
        } else if (width == 8) {
            nm = is_unsigned ? "unsigned long" : "long";
            enc = is_unsigned ? 0x07 : 0x05;
        } else {
            nm = is_unsigned ? "unsigned int" : "int";
            enc = is_unsigned ? 0x07 : 0x05;
            width = 4;
        }
        d_bytes(info, nm, strlen(nm) + 1);
        d_u8(info, enc);
        d_u8(info, (uint8_t)width);
    }
    TypeDie *td = &c->data[c->len++];
    td->tag = tag; td->width = width; td->is_unsigned = is_unsigned;
    td->array_len = array_len; td->offset = off;
    return off;
}
static uint32_t type_for_var(TypeDieCache *c, Buffer *info, const DebugVar *v) {
    return type_die_get(c, info, v->type_tag, v->width, v->is_unsigned, v->array_len);
}
static void emit_loc_bytes(Buffer *expr, DebugLocKind kind, int rbp_offset,
                           int dwarf_reg) {
    if (kind == DBG_LOC_FBREG) {
        d_u8(expr, 0x91); d_sleb(expr, rbp_offset);
    } else if (kind == DBG_LOC_REG) {
        if (dwarf_reg >= 0 && dwarf_reg <= 31)
            d_u8(expr, (uint8_t)(0x50 + dwarf_reg));
        else { d_u8(expr, 0x90); d_uleb(expr, (uint64_t)dwarf_reg); }
    }
}
static void emit_location_expr(Buffer *b, const DebugVar *v,
                               const EmitModule *m, uint64_t text_base) {
    Buffer expr; buffer_init(&expr);
    if (v->loc_kind == DBG_LOC_ADDR) {
        uint64_t addr = resolve_sym_addr(m, v->sym_name ? v->sym_name : v->name, text_base);
        d_u8(&expr, 0x03); d_u64(&expr, addr);
    } else {
        emit_loc_bytes(&expr, v->loc_kind, v->rbp_offset, v->dwarf_reg);
    }
    d_uleb(b, expr.len);
    d_bytes(b, expr.data, expr.len);
    buffer_free(&expr);
}
static uint32_t emit_loclist(Buffer *loc, const DebugVar *v,
                             uint64_t text_base) {
    uint32_t off = (uint32_t)loc->len;
    d_u64(loc, 0xffffffffffffffffULL);
    d_u64(loc, text_base);
    for (size_t i = 0; i < v->num_ranges; i++) {
        const DebugLocRange *r = &v->ranges[i];
        Buffer expr; buffer_init(&expr);
        emit_loc_bytes(&expr, r->loc_kind, r->rbp_offset, r->dwarf_reg);
        d_u64(loc, (uint64_t)r->pc_start);
        d_u64(loc, (uint64_t)r->pc_end);
        d_u16(loc, (uint16_t)expr.len);
        d_bytes(loc, expr.data, expr.len);
        buffer_free(&expr);
    }
    d_u64(loc, 0);
    d_u64(loc, 0);
    return off;
}
static void emit_abbrevs(Buffer *ab) {
    d_uleb(ab, 1); d_uleb(ab, 0x11); d_u8(ab, 0x01);
    d_uleb(ab, 0x03); d_uleb(ab, 0x0e);
    d_uleb(ab, 0x1b); d_uleb(ab, 0x0e);
    d_uleb(ab, 0x13); d_uleb(ab, 0x0b);
    d_uleb(ab, 0x11); d_uleb(ab, 0x01);
    d_uleb(ab, 0x12); d_uleb(ab, 0x07);
    d_uleb(ab, 0x10); d_uleb(ab, 0x17);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 2); d_uleb(ab, 0x24); d_u8(ab, 0x00);
    d_uleb(ab, 0x03); d_uleb(ab, 0x08);
    d_uleb(ab, 0x3e); d_uleb(ab, 0x0b);
    d_uleb(ab, 0x0b); d_uleb(ab, 0x0b);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 3); d_uleb(ab, 0x0f); d_u8(ab, 0x00);
    d_uleb(ab, 0x49); d_uleb(ab, 0x13);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 4); d_uleb(ab, 0x01); d_u8(ab, 0x01);
    d_uleb(ab, 0x49); d_uleb(ab, 0x13);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 5); d_uleb(ab, 0x21); d_u8(ab, 0x00);
    d_uleb(ab, 0x37); d_uleb(ab, 0x06);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 6); d_uleb(ab, 0x13); d_u8(ab, 0x00);
    d_uleb(ab, 0x0b); d_uleb(ab, 0x06);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 7); d_uleb(ab, 0x2e); d_u8(ab, 0x01);
    d_uleb(ab, 0x03); d_uleb(ab, 0x0e);
    d_uleb(ab, 0x11); d_uleb(ab, 0x01);
    d_uleb(ab, 0x12); d_uleb(ab, 0x07);
    d_uleb(ab, 0x40); d_uleb(ab, 0x18);
    d_uleb(ab, 0x3f); d_uleb(ab, 0x19);
    d_uleb(ab, 0x27); d_uleb(ab, 0x19);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 8); d_uleb(ab, 0x05); d_u8(ab, 0x00);
    d_uleb(ab, 0x03); d_uleb(ab, 0x0e);
    d_uleb(ab, 0x49); d_uleb(ab, 0x13);
    d_uleb(ab, 0x02); d_uleb(ab, 0x18);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 9); d_uleb(ab, 0x34); d_u8(ab, 0x00);
    d_uleb(ab, 0x03); d_uleb(ab, 0x0e);
    d_uleb(ab, 0x49); d_uleb(ab, 0x13);
    d_uleb(ab, 0x02); d_uleb(ab, 0x18);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 10); d_uleb(ab, 0x34); d_u8(ab, 0x00);
    d_uleb(ab, 0x03); d_uleb(ab, 0x0e);
    d_uleb(ab, 0x49); d_uleb(ab, 0x13);
    d_uleb(ab, 0x02); d_uleb(ab, 0x18);
    d_uleb(ab, 0x3f); d_uleb(ab, 0x19);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 11); d_uleb(ab, 0x05); d_u8(ab, 0x00);
    d_uleb(ab, 0x03); d_uleb(ab, 0x0e);
    d_uleb(ab, 0x49); d_uleb(ab, 0x13);
    d_uleb(ab, 0x02); d_uleb(ab, 0x17);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 12); d_uleb(ab, 0x34); d_u8(ab, 0x00);
    d_uleb(ab, 0x03); d_uleb(ab, 0x0e);
    d_uleb(ab, 0x49); d_uleb(ab, 0x13);
    d_uleb(ab, 0x02); d_uleb(ab, 0x17);
    d_uleb(ab, 0); d_uleb(ab, 0);
    d_uleb(ab, 0);
}
static void emit_debug_line(const EmitModule *m, uint64_t text_base, Buffer *line) {
    const char **files = ((void*)0);
    size_t nfiles = 0, cfiles = 0;
    for (size_t i = 0; i < m->num_dbg_lines; i++) {
        const char *f = m->dbg_lines[i].file;
        if (!f) continue;
        int found = 0;
        for (size_t j = 0; j < nfiles; j++)
            if (strcmp(files[j], f) == 0) { found = 1; break; }
        if (!found) {
            if (nfiles >= cfiles) { cfiles = cfiles ? cfiles * 2 : 4; files = realloc(files, cfiles * sizeof(char *)); }
            files[nfiles++] = f;
        }
    }
    if (m->dbg_tu_name) {
        int found = 0;
        for (size_t j = 0; j < nfiles; j++)
            if (strcmp(files[j], m->dbg_tu_name) == 0) { found = 1; break; }
        if (!found) {
            if (nfiles >= cfiles) { cfiles = cfiles ? cfiles * 2 : 4; files = realloc(files, cfiles * sizeof(char *)); }
            files[nfiles++] = m->dbg_tu_name;
        }
    }
    if (nfiles == 0) {
        files = realloc(files, sizeof(char *));
        files[0] = m->dbg_tu_name ? m->dbg_tu_name : "unknown.c";
        nfiles = 1;
    }
    Buffer hdr; buffer_init(&hdr);
    d_u32(&hdr, 0);
    d_u16(&hdr, 4);
    d_u32(&hdr, 0);
    size_t header_length_off = hdr.len - 4;
    d_u8(&hdr, 1); d_u8(&hdr, 1); d_u8(&hdr, 1); d_u8(&hdr, 1); d_u8(&hdr, 1);
    d_u8(&hdr, 13);
    d_u8(&hdr, 0); d_u8(&hdr, 1); d_u8(&hdr, 1); d_u8(&hdr, 1);
    d_u8(&hdr, 1); d_u8(&hdr, 0); d_u8(&hdr, 0); d_u8(&hdr, 0);
    d_u8(&hdr, 1); d_u8(&hdr, 0); d_u8(&hdr, 0); d_u8(&hdr, 1);
    d_u8(&hdr, 0);
    for (size_t i = 0; i < nfiles; i++) {
        d_bytes(&hdr, files[i], strlen(files[i]) + 1);
        d_uleb(&hdr, 0); d_uleb(&hdr, 0); d_uleb(&hdr, 0);
    }
    d_u8(&hdr, 0);
    uint32_t header_length = (uint32_t)(hdr.len - (header_length_off + 4));
    memcpy(hdr.data + header_length_off, &header_length, 4);
    Buffer prog; buffer_init(&prog);
    int cur_file = 1, cur_line = 1, have_set_addr = 0;
    uint64_t cur_addr = text_base;
    for (size_t i = 0; i < m->num_dbg_lines; i++) {
        const DebugLineEntry *e = &m->dbg_lines[i];
        uint64_t addr = text_base + e->pc_off;
        int file_idx = 1;
        for (size_t j = 0; j < nfiles; j++)
            if (e->file && strcmp(files[j], e->file) == 0) { file_idx = (int)j + 1; break; }
        if (!have_set_addr || addr < cur_addr) {
            d_u8(&prog, 0); d_uleb(&prog, 9); d_u8(&prog, 2); d_u64(&prog, addr);
            cur_addr = addr; have_set_addr = 1;
        } else if (addr > cur_addr) {
            d_u8(&prog, 2); d_uleb(&prog, addr - cur_addr); cur_addr = addr;
        }
        if (file_idx != cur_file) { d_u8(&prog, 4); d_uleb(&prog, (uint64_t)file_idx); cur_file = file_idx; }
        if (e->line != cur_line) { d_u8(&prog, 3); d_sleb(&prog, (int64_t)e->line - cur_line); cur_line = e->line; }
        if (e->col > 0) { d_u8(&prog, 5); d_uleb(&prog, (uint64_t)e->col); }
        for (size_t k = 0; k < m->num_dbg_funcs; k++) {
            if (m->dbg_funcs[k].prologue_end_pc == e->pc_off &&
                m->dbg_funcs[k].end_pc > m->dbg_funcs[k].start_pc) {
                d_u8(&prog, 10);
                break;
            }
        }
        d_u8(&prog, 1);
    }
    if (m->num_dbg_funcs > 0) {
        size_t max_end = 0;
        for (size_t i = 0; i < m->num_dbg_funcs; i++)
            if (m->dbg_funcs[i].end_pc > max_end) max_end = m->dbg_funcs[i].end_pc;
        uint64_t end_addr = text_base + max_end;
        if (end_addr > cur_addr) { d_u8(&prog, 2); d_uleb(&prog, end_addr - cur_addr); }
    }
    d_u8(&prog, 0); d_uleb(&prog, 1); d_u8(&prog, 1);
    uint32_t unit_length = (uint32_t)(hdr.len - 4 + prog.len);
    memcpy(hdr.data, &unit_length, 4);
    d_bytes(line, hdr.data, hdr.len);
    d_bytes(line, prog.data, prog.len);
    buffer_free(&hdr); buffer_free(&prog); free(files);
}
static void cfa_advance(Buffer *frame, size_t delta) {
    if (delta == 0) return;
    if (delta <= 63) {
        d_u8(frame, (uint8_t)(0x40 | (unsigned)delta));
    } else if (delta <= 0xff) {
        d_u8(frame, 0x02); d_u8(frame, (uint8_t)delta);
    } else if (delta <= 0xffff) {
        d_u8(frame, 0x03); d_u16(frame, (uint16_t)delta);
    } else {
        d_u8(frame, 0x04); d_u32(frame, (uint32_t)delta);
    }
}
static void emit_debug_frame(const EmitModule *m, uint64_t text_base, Buffer *frame) {
    size_t cie_start = frame->len;
    d_u32(frame, 0);
    d_u32(frame, 0xffffffff);
    d_u8(frame, 1);
    d_u8(frame, 0);
    d_uleb(frame, 1);
    d_sleb(frame, -8);
    d_uleb(frame, 16);
    d_u8(frame, 0x0c); d_uleb(frame, 7); d_uleb(frame, 8);
    d_u8(frame, (uint8_t)(0x80 | 16)); d_uleb(frame, 1);
    while ((frame->len - cie_start) & 7) d_u8(frame, 0x00);
    uint32_t cie_len = (uint32_t)(frame->len - cie_start - 4);
    memcpy(frame->data + cie_start, &cie_len, 4);
    for (size_t i = 0; i < m->num_dbg_funcs; i++) {
        const DebugFunc *f = &m->dbg_funcs[i];
        size_t fde_start = frame->len;
        d_u32(frame, 0);
        d_u32(frame, (uint32_t)cie_start);
        d_u64(frame, text_base + f->start_pc);
        d_u64(frame, f->end_pc > f->start_pc ? f->end_pc - f->start_pc : 0);
        size_t push_end = f->after_push_rbp_pc > f->start_pc
                          ? f->after_push_rbp_pc - f->start_pc : 1;
        size_t mov_end = f->after_mov_rbp_pc > f->after_push_rbp_pc
                         ? f->after_mov_rbp_pc - f->after_push_rbp_pc : 3;
        cfa_advance(frame, push_end);
        d_u8(frame, 0x0e); d_uleb(frame, 16);
        d_u8(frame, (uint8_t)(0x80 | 6)); d_uleb(frame, 2);
        cfa_advance(frame, mov_end);
        d_u8(frame, 0x0d); d_uleb(frame, 6);
        while ((frame->len - fde_start) & 7) d_u8(frame, 0x00);
        uint32_t fde_len = (uint32_t)(frame->len - fde_start - 4);
        memcpy(frame->data + fde_start, &fde_len, 4);
    }
}
void debug_emit_dwarf(const EmitModule *m, uint64_t text_base_vaddr,
                      Buffer *debug_abbrev, Buffer *debug_info,
                      Buffer *debug_str, Buffer *debug_line,
                      Buffer *debug_frame, Buffer *debug_loc) {
    emit_abbrevs(debug_abbrev);
    Strtab strs; strtab_init(&strs);
    uint32_t tu_name = strtab_add(&strs, m->dbg_tu_name ? m->dbg_tu_name : "unknown.c");
    uint32_t comp_dir = strtab_add(&strs, ".");
    uint64_t low_pc = text_base_vaddr;
    uint64_t high_len = m->text.len;
    if (m->num_dbg_funcs > 0) {
        size_t max_end = 0, min_start = m->dbg_funcs[0].start_pc;
        for (size_t i = 0; i < m->num_dbg_funcs; i++) {
            if (m->dbg_funcs[i].start_pc < min_start) min_start = m->dbg_funcs[i].start_pc;
            if (m->dbg_funcs[i].end_pc > max_end) max_end = m->dbg_funcs[i].end_pc;
        }
        low_pc = text_base_vaddr + min_start;
        high_len = max_end > min_start ? max_end - min_start : m->text.len;
    }
    size_t info_len_off = debug_info->len;
    d_u32(debug_info, 0);
    d_u16(debug_info, 4);
    d_u32(debug_info, 0);
    d_u8(debug_info, 8);
    TypeDieCache tcache; memset(&tcache, 0, sizeof(tcache));
    d_uleb(debug_info, 1);
    d_u32(debug_info, tu_name);
    d_u32(debug_info, comp_dir);
    d_u8(debug_info, 0x0c);
    d_u64(debug_info, low_pc);
    d_u64(debug_info, high_len);
    d_u32(debug_info, 0);
    (void)type_die_get(&tcache, debug_info, DBG_TY_INT, 4, 0, 0);
    for (size_t gi = 0; gi < m->num_dbg_globals; gi++) {
        const DebugVar *v = &m->dbg_globals[gi];
        uint32_t ty = type_for_var(&tcache, debug_info, v);
        uint32_t nm = strtab_add(&strs, v->name ? v->name : "");
        d_uleb(debug_info, 10);
        d_u32(debug_info, nm);
        d_u32(debug_info, ty);
        emit_location_expr(debug_info, v, m, text_base_vaddr);
    }
    for (size_t fi = 0; fi < m->num_dbg_funcs; fi++) {
        const DebugFunc *f = &m->dbg_funcs[fi];
        uint32_t nm = strtab_add(&strs, f->name ? f->name : "");
        d_uleb(debug_info, 7);
        d_u32(debug_info, nm);
        d_u64(debug_info, text_base_vaddr + f->start_pc);
        d_u64(debug_info, f->end_pc >= f->start_pc ? f->end_pc - f->start_pc : 0);
        { Buffer expr; buffer_init(&expr); d_u8(&expr, 0x50 + 6);
          d_uleb(debug_info, expr.len); d_bytes(debug_info, expr.data, expr.len); buffer_free(&expr); }
        for (size_t vi = 0; vi < f->num_vars; vi++) {
            const DebugVar *v = &f->vars[vi];
            uint32_t ty = type_for_var(&tcache, debug_info, v);
            uint32_t vnm = strtab_add(&strs, v->name ? v->name : "");
            int is_param = v->kind == DBG_VAR_PARAM;
            if (v->num_ranges > 0) {
                uint32_t loc_off = emit_loclist(debug_loc, v, text_base_vaddr);
                d_uleb(debug_info, is_param ? 11 : 12);
                d_u32(debug_info, vnm);
                d_u32(debug_info, ty);
                d_u32(debug_info, loc_off);
            } else {
                d_uleb(debug_info, is_param ? 8 : 9);
                d_u32(debug_info, vnm);
                d_u32(debug_info, ty);
                emit_location_expr(debug_info, v, m, text_base_vaddr);
            }
        }
        d_u8(debug_info, 0);
    }
    d_u8(debug_info, 0);
    uint32_t info_len = (uint32_t)(debug_info->len - info_len_off - 4);
    memcpy(debug_info->data + info_len_off, &info_len, 4);
    d_bytes(debug_str, strs.buf.data, strs.buf.len);
    strtab_free(&strs);
    free(tcache.data);
    emit_debug_line(m, text_base_vaddr, debug_line);
    emit_debug_frame(m, text_base_vaddr, debug_frame);
}
