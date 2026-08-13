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
void emit_link(EmitModule **mods, size_t n, const char *path);
void emit_elf(const EmitModule *m, const char *path);
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
extern int chmod(const char *p, int mode);
void emit_module_init(EmitModule *m) {
    buffer_init(&m->text);
    buffer_init(&m->rodata);
    buffer_init(&m->data);
    m->bss_size = 0;
    m->syms = ((void*)0); m->num_syms = 0; m->cap_syms = 0;
    m->relocs = ((void*)0); m->num_relocs = 0; m->cap_relocs = 0;
    m->data_relocs = ((void*)0); m->num_data_relocs = 0; m->cap_data_relocs = 0;
}
void emit_module_free(EmitModule *m) {
    for (size_t i = 0; i < m->num_syms; i++) free(m->syms[i].name);
    free(m->syms);
    free(m->relocs);
    free(m->data_relocs);
    buffer_free(&m->text);
    buffer_free(&m->rodata);
    buffer_free(&m->data);
}
int emit_module_add_symbol(EmitModule *m, const char *name, uint8_t binding,
                           uint8_t type, uint16_t shndx, size_t value,
                           size_t size) {
    if (m->num_syms >= m->cap_syms) {
        size_t nc = m->cap_syms ? m->cap_syms * 2 : 8;
        m->syms = realloc(m->syms, nc * sizeof(EmitSymbol));
        if (!m->syms) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->cap_syms = nc;
    }
    EmitSymbol *s = &m->syms[m->num_syms];
    s->name = name ? xstrdup(name) : ((void*)0);
    s->binding = binding;
    s->type = type;
    s->shndx = shndx;
    s->value = value;
    s->size = size;
    return (int)m->num_syms++;
}
int emit_module_find_symbol(EmitModule *m, const char *name) {
    if (!name) return -1;
    for (size_t i = 0; i < m->num_syms; i++) {
        if (m->syms[i].name && strcmp(m->syms[i].name, name) == 0)
            return (int)i;
    }
    return -1;
}
int emit_module_add_undefined(EmitModule *m, const char *name) {
    for (size_t i = 0; i < m->num_syms; i++) {
        if (m->syms[i].name && strcmp(m->syms[i].name, name) == 0 &&
            m->syms[i].shndx == 0)
            return (int)i;
    }
    return emit_module_add_symbol(m, name, 1 , 0 ,
                                  0, 0, 0);
}
void emit_module_add_reloc(EmitModule *m, size_t offset, uint32_t type,
                           int sym, int32_t addend) {
    if (m->num_relocs >= m->cap_relocs) {
        size_t nc = m->cap_relocs ? m->cap_relocs * 2 : 8;
        m->relocs = realloc(m->relocs, nc * sizeof(EmitReloc));
        if (!m->relocs) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->cap_relocs = nc;
    }
    EmitReloc *r = &m->relocs[m->num_relocs++];
    r->offset = offset;
    r->type = type;
    r->sym = (uint32_t)sym;
    r->addend = addend;
}
void emit_module_add_data_reloc(EmitModule *m, size_t offset, uint32_t type,
                                int sym, int32_t addend) {
    if (m->num_data_relocs >= m->cap_data_relocs) {
        size_t nc = m->cap_data_relocs ? m->cap_data_relocs * 2 : 8;
        m->data_relocs = realloc(m->data_relocs, nc * sizeof(EmitReloc));
        if (!m->data_relocs) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->cap_data_relocs = nc;
    }
    EmitReloc *r = &m->data_relocs[m->num_data_relocs++];
    r->offset = offset;
    r->type = type;
    r->sym = (uint32_t)sym;
    r->addend = addend;
}
static void buf_u8(Buffer *b, uint8_t v) { buffer_append(b, (const char *)&v, 1); }
static void buf_bytes(Buffer *b, const char *d, size_t n) { buffer_append(b, d, n); }
static void buf_u16(Buffer *b, uint16_t v) { buffer_append(b, (const char *)&v, 2); }
static void buf_u32(Buffer *b, uint32_t v) { buffer_append(b, (const char *)&v, 4); }
static void buf_u64(Buffer *b, uint64_t v) { buffer_append(b, (const char *)&v, 8); }
static void buf_pad(Buffer *b, size_t n) { while (n--) buf_u8(b, 0); }
static void write_ehdr(Buffer *b, uint16_t type, uint16_t shnum) {
    buffer_append(b, "\x7f" "ELF", 4);
    char ident[12];
    memset(ident, 0, 12);
    ident[0] = 2;
    ident[1] = 1;
    ident[2] = 1;
    ident[3] = 0;
    buffer_append(b, ident, 12);
    buf_u16(b, type);
    buf_u16(b, 62);
    buf_u32(b, 1);
    buf_u64(b, 0);
    buf_u64(b, 0);
    buf_u64(b, 0);
    buf_u32(b, 0);
    buf_u16(b, 64);
    buf_u16(b, 56);
    buf_u16(b, 0);
    buf_u16(b, 64);
    buf_u16(b, shnum);
    buf_u16(b, 0);
}
static void write_shdr(Buffer *b, uint32_t name, uint32_t type, uint64_t flags,
                       uint64_t addr, uint64_t offset, uint64_t size,
                       uint32_t link, uint32_t info, uint64_t addralign,
                       uint64_t entsize) {
    buf_u32(b, name);
    buf_u32(b, type);
    buf_u64(b, flags);
    buf_u64(b, addr);
    buf_u64(b, offset);
    buf_u64(b, size);
    buf_u32(b, link);
    buf_u32(b, info);
    buf_u64(b, addralign);
    buf_u64(b, entsize);
}
void emit_obj(const EmitModule *m, const char *path) {
    Buffer shstrtab;
    buffer_init(&shstrtab);
    buf_u8(&shstrtab, 0);
    uint32_t shname_text = (uint32_t)shstrtab.len;
    buf_bytes(&shstrtab, ".text", sizeof(".text"));
    uint32_t shname_rodata = (uint32_t)shstrtab.len;
    buf_bytes(&shstrtab, ".rodata", sizeof(".rodata"));
    uint32_t shname_data = (uint32_t)shstrtab.len;
    buf_bytes(&shstrtab, ".data", sizeof(".data"));
    uint32_t shname_bss = (uint32_t)shstrtab.len;
    buf_bytes(&shstrtab, ".bss", sizeof(".bss"));
    uint32_t shname_symtab = (uint32_t)shstrtab.len;
    buf_bytes(&shstrtab, ".symtab", sizeof(".symtab"));
    uint32_t shname_strtab = (uint32_t)shstrtab.len;
    buf_bytes(&shstrtab, ".strtab", sizeof(".strtab"));
    uint32_t shname_rela_text = (uint32_t)shstrtab.len;
    buf_bytes(&shstrtab, ".rela.text", sizeof(".rela.text"));
    uint32_t shname_rela_data = (uint32_t)shstrtab.len;
    buf_bytes(&shstrtab, ".rela.data", sizeof(".rela.data"));
    uint32_t shname_shstrtab = (uint32_t)shstrtab.len;
    buf_bytes(&shstrtab, ".shstrtab", sizeof(".shstrtab"));
    Buffer strtab;
    buffer_init(&strtab);
    buf_u8(&strtab, 0);
    Buffer symtab;
    buffer_init(&symtab);
    buf_pad(&symtab, 24);
    uint32_t sym_text = (uint32_t)(symtab.len / 24);
    buf_pad(&symtab, 24);
    uint32_t sym_rodata = (uint32_t)(symtab.len / 24);
    buf_pad(&symtab, 24);
    uint32_t sym_data = (uint32_t)(symtab.len / 24);
    buf_pad(&symtab, 24);
    uint32_t sym_bss = (uint32_t)(symtab.len / 24);
    buf_pad(&symtab, 24);
    int *sym_remap = malloc(m->num_syms * sizeof(int));
    if (!sym_remap) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    for (size_t i = 0; i < m->num_syms; i++) sym_remap[i] = -1;
    for (size_t i = 0; i < m->num_syms; i++) {
        const EmitSymbol *s = &m->syms[i];
        if (s->binding != 0) continue;
        sym_remap[i] = (int)(symtab.len / 24);
        uint32_t name_off = (uint32_t)strtab.len;
        if (s->name) buf_bytes(&strtab, s->name, strlen(s->name) + 1);
        else buf_u8(&strtab, 0);
        buf_u32(&symtab, name_off);
        buf_u8(&symtab, (s->binding << 4) | (s->type & 0xf));
        buf_u8(&symtab, 0);
        buf_u16(&symtab, s->shndx);
        buf_u64(&symtab, s->value);
        buf_u64(&symtab, s->size);
    }
    unsigned first_global = (unsigned)(symtab.len / 24);
    for (size_t i = 0; i < m->num_syms; i++) {
        const EmitSymbol *s = &m->syms[i];
        if (s->binding == 0) continue;
        sym_remap[i] = (int)(symtab.len / 24);
        uint32_t name_off = (uint32_t)strtab.len;
        if (s->name) buf_bytes(&strtab, s->name, strlen(s->name) + 1);
        else buf_u8(&strtab, 0);
        buf_u32(&symtab, name_off);
        buf_u8(&symtab, (s->binding << 4) | (s->type & 0xf));
        buf_u8(&symtab, 0);
        buf_u16(&symtab, s->shndx);
        buf_u64(&symtab, s->value);
        buf_u64(&symtab, s->size);
    }
    uint8_t sec_info = (0 << 4) | 3;
    memcpy(symtab.data + sym_text * 24 + 4, &sec_info, 1);
    memcpy(symtab.data + sym_rodata * 24 + 4, &sec_info, 1);
    memcpy(symtab.data + sym_data * 24 + 4, &sec_info, 1);
    memcpy(symtab.data + sym_bss * 24 + 4, &sec_info, 1);
    Buffer rela_text;
    buffer_init(&rela_text);
    for (size_t i = 0; i < m->num_relocs; i++) {
        const EmitReloc *r = &m->relocs[i];
        int old_sym = r->sym;
        int new_sym = (old_sym >= 0 && old_sym < (int)m->num_syms)
                      ? sym_remap[old_sym] : old_sym;
        if (new_sym < 0) new_sym = old_sym;
        buf_u64(&rela_text, r->offset);
        buf_u64(&rela_text, ((uint64_t)new_sym << 32) | r->type);
        buf_u64(&rela_text, (uint64_t)(int64_t)r->addend);
    }
    Buffer rela_data;
    buffer_init(&rela_data);
    for (size_t i = 0; i < m->num_data_relocs; i++) {
        const EmitReloc *r = &m->data_relocs[i];
        int old_sym = r->sym;
        int new_sym = (old_sym >= 0 && old_sym < (int)m->num_syms)
                      ? sym_remap[old_sym] : old_sym;
        if (new_sym < 0) new_sym = old_sym;
        buf_u64(&rela_data, r->offset);
        buf_u64(&rela_data, ((uint64_t)new_sym << 32) | r->type);
        buf_u64(&rela_data, (uint64_t)(int64_t)r->addend);
    }
    size_t hdr_size = 64;
    Buffer body;
    buffer_init(&body);
    size_t off_text = body.len;
    buf_bytes(&body, m->text.data, m->text.len);
    size_t off_rodata = body.len;
    buf_bytes(&body, m->rodata.data, m->rodata.len);
    size_t off_data = body.len;
    buf_bytes(&body, m->data.data, m->data.len);
    size_t off_symtab = body.len;
    buf_bytes(&body, symtab.data, symtab.len);
    size_t off_strtab = body.len;
    buf_bytes(&body, strtab.data, strtab.len);
    size_t off_shstrtab = body.len;
    buf_bytes(&body, shstrtab.data, shstrtab.len);
    size_t off_rela_text = body.len;
    buf_bytes(&body, rela_text.data, rela_text.len);
    size_t off_rela_data = body.len;
    buf_bytes(&body, rela_data.data, rela_data.len);
    size_t shoff = hdr_size + body.len;
    unsigned shnum = 10;
    unsigned shstrndx = 7;
    Buffer elf;
    buffer_init(&elf);
    write_ehdr(&elf, 1, shnum);
    uint64_t shoff_val = shoff;
    memcpy(elf.data + 40, &shoff_val, 8);
    uint16_t shnum_val = shnum;
    memcpy(elf.data + 60, &shnum_val, 2);
    uint16_t shstrndx_val = shstrndx;
    memcpy(elf.data + 62, &shstrndx_val, 2);
    buf_bytes(&elf, body.data, body.len);
    buf_pad(&elf, 64);
    write_shdr(&elf, shname_text, 1, 0x2 | 0x4,
               0, hdr_size + off_text, m->text.len, 0, 0, 16, 0);
    write_shdr(&elf, shname_rodata, 1, 0x2,
               0, hdr_size + off_rodata, m->rodata.len, 0, 0, 8, 0);
    write_shdr(&elf, shname_data, 1, 0x2 | 0x1,
               0, hdr_size + off_data, m->data.len, 0, 0, 8, 0);
    write_shdr(&elf, shname_bss, 8, 0x2 | 0x1,
               0, hdr_size + off_data + m->data.len, m->bss_size, 0, 0, 8, 0);
    unsigned symtab_idx = 5;
    unsigned strtab_idx = 6;
    write_shdr(&elf, shname_symtab, 2, 0,
               0, hdr_size + off_symtab, symtab.len, strtab_idx, first_global, 8,
               24);
    write_shdr(&elf, shname_strtab, 3, 0,
               0, hdr_size + off_strtab, strtab.len, 0, 0, 1, 0);
    write_shdr(&elf, shname_shstrtab, 3, 0,
               0, hdr_size + off_shstrtab, shstrtab.len, 0, 0, 1, 0);
    write_shdr(&elf, shname_rela_text, 4, 0,
               0, hdr_size + off_rela_text, rela_text.len, symtab_idx,
               1 , 8, 24);
    write_shdr(&elf, shname_rela_data, 4, 0,
               0, hdr_size + off_rela_data, rela_data.len, symtab_idx,
               3 , 8, 24);
    (void)shstrndx; (void)symtab_idx; (void)strtab_idx; (void)first_global;
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "fakecc: cannot write '%s'\n", path); exit(1); }
    fwrite(elf.data, 1, elf.len, f);
    fclose(f);
    free(sym_remap);
    buffer_free(&shstrtab);
    buffer_free(&strtab);
    buffer_free(&symtab);
    buffer_free(&rela_text);
    buffer_free(&rela_data);
    buffer_free(&body);
    buffer_free(&elf);
}
void emit_elf(const EmitModule *m, const char *path) {
    EmitModule *arr = (EmitModule *)m;
    emit_link(&arr, 1, path);
}
static uint64_t rd_u64(const unsigned char *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)p[i] << (8 * i);
    return v;
}
static uint32_t rd_u32(const unsigned char *p) {
    return (uint32_t)rd_u64(p);
}
static uint16_t rd_u16(const unsigned char *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}
int emit_obj_read(const char *path, EmitModule *m) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "fakecc: cannot open '%s'\n", path); return -1; }
    fseek(f, 0, 2);
    long fsize = ftell(f);
    fseek(f, 0, 0);
    unsigned char *buf = malloc((size_t)fsize);
    size_t nread = fread(buf, 1, (size_t)fsize, f);
    fclose(f);
    if (nread != (size_t)fsize) {
        fprintf(stderr, "fakecc: short read on '%s'\n", path); free(buf); return -1;
    }
    if (fsize < 64 || buf[0] != 0x7f || buf[1] != 'E' || buf[2] != 'L' || buf[3] != 'F') {
        fprintf(stderr, "fakecc: '%s' is not an ELF object\n", path); free(buf); return -1;
    }
    if (rd_u16(buf + 16) != 1) {
        fprintf(stderr, "fakecc: '%s' is not a relocatable object\n", path); free(buf); return -1;
    }
    uint64_t shoff = rd_u64(buf + 40);
    uint16_t shentsize = rd_u16(buf + 58);
    uint16_t shnum = rd_u16(buf + 60);
    uint16_t shstrndx = rd_u16(buf + 62);
    emit_module_init(m);
    const unsigned char *shstr_sh = buf + shoff + (size_t)shstrndx * shentsize;
    uint64_t shstr_off = rd_u64(shstr_sh + 24);
    const char *shstr = (const char *)buf + shstr_off;
    int symtab_idx = -1, rela_text_idx = -1, rela_data_idx = -1, strtab_idx = -1;
    int text_idx = -1, rodata_idx = -1, data_idx = -1, bss_idx = -1;
    for (int s = 0; s < shnum; s++) {
        const unsigned char *sh = buf + shoff + (size_t)s * shentsize;
        uint32_t name_idx = rd_u32(sh);
        const char *sname = shstr + name_idx;
        uint32_t type = rd_u32(sh + 4);
        if (strcmp(sname, ".text") == 0 && type == 1) text_idx = s;
        else if (strcmp(sname, ".rodata") == 0 && type == 1) rodata_idx = s;
        else if (strcmp(sname, ".data") == 0 && type == 1) data_idx = s;
        else if (strcmp(sname, ".bss") == 0 && type == 8) bss_idx = s;
        else if (strcmp(sname, ".symtab") == 0 && type == 2) symtab_idx = s;
        else if (strcmp(sname, ".strtab") == 0 && type == 3) strtab_idx = s;
        else if (strcmp(sname, ".rela.text") == 0 && type == 4) rela_text_idx = s;
        else if (strcmp(sname, ".rela.data") == 0 && type == 4) rela_data_idx = s;
    }
    const unsigned char *strtab_data = ((void*)0);
    size_t strtab_len = 0;
    if (strtab_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)strtab_idx * shentsize;
        uint64_t stoff = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        strtab_data = buf + stoff;
        strtab_len = (size_t)sz;
    }
    if (text_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)text_idx * shentsize;
        uint64_t off = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        m->text.data = malloc(sz); memcpy(m->text.data, buf + off, sz); m->text.len = sz; m->text.cap = sz;
    }
    if (rodata_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)rodata_idx * shentsize;
        uint64_t off = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        m->rodata.data = malloc(sz); memcpy(m->rodata.data, buf + off, sz); m->rodata.len = sz; m->rodata.cap = sz;
    }
    if (data_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)data_idx * shentsize;
        uint64_t off = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        m->data.data = malloc(sz); memcpy(m->data.data, buf + off, sz); m->data.len = sz; m->data.cap = sz;
    }
    if (bss_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)bss_idx * shentsize;
        m->bss_size = (size_t)rd_u64(sh + 32);
    }
    if (symtab_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)symtab_idx * shentsize;
        uint64_t off = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        uint32_t entsize = rd_u32(sh + 56);
        if (entsize == 0) entsize = 24;
        size_t count = sz / entsize;
        for (size_t i = 0; i < count; i++) {
            const unsigned char *sym = buf + off + i * entsize;
            uint32_t name_idx = rd_u32(sym);
            uint8_t info = sym[4];
            uint8_t other = sym[5];
            uint16_t shndx = rd_u16(sym + 6);
            uint64_t value = rd_u64(sym + 8);
            uint64_t size = rd_u64(sym + 16);
            (void)other;
            const char *name = ((void*)0);
            if (name_idx != 0 && strtab_data != ((void*)0) &&
                (size_t)name_idx < strtab_len) {
                name = (const char *)strtab_data + name_idx;
            }
            emit_module_add_symbol(m, name,
                                   (uint8_t)(info >> 4), (uint8_t)(info & 0xf),
                                   shndx, (size_t)value, (size_t)size);
        }
    }
    if (rela_text_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)rela_text_idx * shentsize;
        uint64_t off = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        uint32_t entsize = rd_u32(sh + 56);
        if (entsize == 0) entsize = 24;
        size_t count = sz / entsize;
        for (size_t i = 0; i < count; i++) {
            const unsigned char *r = buf + off + i * entsize;
            uint64_t roff = rd_u64(r);
            uint64_t rinfo = rd_u64(r + 8);
            uint64_t raddend = rd_u64(r + 16);
            uint32_t sym = (uint32_t)(rinfo >> 32);
            uint32_t type = (uint32_t)(rinfo & 0xffffffff);
            emit_module_add_reloc(m, (size_t)roff, type, (int)sym, (int32_t)raddend);
        }
    }
    if (rela_data_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)rela_data_idx * shentsize;
        uint64_t off = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        uint32_t entsize = rd_u32(sh + 56);
        if (entsize == 0) entsize = 24;
        size_t count = sz / entsize;
        for (size_t i = 0; i < count; i++) {
            const unsigned char *r = buf + off + i * entsize;
            uint64_t roff = rd_u64(r);
            uint64_t rinfo = rd_u64(r + 8);
            uint64_t raddend = rd_u64(r + 16);
            uint32_t sym = (uint32_t)(rinfo >> 32);
            uint32_t type = (uint32_t)(rinfo & 0xffffffff);
            emit_module_add_data_reloc(m, (size_t)roff, type, (int)sym, (int32_t)raddend);
        }
    }
    free(buf);
    return 0;
}
