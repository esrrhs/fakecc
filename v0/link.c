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
static const char INTERP_PATH[] = "/lib64/ld-linux-x86-64.so.2";
struct SymInfo { int defined; int shndx; size_t value; uint8_t binding; };
typedef struct SymInfo SymInfo;
static void emit_byte(Buffer *b, uint8_t v) { buffer_append(b, (const char *)&v, 1); }
static void emit_int32(Buffer *b, int32_t v) { buffer_append(b, (const char *)&v, 4); }
static void *xcalloc(size_t nmemb, size_t size) {
    void *p = calloc(nmemb, size);
    if (!p) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    return p;
}
static void buf_u8(Buffer *b, uint8_t v) { buffer_append(b, (const char *)&v, 1); }
static void buf_pad(Buffer *b, size_t n) { while (n--) buf_u8(b, 0); }
static void buf_bytes(Buffer *b, const char *d, size_t n) { buffer_append(b, d, n); }
static void buf_u16(Buffer *b, uint16_t v) { buffer_append(b, (const char *)&v, 2); }
static void buf_u32(Buffer *b, uint32_t v) { buffer_append(b, (const char *)&v, 4); }
static void buf_u64(Buffer *b, uint64_t v) { buffer_append(b, (const char *)&v, 8); }
static void write_ehdr(Buffer *b, uint64_t entry, uint64_t phoff,
                       uint16_t phnum) {
    buffer_append(b, "\x7f" "ELF", 4);
    char ident[12];
    memset(ident, 0, 12);
    ident[0] = 2;
    ident[1] = 1;
    ident[2] = 1;
    ident[3] = 0;
    buffer_append(b, ident, 12);
    buf_u16(b, 2);
    buf_u16(b, 62);
    buf_u32(b, 1);
    buf_u64(b, entry);
    buf_u64(b, phoff);
    buf_u64(b, 0);
    buf_u32(b, 0);
    buf_u16(b, 64);
    buf_u16(b, 56);
    buf_u16(b, phnum);
    buf_u16(b, 64);
    buf_u16(b, 0);
    buf_u16(b, 0);
}
static void write_phdr(Buffer *b, uint32_t type, uint32_t flags,
                       uint64_t offset, uint64_t vaddr,
                       uint64_t filesz, uint64_t memsz,
                       uint64_t align) {
    buf_u32(b, type);
    buf_u32(b, flags);
    buf_u64(b, offset);
    buf_u64(b, vaddr);
    buf_u64(b, vaddr);
    buf_u64(b, filesz);
    buf_u64(b, memsz);
    buf_u64(b, align);
}
static void gen_start(Buffer *code, uint64_t call_vaddr, uint64_t main_vaddr,
                      uint64_t exit_plt_vaddr) {
    uint8_t mov_edi[] = {0x8b, 0x3c, 0x24};
    buffer_append(code, (const char *)mov_edi, 3);
    uint8_t lea_rsi[] = {0x48, 0x8d, 0x74, 0x24, 0x08};
    buffer_append(code, (const char *)lea_rsi, 5);
    uint8_t call_opcode = 0xe8;
    buffer_append(code, (const char *)&call_opcode, 1);
    int32_t rel = (int32_t)(main_vaddr - (call_vaddr + 3 + 5 + 5));
    buffer_append(code, (const char *)&rel, 4);
    uint8_t mov_reg[] = {0x89, 0xc7};
    buffer_append(code, (const char *)mov_reg, 2);
    if (exit_plt_vaddr != 0) {
        buffer_append(code, (const char *)&call_opcode, 1);
        int32_t erel = (int32_t)(exit_plt_vaddr -
                                 (call_vaddr + 3 + 5 + 5 + 2 + 5));
        buffer_append(code, (const char *)&erel, 4);
        uint8_t ud2[] = {0x0f, 0x0b};
        buffer_append(code, (const char *)ud2, 2);
    } else {
        uint8_t mov_imm[] = {0xb8, 0x3c, 0x00, 0x00, 0x00};
        buffer_append(code, (const char *)mov_imm, 5);
        uint8_t syscall[] = {0x0f, 0x05};
        buffer_append(code, (const char *)syscall, 2);
    }
}
static unsigned long elf_hash(const char *name) {
    unsigned long h = 0, g;
    while (*name) {
        h = (h << 4) + (unsigned char)*name++;
        g = h & 0xf0000000;
        if (g) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}
static size_t emit_plt0(Buffer *code, size_t got_fixup[2]) {
    size_t start = code->len;
    emit_byte(code, 0xFF); emit_byte(code, 0x35);
    got_fixup[0] = code->len;
    emit_int32(code, 0);
    emit_byte(code, 0xFF); emit_byte(code, 0x25);
    got_fixup[1] = code->len;
    emit_int32(code, 0);
    while ((code->len - start) < 16) emit_byte(code, 0x90);
    return start;
}
static size_t emit_plt_entry(Buffer *code, size_t idx, size_t plt0_off,
                             size_t *got_fixup) {
    size_t start = code->len;
    emit_byte(code, 0xFF); emit_byte(code, 0x25);
    *got_fixup = code->len;
    emit_int32(code, 0);
    emit_byte(code, 0x68);
    emit_int32(code, (int32_t)idx);
    emit_byte(code, 0xE9);
    size_t pjmp = code->len;
    emit_int32(code, 0);
    int32_t rel = (int32_t)plt0_off - (int32_t)(pjmp + 4);
    memcpy(code->data + pjmp, &rel, 4);
    return start;
}
static int ext_find_or_add(char ***ext, int *num_ext, const char *name) {
    for (int e = 0; e < *num_ext; e++)
        if (strcmp((*ext)[e], name) == 0) return e;
    *ext = realloc(*ext, ((size_t)*num_ext + 1) * sizeof(char *));
    (*ext)[(*num_ext)++] = xstrdup(name);
    return *num_ext - 1;
}
void emit_link(EmitModule **mods, size_t n, const char *path) {
Buffer text;
Buffer rodata;
Buffer data;
    buffer_init(&text); buffer_init(&rodata); buffer_init(&data);
    size_t bss_size = 0;
    size_t *mod_text_off = xcalloc(n, sizeof(size_t));
    size_t *mod_rodata_off = xcalloc(n, sizeof(size_t));
    size_t *mod_data_off = xcalloc(n, sizeof(size_t));
    size_t *mod_bss_off = xcalloc(n, sizeof(size_t));
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        while (text.len & 15) { char z = 0; buffer_append(&text, &z, 1); }
        mod_text_off[i] = text.len;
        buffer_append(&text, m->text.data, m->text.len);
        while (rodata.len & 7) { char z = 0; buffer_append(&rodata, &z, 1); }
        mod_rodata_off[i] = rodata.len;
        buffer_append(&rodata, m->rodata.data, m->rodata.len);
        while (data.len & 7) { char z = 0; buffer_append(&data, &z, 1); }
        mod_data_off[i] = data.len;
        buffer_append(&data, m->data.data, m->data.len);
        while (bss_size & 7) bss_size++;
        mod_bss_off[i] = bss_size;
        bss_size += m->bss_size;
    }
    size_t *mod_sym_base = xcalloc(n + 1, sizeof(size_t));
    for (size_t i = 0; i < n; i++)
        mod_sym_base[i + 1] = mod_sym_base[i] + mods[i]->num_syms;
    size_t total_syms = mod_sym_base[n];
    SymInfo *sinfo = xcalloc(total_syms, sizeof(SymInfo));
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t j = 0; j < m->num_syms; j++) {
            size_t gsi = mod_sym_base[i] + j;
            sinfo[gsi].shndx = m->syms[j].shndx;
            sinfo[gsi].value = m->syms[j].value;
            sinfo[gsi].binding = m->syms[j].binding;
            sinfo[gsi].defined = (m->syms[j].shndx != 0);
        }
    }
    char **ext_list = ((void*)0);
    int num_ext = 0;
    int *reloc_ext_idx = xcalloc(total_syms, sizeof(int));
    for (size_t i = 0; i < total_syms; i++) reloc_ext_idx[i] = -1;
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t r = 0; r < m->num_relocs; r++) {
            if (m->relocs[r].type == 9) continue;
            size_t gsi = mod_sym_base[i] + m->relocs[r].sym;
            if (!sinfo[gsi].defined) {
                const char *nm = m->syms[m->relocs[r].sym].name
                                 ? m->syms[m->relocs[r].sym].name : "";
                reloc_ext_idx[gsi] = ext_find_or_add(&ext_list, &num_ext, nm);
            }
        }
    }
    char **data_ext_list = ((void*)0);
    int num_data_ext = 0;
    int *reloc_data_got_idx = xcalloc(total_syms, sizeof(int));
    for (size_t i = 0; i < total_syms; i++) reloc_data_got_idx[i] = -1;
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t r = 0; r < m->num_relocs; r++) {
            if (m->relocs[r].type != 9) continue;
            size_t gsi = mod_sym_base[i] + m->relocs[r].sym;
            if (reloc_data_got_idx[gsi] >= 0) continue;
            const char *nm = m->syms[m->relocs[r].sym].name
                             ? m->syms[m->relocs[r].sym].name : "";
            reloc_data_got_idx[gsi] = ext_find_or_add(&data_ext_list, &num_data_ext, nm);
        }
    }
    int *data_got_external = num_data_ext ? xcalloc(num_data_ext, sizeof(int)) : ((void*)0);
    for (int j = 0; j < num_data_ext; j++) {
        const char *nm = data_ext_list[j];
        int found = 0;
        for (size_t mi = 0; mi < n && !found; mi++) {
            EmitModule *om = mods[mi];
            for (size_t mj = 0; mj < om->num_syms; mj++) {
                size_t ogsi = mod_sym_base[mi] + mj;
                if (sinfo[ogsi].defined && sinfo[ogsi].binding == 1
                    && om->syms[mj].name
                    && strcmp(om->syms[mj].name, nm) == 0) {
                    found = 1;
                    break;
                }
            }
        }
        data_got_external[j] = !found;
    }
    int exit_ext_idx = -1;
    if (num_ext > 0 || num_data_ext > 0)
        exit_ext_idx = ext_find_or_add(&ext_list, &num_ext, "exit");
    size_t *plt_entry_off = num_ext ? xcalloc(num_ext, sizeof(size_t)) : ((void*)0);
    size_t *plt_got_fixup = num_ext ? xcalloc(num_ext, sizeof(size_t)) : ((void*)0);
    size_t plt0_got_fixup[2];
    size_t plt0_off = emit_plt0(&text, plt0_got_fixup);
    for (int e = 0; e < num_ext; e++)
        plt_entry_off[e] = emit_plt_entry(&text, e, plt0_off, &plt_got_fixup[e]);
