#include "fakecc/emit.h"
#include "fakecc/common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------ */
/* EmitModule lifetime                                                 */
/* ------------------------------------------------------------------ */

void emit_module_init(EmitModule *m) {
    buffer_init(&m->text);
    buffer_init(&m->rodata);
    buffer_init(&m->data);
    m->bss_size = 0;
    m->syms = NULL; m->num_syms = 0; m->cap_syms = 0;
    m->relocs = NULL; m->num_relocs = 0; m->cap_relocs = 0;
    m->data_relocs = NULL; m->num_data_relocs = 0; m->cap_data_relocs = 0;
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

/* ------------------------------------------------------------------ */
/* Symbol table                                                        */
/* ------------------------------------------------------------------ */

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
    s->name = name ? xstrdup(name) : NULL;
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
    /* Reuse an existing undefined symbol of the same name if present. */
    for (size_t i = 0; i < m->num_syms; i++) {
        if (m->syms[i].name && strcmp(m->syms[i].name, name) == 0 &&
            m->syms[i].shndx == SECT_UNDEF)
            return (int)i;
    }
    return emit_module_add_symbol(m, name, 1 /* STB_GLOBAL */, 0 /* STT_NOTYPE */,
                                  SECT_UNDEF, 0, 0);
}

/* ------------------------------------------------------------------ */
/* Relocations                                                         */
/* ------------------------------------------------------------------ */

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

/* Add a relocation within .data (for pointer fixups in global initializers).
 * These are written to a separate .rela.data section. */
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

/* ------------------------------------------------------------------ */
/* ELF primitives for object-file writing                              */
/* ------------------------------------------------------------------ */

#define ELFCLASS64      2
#define ELFDATA2LSB     1
#define EV_CURRENT      1
#define ELFOSABI_NONE   0
#define ET_REL          1
#define EM_X86_64       62

#define SHT_NULL        0
#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_NOBITS      8

#define SHF_WRITE       0x1
#define SHF_ALLOC       0x2
#define SHF_EXECINSTR   0x4

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3

#define SHN_UNDEF       0
#define SHN_ABS         0xfff1

#define ELF64_EHDR_SIZE 64
#define ELF64_PHDR_SIZE 56
#define ELF64_SYM_SIZE  24
#define ELF64_RELA_SIZE 24
#define ELF64_SHDR_SIZE 64

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
    ident[0] = ELFCLASS64;
    ident[1] = ELFDATA2LSB;
    ident[2] = EV_CURRENT;
    ident[3] = ELFOSABI_NONE;
    buffer_append(b, ident, 12);

    buf_u16(b, type);          /* e_type */
    buf_u16(b, EM_X86_64);     /* e_machine */
    buf_u32(b, EV_CURRENT);    /* e_version */
    buf_u64(b, 0);             /* e_entry (none for relocatable) */
    buf_u64(b, 0);             /* e_phoff (no program headers) */
    buf_u64(b, 0);             /* e_shoff (patched later) */
    buf_u32(b, 0);             /* e_flags */
    buf_u16(b, ELF64_EHDR_SIZE); /* e_ehsize */
    buf_u16(b, ELF64_PHDR_SIZE); /* e_phentsize */
    buf_u16(b, 0);             /* e_phnum */
    buf_u16(b, ELF64_SHDR_SIZE); /* e_shentsize */
    buf_u16(b, shnum);         /* e_shnum */
    buf_u16(b, 0);             /* e_shstrndx (patched later) */
}

static void write_shdr(Buffer *b, uint32_t name, uint32_t type, uint64_t flags,
                       uint64_t addr, uint64_t offset, uint64_t size,
                       uint32_t link, uint32_t info, uint64_t addralign,
                       uint64_t entsize) {
    buf_u32(b, name);          /* sh_name */
    buf_u32(b, type);          /* sh_type */
    buf_u64(b, flags);         /* sh_flags */
    buf_u64(b, addr);          /* sh_addr */
    buf_u64(b, offset);        /* sh_offset */
    buf_u64(b, size);          /* sh_size */
    buf_u32(b, link);          /* sh_link */
    buf_u32(b, info);          /* sh_info */
    buf_u64(b, addralign);     /* sh_addralign */
    buf_u64(b, entsize);       /* sh_entsize */
}

/* ------------------------------------------------------------------ */
/* emit_obj — write a relocatable object file (ET_REL)                 */
/* ------------------------------------------------------------------ */