Buffer dynstr;
Buffer dynsym;
Buffer hash;
Buffer rela_plt;
Buffer rela_dyn;
Buffer dynamic;
    buffer_init(&dynstr); buffer_init(&dynsym); buffer_init(&hash);
    buffer_init(&rela_plt); buffer_init(&rela_dyn); buffer_init(&dynamic);
    size_t interp_len = (num_ext > 0 || num_data_ext > 0) ? sizeof(INTERP_PATH) : 0;
    int num_dynsym_ext = num_ext + num_data_ext;
    if (num_dynsym_ext > 0) {
        buf_u8(&dynstr, 0);
        buf_bytes(&dynstr, "libc.so.6", sizeof("libc.so.6"));
        for (int i = 0; i < num_ext; i++)
            buf_bytes(&dynstr, ext_list[i], strlen(ext_list[i]) + 1);
        for (int j = 0; j < num_data_ext; j++)
            buf_bytes(&dynstr, data_ext_list[j], strlen(data_ext_list[j]) + 1);
        buf_pad(&dynsym, 24);
        for (int k = 0; k < num_dynsym_ext; k++) {
            buf_u32(&dynsym, 0);
            buf_u8(&dynsym, 1 << 4);
            buf_u8(&dynsym, 0);
            buf_u16(&dynsym, 0);
            buf_u64(&dynsym, 0);
            buf_u64(&dynsym, 0);
        }
        size_t nsyms = 1 + num_dynsym_ext;
        size_t nbucket = (nsyms < 2) ? 1 : 3;
        uint32_t *bucket = calloc(nbucket, sizeof(uint32_t));
        uint32_t *chain = calloc(nsyms, sizeof(uint32_t));
        for (size_t i = 0; i < nsyms; i++) {
            const char *nm;
            if (i == 0) nm = "";
            else if ((int)(i - 1) < num_ext) nm = ext_list[i - 1];
            else nm = data_ext_list[i - 1 - num_ext];
            uint32_t b = (uint32_t)(elf_hash(nm) % nbucket);
            chain[i] = bucket[b];
            bucket[b] = (uint32_t)i;
        }
        buf_u32(&hash, (uint32_t)nbucket);
        buf_u32(&hash, (uint32_t)nsyms);
        for (size_t i = 0; i < nbucket; i++) buf_u32(&hash, bucket[i]);
        for (size_t i = 0; i < nsyms; i++) buf_u32(&hash, chain[i]);
        free(bucket); free(chain);
        for (int i = 0; i < num_ext; i++) {
            buf_u64(&rela_plt, 0);
            buf_u64(&rela_plt, ((uint64_t)(i + 1) << 32) | 7);
            buf_u64(&rela_plt, 0);
        }
        for (int j = 0; j < num_data_ext; j++) {
            if (!data_got_external[j]) continue;
            buf_u64(&rela_dyn, 0);
            buf_u64(&rela_dyn, ((uint64_t)(1 + num_ext + j) << 32) | 6);
            buf_u64(&rela_dyn, 0);
        }
    }
    uint16_t phnum_max = 4;
    size_t hdr_size = 64 + 56 * phnum_max;
    size_t start_offset = hdr_size;
    size_t text_offset = start_offset + 22;
    size_t dynamic_size = 0;
    if (num_ext > 0 || num_data_ext > 0) {
        dynamic_size = (size_t)(11 + (num_data_ext > 0 ? 3 : 0)) * 16;
    }
    size_t dyn_sections_len = interp_len + dynstr.len + dynsym.len + hash.len
        + rela_plt.len + rela_dyn.len + dynamic_size;
    size_t rx_content_len = 22 + text.len + rodata.len + dyn_sections_len;
    size_t rx_filesz = hdr_size + rx_content_len;
    size_t data_file_offset = rx_filesz;
    if (data_file_offset & (0x1000 - 1))
        data_file_offset = (data_file_offset + 0x1000 - 1) & ~(size_t)(0x1000 - 1);
    uint64_t base = 0x400000;
    uint64_t data_vaddr = base + data_file_offset;
    size_t got_data_off = data.len;
    while (got_data_off & 7) got_data_off++;
    uint64_t got_vaddr = data_vaddr + got_data_off;
    uint64_t code_vaddr = base + text_offset;
    size_t *sym_addr = xcalloc(total_syms, sizeof(size_t));
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t j = 0; j < m->num_syms; j++) {
            size_t gsi = mod_sym_base[i] + j;
            const EmitSymbol *sym = &m->syms[j];
            if (sym->shndx == 0) continue;
            switch (sym->shndx) {
            case 1:
                sym_addr[gsi] = code_vaddr + mod_text_off[i] + sym->value; break;
            case 2:
                sym_addr[gsi] = code_vaddr + text.len + mod_rodata_off[i] + sym->value; break;
            case 3:
                sym_addr[gsi] = data_vaddr + mod_data_off[i] + sym->value; break;
            case 4:
                sym_addr[gsi] = data_vaddr + data.len + mod_bss_off[i] + sym->value; break;
            default:
                sym_addr[gsi] = sym->value; break;
            }
        }
    }
    size_t *data_got_addr = num_data_ext ? xcalloc(num_data_ext, sizeof(size_t)) : ((void*)0);
    for (int j = 0; j < num_data_ext; j++) {
        if (data_got_external[j]) continue;
        const char *nm = data_ext_list[j];
        for (size_t mi = 0; mi < n; mi++) {
            EmitModule *om = mods[mi];
            for (size_t mj = 0; mj < om->num_syms; mj++) {
                size_t ogsi = mod_sym_base[mi] + mj;
                if (sinfo[ogsi].defined && sinfo[ogsi].binding == 1
                    && om->syms[mj].name
                    && strcmp(om->syms[mj].name, nm) == 0) {
                    data_got_addr[j] = sym_addr[ogsi];
                    break;
                }
            }
        }
    }
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t r = 0; r < m->num_relocs; r++) {
            const EmitReloc *rel = &m->relocs[r];
            size_t patch_in_text = mod_text_off[i] + rel->offset;
            uint64_t P = code_vaddr + patch_in_text;
            if (rel->type == 9) {
                size_t gsi = mod_sym_base[i] + rel->sym;
                int dgidx = reloc_data_got_idx[gsi];
                uint64_t got_slot_vaddr = got_vaddr + (3 + num_ext + dgidx) * 8;
                int32_t disp = (int32_t)(got_slot_vaddr - (P + 4));
                memcpy(text.data + patch_in_text, &disp, 4);
                continue;
            }
            size_t gsi = mod_sym_base[i] + rel->sym;
            uint64_t S;
            if (sinfo[gsi].defined) {
                S = sym_addr[gsi];
            } else {
                const char *nm = m->syms[rel->sym].name
                                 ? m->syms[rel->sym].name : "";
                size_t global_addr = (size_t)-1;
                for (size_t mi = 0; mi < n && global_addr == (size_t)-1; mi++) {
                    EmitModule *om = mods[mi];
                    for (size_t mj = 0; mj < om->num_syms; mj++) {
                        size_t ogsi = mod_sym_base[mi] + mj;
                        if (sinfo[ogsi].defined && sinfo[ogsi].binding == 1
                            && om->syms[mj].name
                            && strcmp(om->syms[mj].name, nm) == 0) {
                            global_addr = sym_addr[ogsi];
                            break;
                        }
                    }
                }
                if (global_addr != (size_t)-1) {
                    S = global_addr;
                } else {
                    int eidx = reloc_ext_idx[gsi];
                    S = code_vaddr + (eidx >= 0 ? plt_entry_off[eidx] : plt0_off);
                }
            }
            int32_t disp = (int32_t)(S + rel->addend - P);
            memcpy(text.data + patch_in_text, &disp, 4);
        }
    }
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t r = 0; r < m->num_data_relocs; r++) {
            const EmitReloc *rel = &m->data_relocs[r];
            size_t patch_in_data = mod_data_off[i] + rel->offset;
            size_t gsi = mod_sym_base[i] + rel->sym;
            uint64_t S;
            if (sinfo[gsi].defined) {
                S = sym_addr[gsi];
            } else {
                const char *nm = m->syms[rel->sym].name
                                 ? m->syms[rel->sym].name : "";
                size_t global_addr = (size_t)-1;
                for (size_t mi = 0; mi < n && global_addr == (size_t)-1; mi++) {
                    EmitModule *om = mods[mi];
                    for (size_t mj = 0; mj < om->num_syms; mj++) {
                        size_t ogsi = mod_sym_base[mi] + mj;
                        if (sinfo[ogsi].defined && sinfo[ogsi].binding == 1
                            && om->syms[mj].name
                            && strcmp(om->syms[mj].name, nm) == 0) {
                            global_addr = sym_addr[ogsi];
                            break;
                        }
                    }
                }
                if (global_addr != (size_t)-1) {
                    S = global_addr;
                } else {
                    int eidx = reloc_ext_idx[gsi];
                    S = code_vaddr + (eidx >= 0 ? plt_entry_off[eidx] : plt0_off);
                }
            }
            uint64_t value = S + rel->addend;
            memcpy(data.data + patch_in_data, &value, 8);
        }
    }
    for (int f = 0; f < 2; f++) {
        uint64_t target = got_vaddr + (1 + f) * 8;
        uint64_t rip_next = code_vaddr + plt0_got_fixup[f] + 4;
        int32_t disp = (int32_t)((int64_t)target - (int64_t)rip_next);
        memcpy(text.data + plt0_got_fixup[f], &disp, 4);
    }
    for (int e = 0; e < num_ext; e++) {
        uint64_t target = got_vaddr + (3 + e) * 8;
        uint64_t rip_next = code_vaddr + plt_got_fixup[e] + 4;
        int32_t disp = (int32_t)((int64_t)target - (int64_t)rip_next);
        memcpy(text.data + plt_got_fixup[e], &disp, 4);
    }
    uint64_t main_addr = 0;
    int found_main = 0;
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t j = 0; j < m->num_syms; j++) {
            const EmitSymbol *sym = &m->syms[j];
            if (sym->name && strcmp(sym->name, "main") == 0 &&
                sym->shndx == 1 && sym->binding == 1 ) {
                main_addr = code_vaddr + mod_text_off[i] + sym->value;
                found_main = 1;
            }
        }
    }
    if (!found_main) {
        fprintf(stderr, "fakecc: no 'main' function found\n");
        exit(1);
    }
    if (num_dynsym_ext > 0) {
        {
            size_t acc = 1 + sizeof("libc.so.6");
            for (int k = 0; k < num_dynsym_ext; k++) {
                uint32_t noff = (uint32_t)acc;
                memcpy(dynsym.data + 24 + (size_t)k * 24, &noff, 4);
                const char *nm = (k < num_ext) ? ext_list[k] : data_ext_list[k - num_ext];
                acc += strlen(nm) + 1;
            }
        }
        for (int i = 0; i < num_ext; i++) {
            uint64_t roff = got_vaddr + (3 + i) * 8;
            memcpy(rela_plt.data + (size_t)i * 24, &roff, 8);
        }
        {
            int rdj = 0;
            for (int j = 0; j < num_data_ext; j++) {
                if (!data_got_external[j]) continue;
                uint64_t roff = got_vaddr + (3 + num_ext + j) * 8;
                memcpy(rela_dyn.data + (size_t)rdj * 24, &roff, 8);
                rdj++;
            }
        }
        size_t rx_base_vaddr = base + hdr_size;
        size_t dynstr_off = 22 + text.len + rodata.len + interp_len;
        size_t dynsym_off = dynstr_off + dynstr.len;
        size_t hash_off = dynsym_off + dynsym.len;
        size_t rela_plt_off = hash_off + hash.len;
        size_t rela_dyn_off = rela_plt_off + rela_plt.len;
        size_t dynamic_off = rela_dyn_off + rela_dyn.len;
        uint64_t dynstr_vaddr = rx_base_vaddr + dynstr_off;
        buf_u64(&dynamic, 1); buf_u64(&dynamic, 1);
        buf_u64(&dynamic, 5); buf_u64(&dynamic, dynstr_vaddr);
        buf_u64(&dynamic, 6); buf_u64(&dynamic, rx_base_vaddr + dynsym_off);
        buf_u64(&dynamic, 11); buf_u64(&dynamic, 24);
        buf_u64(&dynamic, 10); buf_u64(&dynamic, dynstr.len);
        buf_u64(&dynamic, 4); buf_u64(&dynamic, rx_base_vaddr + hash_off);
        buf_u64(&dynamic, 3); buf_u64(&dynamic, got_vaddr);
        buf_u64(&dynamic, 2); buf_u64(&dynamic, rela_plt.len);
        buf_u64(&dynamic, 20); buf_u64(&dynamic, 7);
        buf_u64(&dynamic, 23); buf_u64(&dynamic, rx_base_vaddr + rela_plt_off);
        if (num_data_ext > 0) {
            buf_u64(&dynamic, 7); buf_u64(&dynamic, rx_base_vaddr + rela_dyn_off);
            buf_u64(&dynamic, 8); buf_u64(&dynamic, rela_dyn.len);
            buf_u64(&dynamic, 9); buf_u64(&dynamic, 24);
        }
        buf_u64(&dynamic, 0); buf_u64(&dynamic, 0);
        Buffer rx;
        buffer_init(&rx);
        uint64_t exit_plt_vaddr = (exit_ext_idx >= 0)
            ? code_vaddr + plt_entry_off[exit_ext_idx] : 0;
        gen_start(&rx, base + start_offset, main_addr, exit_plt_vaddr);
        buf_bytes(&rx, text.data, text.len);
        buf_bytes(&rx, rodata.data, rodata.len);
        size_t interp_off = rx.len;
        buf_bytes(&rx, INTERP_PATH, interp_len);
        buf_bytes(&rx, dynstr.data, dynstr.len);
        buf_bytes(&rx, dynsym.data, dynsym.len);
        buf_bytes(&rx, hash.data, hash.len);
        buf_bytes(&rx, rela_plt.data, rela_plt.len);
        buf_bytes(&rx, rela_dyn.data, rela_dyn.len);
        buf_bytes(&rx, dynamic.data, dynamic.len);
        uint64_t entry = base + start_offset;
        size_t got_count = 3 + num_ext + num_data_ext;
        size_t got_bytes = got_count * 8;
        Buffer got;
        buffer_init(&got);
        buf_u64(&got, rx_base_vaddr + dynamic_off);
        buf_u64(&got, 0);
        buf_u64(&got, 0);
        for (int i = 0; i < num_ext; i++)
            buf_u64(&got, code_vaddr + plt_entry_off[i] + 6);
        for (int j = 0; j < num_data_ext; j++)
            buf_u64(&got, data_got_external[j] ? 0 : data_got_addr[j]);
        Buffer elf;
        buffer_init(&elf);
        uint16_t phnum = 4;
        write_ehdr(&elf, entry, 64, phnum);
        write_phdr(&elf, 1, 4 | 1, 0, base, rx_filesz, rx_filesz, 0x1000);
        write_phdr(&elf, 1, 4 | 2, data_file_offset, data_vaddr,
                   data.len + got_bytes, data.len + got_bytes, 0x1000);
        write_phdr(&elf, 3, 4, hdr_size + interp_off, rx_base_vaddr + interp_off,
                   interp_len, interp_len, 1);
        write_phdr(&elf, 2, 4, hdr_size + dynamic_off, rx_base_vaddr + dynamic_off,
                   dynamic.len, dynamic.len, 8);
        buf_bytes(&elf, rx.data, rx.len);
        while (elf.len < data_file_offset) buf_u8(&elf, 0);
        buf_bytes(&elf, data.data, data.len);
        buf_bytes(&elf, got.data, got.len);
        FILE *f = fopen(path, "wb");
        if (!f) { fprintf(stderr, "fakecc: cannot write '%s'\n", path); exit(1); }
        fwrite(elf.data, 1, elf.len, f);
        fclose(f);
        chmod(path, 0755);
        buffer_free(&rx); buffer_free(&got); buffer_free(&elf);
    } else {
        uint64_t entry = base + start_offset;
        Buffer rx;
        buffer_init(&rx);
        gen_start(&rx, base + start_offset, main_addr, 0);
        buf_bytes(&rx, text.data, text.len);
        buf_bytes(&rx, rodata.data, rodata.len);
        Buffer elf;
        buffer_init(&elf);
        uint16_t phnum = (data.len > 0 || bss_size > 0) ? 2 : 1;
        write_ehdr(&elf, entry, 64, phnum);
        write_phdr(&elf, 1, 4 | 1, 0, base,
                   rx_filesz, rx_filesz, 0x1000);
        if (data.len > 0 || bss_size > 0) {
            write_phdr(&elf, 1, 4 | 2, data_file_offset, data_vaddr,
                       data.len, data.len + bss_size, 0x1000);
        }
        while (elf.len < hdr_size)
            buf_u8(&elf, 0);
        buf_bytes(&elf, rx.data, rx.len);
        if (data.len > 0 || bss_size > 0) {
            while (elf.len < data_file_offset) buf_u8(&elf, 0);
            buf_bytes(&elf, data.data, data.len);
            for (size_t i = 0; i < bss_size; i++) buf_u8(&elf, 0);
        }
        FILE *f = fopen(path, "wb");
        if (!f) { fprintf(stderr, "fakecc: cannot write '%s'\n", path); exit(1); }
        fwrite(elf.data, 1, elf.len, f);
        fclose(f);
        chmod(path, 0755);
        buffer_free(&rx); buffer_free(&elf);
    }
    buffer_free(&dynstr); buffer_free(&dynsym); buffer_free(&hash);
    buffer_free(&rela_plt); buffer_free(&rela_dyn); buffer_free(&dynamic);
    for (int i = 0; i < num_ext; i++) free(ext_list[i]);
    free(ext_list);
    for (int j = 0; j < num_data_ext; j++) free(data_ext_list[j]);
    free(data_ext_list);
    buffer_free(&text); buffer_free(&rodata); buffer_free(&data);
    free(mod_text_off); free(mod_rodata_off); free(mod_data_off); free(mod_bss_off);
    free(mod_sym_base); free(sym_addr); free(sinfo); free(reloc_ext_idx);
    free(reloc_data_got_idx);
    free(plt_entry_off); free(plt_got_fixup);
    free(data_got_addr); free(data_got_external);
}