void emit_obj(const EmitModule *m, const char *path) {
    /* Section layout: .text, .rodata, .data, .bss, .symtab, .strtab,
     * .shstrtab, .rela.text.  We build the section data first, then the
     * section headers, then patch e_shoff + e_shstrndx into the ehdr. */

    /* --- section name string table --- */
    Buffer shstrtab;
    buffer_init(&shstrtab);
    buf_u8(&shstrtab, 0); /* empty string at index 0 */

    /* name offsets: index by section id */
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

    /* --- symbol name string table --- */
    Buffer strtab;
    buffer_init(&strtab);
    buf_u8(&strtab, 0); /* empty string at index 0 */

    /* --- symbol table --- */
    Buffer symtab;
    buffer_init(&symtab);
    /* [0] NULL symbol */
    buf_pad(&symtab, ELF64_SYM_SIZE);

    /* section symbols (one per real section, in section-index order) */
    uint32_t sym_text = (uint32_t)(symtab.len / ELF64_SYM_SIZE);
    buf_pad(&symtab, ELF64_SYM_SIZE); /* STT_SECTION for .text */
    uint32_t sym_rodata = (uint32_t)(symtab.len / ELF64_SYM_SIZE);
    buf_pad(&symtab, ELF64_SYM_SIZE);
    uint32_t sym_data = (uint32_t)(symtab.len / ELF64_SYM_SIZE);
    buf_pad(&symtab, ELF64_SYM_SIZE);
    uint32_t sym_bss = (uint32_t)(symtab.len / ELF64_SYM_SIZE);
    buf_pad(&symtab, ELF64_SYM_SIZE);

    /* Emit defined + undefined symbols in ELF-required order: all LOCAL
     * symbols first, then all GLOBAL/WEAK symbols.  This groups the
     * .symtab correctly so sh_info (index of first non-local) is valid.
     * We also remap old m->syms[] indices → new .symtab indices so
     * relocation entries (which reference symbols by index) stay correct. */
    int *sym_remap = malloc(m->num_syms * sizeof(int));
    if (!sym_remap) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    for (size_t i = 0; i < m->num_syms; i++) sym_remap[i] = -1;

    /* Pass 1: LOCAL symbols */
    for (size_t i = 0; i < m->num_syms; i++) {
        const EmitSymbol *s = &m->syms[i];
        if (s->binding != STB_LOCAL) continue;
        sym_remap[i] = (int)(symtab.len / ELF64_SYM_SIZE);
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
    /* Remember the first non-local index for sh_info. */
    unsigned first_global = (unsigned)(symtab.len / ELF64_SYM_SIZE);

    /* Pass 2: GLOBAL / WEAK symbols */
    for (size_t i = 0; i < m->num_syms; i++) {
        const EmitSymbol *s = &m->syms[i];
        if (s->binding == STB_LOCAL) continue;
        sym_remap[i] = (int)(symtab.len / ELF64_SYM_SIZE);
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
    /* Fill in the section symbols' st_info now that the table exists. */
    uint8_t sec_info = (STB_LOCAL << 4) | STT_SECTION;
    memcpy(symtab.data + sym_text * ELF64_SYM_SIZE + 4, &sec_info, 1);
    memcpy(symtab.data + sym_rodata * ELF64_SYM_SIZE + 4, &sec_info, 1);
    memcpy(symtab.data + sym_data * ELF64_SYM_SIZE + 4, &sec_info, 1);
    memcpy(symtab.data + sym_bss * ELF64_SYM_SIZE + 4, &sec_info, 1);

    /* --- relocation tables (.rela.text and .rela.data) --- */
    /* Relocations reference symbols by their old m->syms[] index.
     * sym_remap[] maps old index → new .symtab index (which already
     * includes the 5 section symbols at positions 0-4), so we use
     * the remapped value directly. */
    Buffer rela_text;
    buffer_init(&rela_text);
    for (size_t i = 0; i < m->num_relocs; i++) {
        const EmitReloc *r = &m->relocs[i];
        int old_sym = r->sym;
        int new_sym = (old_sym >= 0 && old_sym < (int)m->num_syms)
                      ? sym_remap[old_sym] : old_sym;
        if (new_sym < 0) new_sym = old_sym; /* safety: unmapped stays as-is */
        buf_u64(&rela_text, r->offset);                        /* r_offset */
        buf_u64(&rela_text, ((uint64_t)new_sym << 32) | r->type);
        buf_u64(&rela_text, (uint64_t)(int64_t)r->addend);     /* r_addend */
    }
    Buffer rela_data;
    buffer_init(&rela_data);
    for (size_t i = 0; i < m->num_data_relocs; i++) {
        const EmitReloc *r = &m->data_relocs[i];
        int old_sym = r->sym;
        int new_sym = (old_sym >= 0 && old_sym < (int)m->num_syms)
                      ? sym_remap[old_sym] : old_sym;
        if (new_sym < 0) new_sym = old_sym;
        buf_u64(&rela_data, r->offset);                        /* r_offset */
        buf_u64(&rela_data, ((uint64_t)new_sym << 32) | r->type);
        buf_u64(&rela_data, (uint64_t)(int64_t)r->addend);     /* r_addend */
    }

    /* --- assemble section data in order --- */
    /* We write: .text, .rodata, .data, .bss(implicit size), then metadata. */
    size_t hdr_size = ELF64_EHDR_SIZE;

    Buffer body;
    buffer_init(&body);
    size_t off_text = body.len;
    buf_bytes(&body, m->text.data, m->text.len);
    size_t off_rodata = body.len;
    buf_bytes(&body, m->rodata.data, m->rodata.len);
    size_t off_data = body.len;
    buf_bytes(&body, m->data.data, m->data.len);
    /* .bss contributes size but no bytes. */
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
    /* null + 4 data + symtab + strtab + shstrtab + rela.text + rela.data */
    unsigned shnum = 10;
    unsigned shstrndx = 7; /* index of .shstrtab */

    /* --- ELF header --- */
    Buffer elf;
    buffer_init(&elf);
    write_ehdr(&elf, ET_REL, shnum);
    /* patch e_shoff (offset 40) + e_shnum (offset 60) + e_shstrndx (offset 62) */
    uint64_t shoff_val = shoff;
    memcpy(elf.data + 40, &shoff_val, 8);
    uint16_t shnum_val = shnum;
    memcpy(elf.data + 60, &shnum_val, 2);
    uint16_t shstrndx_val = shstrndx;
    memcpy(elf.data + 62, &shstrndx_val, 2);
    buf_bytes(&elf, body.data, body.len);

    /* --- section headers --- */
    /* Section data begins right after the ELF header (file offset hdr_size);
     * all stored offsets below are body-relative, so add hdr_size. */
    /* [0] NULL */
    buf_pad(&elf, ELF64_SHDR_SIZE);
    /* .text */
    write_shdr(&elf, shname_text, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR,
               0, hdr_size + off_text, m->text.len, 0, 0, 16, 0);
    /* .rodata */
    write_shdr(&elf, shname_rodata, SHT_PROGBITS, SHF_ALLOC,
               0, hdr_size + off_rodata, m->rodata.len, 0, 0, 8, 0);
    /* .data */
    write_shdr(&elf, shname_data, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE,
               0, hdr_size + off_data, m->data.len, 0, 0, 8, 0);
    /* .bss */
    write_shdr(&elf, shname_bss, SHT_NOBITS, SHF_ALLOC | SHF_WRITE,
               0, hdr_size + off_data + m->data.len, m->bss_size, 0, 0, 8, 0);
    /* Section indices (0-based): 0=NULL, 1=.text, 2=.rodata, 3=.data, 4=.bss,
     * 5=.symtab, 6=.strtab, 7=.shstrtab, 8=.rela.text. */
    unsigned symtab_idx = 5;
    unsigned strtab_idx = 6;
    /* Symbol table: sh_info = first_global (computed during symtab emission) */
    write_shdr(&elf, shname_symtab, SHT_SYMTAB, 0,
               0, hdr_size + off_symtab, symtab.len, strtab_idx, first_global, 8,
               ELF64_SYM_SIZE);
    /* .strtab */
    write_shdr(&elf, shname_strtab, SHT_STRTAB, 0,
               0, hdr_size + off_strtab, strtab.len, 0, 0, 1, 0);
    /* .shstrtab */
    write_shdr(&elf, shname_shstrtab, SHT_STRTAB, 0,
               0, hdr_size + off_shstrtab, shstrtab.len, 0, 0, 1, 0);
    /* .rela.text (sh_link = symtab index, sh_info = text index) */
    write_shdr(&elf, shname_rela_text, SHT_RELA, 0,
               0, hdr_size + off_rela_text, rela_text.len, symtab_idx,
               1 /* .text */, 8, ELF64_RELA_SIZE);
    /* .rela.data (sh_link = symtab index, sh_info = data index) */
    write_shdr(&elf, shname_rela_data, SHT_RELA, 0,
               0, hdr_size + off_rela_data, rela_data.len, symtab_idx,
               3 /* .data */, 8, ELF64_RELA_SIZE);

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

/* ------------------------------------------------------------------ */
/* emit_elf — single-TU convenience wrapper                            */
/* ------------------------------------------------------------------ */

void emit_elf(const EmitModule *m, const char *path) {
    EmitModule *arr = (EmitModule *)m;
    emit_link(&arr, 1, path);
}

/* ------------------------------------------------------------------ */
/* emit_obj_read — read a relocatable object file into an EmitModule    */
/* ------------------------------------------------------------------ */

/* Read a uint64 from a byte buffer (little-endian). */
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
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = malloc((size_t)fsize);
    fread(buf, 1, (size_t)fsize, f);
    fclose(f);

    /* Validate ELF header. */
    if (fsize < 64 || buf[0] != 0x7f || buf[1] != 'E' || buf[2] != 'L' || buf[3] != 'F') {
        fprintf(stderr, "fakecc: '%s' is not an ELF object\n", path); free(buf); return -1;
    }
    if (rd_u16(buf + 16) != ET_REL) {
        fprintf(stderr, "fakecc: '%s' is not a relocatable object\n", path); free(buf); return -1;
    }

    uint64_t shoff = rd_u64(buf + 40);
    uint16_t shentsize = rd_u16(buf + 58);
    uint16_t shnum = rd_u16(buf + 60);
    uint16_t shstrndx = rd_u16(buf + 62);

    emit_module_init(m);

    /* Find section name string table. */
    const unsigned char *shstr_sh = buf + shoff + (size_t)shstrndx * shentsize;
    uint64_t shstr_off = rd_u64(shstr_sh + 24);
    const char *shstr = (const char *)buf + shstr_off;

    /* Map section index → section id (SECT_TEXT etc.) by name, and find
     * symtab + rela.text indices. */
    int symtab_idx = -1, rela_text_idx = -1, rela_data_idx = -1, strtab_idx = -1;
    int text_idx = -1, rodata_idx = -1, data_idx = -1, bss_idx = -1;
    for (int s = 0; s < shnum; s++) {
        const unsigned char *sh = buf + shoff + (size_t)s * shentsize;
        uint32_t name_idx = rd_u32(sh);
        const char *sname = shstr + name_idx;
        uint32_t type = rd_u32(sh + 4);
        if (strcmp(sname, ".text") == 0 && type == SHT_PROGBITS) text_idx = s;
        else if (strcmp(sname, ".rodata") == 0 && type == SHT_PROGBITS) rodata_idx = s;
        else if (strcmp(sname, ".data") == 0 && type == SHT_PROGBITS) data_idx = s;
        else if (strcmp(sname, ".bss") == 0 && type == SHT_NOBITS) bss_idx = s;
        else if (strcmp(sname, ".symtab") == 0 && type == SHT_SYMTAB) symtab_idx = s;
        else if (strcmp(sname, ".strtab") == 0 && type == SHT_STRTAB) strtab_idx = s;
        else if (strcmp(sname, ".rela.text") == 0 && type == SHT_RELA) rela_text_idx = s;
        else if (strcmp(sname, ".rela.data") == 0 && type == SHT_RELA) rela_data_idx = s;
    }

    /* String table (for symbol names). */
    const unsigned char *strtab_data = NULL;
    size_t strtab_len = 0;
    if (strtab_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)strtab_idx * shentsize;
        uint64_t stoff = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        strtab_data = buf + stoff;
        strtab_len = (size_t)sz;
    }

    /* Read section data. */
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

    /* Read symbol table. */
    if (symtab_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)symtab_idx * shentsize;
        uint64_t off = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        uint32_t entsize = rd_u32(sh + 56);
        if (entsize == 0) entsize = ELF64_SYM_SIZE;
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
            const char *name = NULL;
            if (name_idx != 0 && strtab_data != NULL &&
                (size_t)name_idx < strtab_len) {
                name = (const char *)strtab_data + name_idx;
            }
            emit_module_add_symbol(m, name,
                                   (uint8_t)(info >> 4), (uint8_t)(info & 0xf),
                                   shndx, (size_t)value, (size_t)size);
        }
    }

    /* Read relocations. */
    if (rela_text_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)rela_text_idx * shentsize;
        uint64_t off = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        uint32_t entsize = rd_u32(sh + 56);
        if (entsize == 0) entsize = ELF64_RELA_SIZE;
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
    /* Read data relocations (pointer fixups). */
    if (rela_data_idx >= 0) {
        const unsigned char *sh = buf + shoff + (size_t)rela_data_idx * shentsize;
        uint64_t off = rd_u64(sh + 24); uint64_t sz = rd_u64(sh + 32);
        uint32_t entsize = rd_u32(sh + 56);
        if (entsize == 0) entsize = ELF64_RELA_SIZE;
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
