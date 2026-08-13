#include "fakecc/emit.h"
#include "fakecc/common.h"
#include "fakecc/debug.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------ */
/* ELF constants                                                       */
/* ------------------------------------------------------------------ */

#define ELFCLASS64      2
#define ELFDATA2LSB     1
#define EV_CURRENT      1
#define ELFOSABI_NONE   0
#define ET_EXEC         2
#define EM_X86_64       62

#define PT_LOAD         1
#define PT_INTERP       3
#define PT_DYNAMIC      2
#define PF_X            1
#define PF_W            2
#define PF_R            4

#define ELF_BASE        0x400000
#define PAGE_SIZE       0x1000

#define ELF64_EHDR_SIZE  64
#define ELF64_PHDR_SIZE  56
#define ELF64_SHDR_SIZE  64
#define ELF64_SYM_SIZE   24

#define SHT_NULL         0
#define SHT_PROGBITS     1
#define SHT_SYMTAB       2
#define SHT_STRTAB       3
#define SHT_NOBITS       8

#define SHF_WRITE        0x1
#define SHF_ALLOC        0x2
#define SHF_EXECINSTR    0x4

#define STB_LOCAL        0
#define STT_SECTION      3

#define DT_NULL         0
#define DT_NEEDED       1
#define DT_STRTAB       5
#define DT_SYMTAB       6
#define DT_SYMENT       11
#define DT_STRSZ        10
#define DT_HASH         4
#define DT_PLTGOT       3
#define DT_PLTRELSZ     2
#define DT_PLTREL       20
#define DT_JMPREL       23
#define DT_RELA         7
#define DT_RELASZ       8
#define DT_RELENT       9
#define DT_RUNPATH      29
#define R_X86_64_JUMP_SLOT 7

#define STB_GLOBAL      1
#define SHN_UNDEF       0

static const char INTERP_PATH[] = "/lib64/ld-linux-x86-64.so.2";

#define START_SIZE  22 /* gen_start: mov_edi(3)+lea_rsi(5)+call(5)+mov_reg(2)+mov_imm(5)+syscall(2) */
#define CALL_SIZE   5

/* ================================================================== */
/* ELF output primitives                                               */
/* ================================================================== */

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
    ident[0] = ELFCLASS64;
    ident[1] = ELFDATA2LSB;
    ident[2] = EV_CURRENT;
    ident[3] = ELFOSABI_NONE;
    buffer_append(b, ident, 12);
    buf_u16(b, ET_EXEC);
    buf_u16(b, EM_X86_64);
    buf_u32(b, EV_CURRENT);
    buf_u64(b, entry);
    buf_u64(b, phoff);
    buf_u64(b, 0); /* e_shoff */
    buf_u32(b, 0); /* e_flags */
    buf_u16(b, ELF64_EHDR_SIZE);
    buf_u16(b, ELF64_PHDR_SIZE);
    buf_u16(b, phnum);
    buf_u16(b, 64);  /* e_shentsize */
    buf_u16(b, 0);   /* e_shnum */
    buf_u16(b, 0);   /* e_shstrndx */
}

static void write_phdr(Buffer *b, uint32_t type, uint32_t flags,
                       uint64_t offset, uint64_t vaddr,
                       uint64_t filesz, uint64_t memsz,
                       uint64_t align) {
    buf_u32(b, type);
    buf_u32(b, flags);
    buf_u64(b, offset);
    buf_u64(b, vaddr);
    buf_u64(b, vaddr); /* paddr */
    buf_u64(b, filesz);
    buf_u64(b, memsz);
    buf_u64(b, align);
}

static void write_shdr_exec(Buffer *b, uint32_t name, uint32_t type,
                            uint64_t flags, uint64_t addr, uint64_t offset,
                            uint64_t size, uint32_t link, uint32_t info,
                            uint64_t addralign, uint64_t entsize) {
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

static uint32_t append_string(Buffer *b, const char *s) {
    uint32_t off = (uint32_t)b->len;
    buf_bytes(b, s, strlen(s) + 1);
    return off;
}

static void write_sym(Buffer *b, uint32_t name, uint8_t binding, uint8_t type,
                      uint16_t shndx, uint64_t value, uint64_t size) {
    buf_u32(b, name);
    buf_u8(b, (uint8_t)((binding << 4) | (type & 0xf)));
    buf_u8(b, 0);
    buf_u16(b, shndx);
    buf_u64(b, value);
    buf_u64(b, size);
}

/*
 * Add the non-ALLOC metadata after all PT_LOAD file content.  The first four
 * section headers point back into those segments; every other section lives
 * after them and therefore cannot enlarge a load segment's p_filesz.
 */
/* Where each output section landed, both in the file and in memory.  Passed
 * as one struct so finalize_sections stays within the dialect's argument
 * limit (and so callers can't transpose two same-typed arguments). */
typedef struct {
    uint64_t code_vaddr;
    uint64_t data_vaddr;
    uint64_t bss_vaddr;
    size_t   text_offset;
    size_t   data_file_offset;
    size_t   text_len;
    size_t   rodata_len;
    size_t   data_len;
    size_t   bss_file_offset;
    size_t   bss_size;
} SectionLayout;

static void finalize_sections(
    Buffer *elf, EmitModule **mods, size_t n,
    const size_t *mod_text_off, const size_t *mod_sym_base,
    const size_t *sym_addr, const SectionLayout *lay,
    uint64_t entry, int want_debug) {
    uint64_t code_vaddr = lay->code_vaddr;
    uint64_t data_vaddr = lay->data_vaddr;
    uint64_t bss_vaddr = lay->bss_vaddr;
    size_t text_offset = lay->text_offset;
    size_t data_file_offset = lay->data_file_offset;
    size_t text_len = lay->text_len;
    size_t rodata_len = lay->rodata_len;
    size_t data_len = lay->data_len;
    size_t bss_file_offset = lay->bss_file_offset;
    size_t bss_size = lay->bss_size;
    Buffer symtab, strtab, shstrtab;
    Buffer debug_abbrev, debug_info, debug_str, debug_line, debug_frame;
    Buffer debug_loc;
    buffer_init(&symtab); buffer_init(&strtab); buffer_init(&shstrtab);
    buffer_init(&debug_abbrev); buffer_init(&debug_info);
    buffer_init(&debug_str); buffer_init(&debug_line);
    buffer_init(&debug_frame); buffer_init(&debug_loc);

    buf_u8(&strtab, 0);
    buf_pad(&symtab, ELF64_SYM_SIZE);

    /* ELF requires all locals to precede the first global symbol. */
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t j = 0; j < m->num_syms; j++) {
            const EmitSymbol *s = &m->syms[j];
            if (!s->name || s->type == STT_SECTION ||
                s->shndx == SECT_UNDEF || s->binding != STB_LOCAL)
                continue;
            uint32_t name = append_string(&strtab, s->name);
            write_sym(&symtab, name, s->binding, s->type, s->shndx,
                      sym_addr[mod_sym_base[i] + j], s->size);
        }
    }
    uint32_t first_global = (uint32_t)(symtab.len / ELF64_SYM_SIZE);
    uint32_t start_name = append_string(&strtab, "_start");
    write_sym(&symtab, start_name, STB_GLOBAL, 2, SECT_TEXT,
              entry, START_SIZE);
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t j = 0; j < m->num_syms; j++) {
            const EmitSymbol *s = &m->syms[j];
            if (!s->name || s->type == STT_SECTION ||
                s->shndx == SECT_UNDEF || s->binding == STB_LOCAL)
                continue;
            uint32_t name = append_string(&strtab, s->name);
            write_sym(&symtab, name, s->binding, s->type, s->shndx,
                      sym_addr[mod_sym_base[i] + j], s->size);
        }
    }

    EmitModule dbg;
    int have_dbg = want_debug;
    if (have_dbg) {
        emit_module_init(&dbg);
        dbg.text.len = text_len;
        for (size_t i = 0; i < n; i++) {
            EmitModule *m = mods[i];
            if (!dbg.dbg_tu_name && m->dbg_tu_name)
                dbg.dbg_tu_name = xstrdup(m->dbg_tu_name);
            for (size_t j = 0; j < m->num_dbg_lines; j++) {
                const DebugLineEntry *line = &m->dbg_lines[j];
                emit_module_add_dbg_line(&dbg, line->file, line->line,
                                         line->col,
                                         mod_text_off[i] + line->pc_off);
            }
            for (size_t j = 0; j < m->num_dbg_funcs; j++) {
                const DebugFunc *f = &m->dbg_funcs[j];
                int fi = emit_module_add_dbg_func(
                    &dbg, f->name, f->file, f->line,
                    mod_text_off[i] + f->start_pc);
                emit_module_dbg_func_end(
                    &dbg, fi, mod_text_off[i] + f->end_pc,
                    mod_text_off[i] + f->prologue_end_pc);
                for (size_t k = 0; k < f->num_vars; k++) {
                    emit_module_add_dbg_var(&dbg, fi, &f->vars[k]);
                    /* Location-list ranges are offsets into the module's own
                     * .text; rebase them onto the linked image. */
                    DebugVar *dv = &dbg.dbg_funcs[fi].vars[
                        dbg.dbg_funcs[fi].num_vars - 1];
                    for (size_t r = 0; r < dv->num_ranges; r++) {
                        dv->ranges[r].pc_start += mod_text_off[i];
                        dv->ranges[r].pc_end += mod_text_off[i];
                    }
                }
                for (size_t k = 0; k < f->num_call_sites; k++) {
                    DebugCallSite cs = f->call_sites[k];
                    /* Shallow fields; add_dbg_call_site deep-copies. */
                    cs.call_pc += mod_text_off[i];
                    cs.return_pc += mod_text_off[i];
                    emit_module_add_dbg_call_site(&dbg, fi, &cs);
                }
            }
            for (size_t j = 0; j < m->num_dbg_globals; j++)
                emit_module_add_dbg_global(&dbg, &m->dbg_globals[j]);
            for (size_t j = 0; j < m->num_syms; j++) {
                const EmitSymbol *s = &m->syms[j];
                if (!s->name || s->shndx == SECT_UNDEF) continue;
                emit_module_add_symbol(
                    &dbg, s->name, s->binding, s->type, s->shndx,
                    sym_addr[mod_sym_base[i] + j], s->size);
            }
        }
        debug_emit_dwarf(&dbg, code_vaddr, &debug_abbrev, &debug_info,
                         &debug_str, &debug_line, &debug_frame, &debug_loc);
    }

    buf_u8(&shstrtab, 0);
    uint32_t shname_text = append_string(&shstrtab, ".text");
    uint32_t shname_rodata = append_string(&shstrtab, ".rodata");
    uint32_t shname_data = append_string(&shstrtab, ".data");
    uint32_t shname_bss = append_string(&shstrtab, ".bss");
    uint32_t shname_symtab = append_string(&shstrtab, ".symtab");
    uint32_t shname_strtab = append_string(&shstrtab, ".strtab");
    uint32_t shname_shstrtab = append_string(&shstrtab, ".shstrtab");
    uint32_t shname_debug_abbrev = 0, shname_debug_info = 0;
    uint32_t shname_debug_str = 0, shname_debug_line = 0;
    uint32_t shname_debug_frame = 0, shname_debug_loc = 0;
    if (have_dbg) {
        shname_debug_abbrev = append_string(&shstrtab, ".debug_abbrev");
        shname_debug_info = append_string(&shstrtab, ".debug_info");
        shname_debug_str = append_string(&shstrtab, ".debug_str");
        shname_debug_line = append_string(&shstrtab, ".debug_line");
        shname_debug_frame = append_string(&shstrtab, ".debug_frame");
        shname_debug_loc = append_string(&shstrtab, ".debug_loc");
    }

    while (elf->len & 7) buf_u8(elf, 0);
    size_t off_symtab = elf->len;
    buf_bytes(elf, symtab.data, symtab.len);
    size_t off_strtab = elf->len;
    buf_bytes(elf, strtab.data, strtab.len);
    size_t off_shstrtab = elf->len;
    buf_bytes(elf, shstrtab.data, shstrtab.len);
    size_t off_debug_abbrev = elf->len;
    if (have_dbg) buf_bytes(elf, debug_abbrev.data, debug_abbrev.len);
    size_t off_debug_info = elf->len;
    if (have_dbg) buf_bytes(elf, debug_info.data, debug_info.len);
    size_t off_debug_str = elf->len;
    if (have_dbg) buf_bytes(elf, debug_str.data, debug_str.len);
    size_t off_debug_line = elf->len;
    if (have_dbg) buf_bytes(elf, debug_line.data, debug_line.len);
    if (have_dbg) while (elf->len & 7) buf_u8(elf, 0);
    size_t off_debug_frame = elf->len;
    if (have_dbg) buf_bytes(elf, debug_frame.data, debug_frame.len);
    if (have_dbg) while (elf->len & 7) buf_u8(elf, 0);
    size_t off_debug_loc = elf->len;
    if (have_dbg) buf_bytes(elf, debug_loc.data, debug_loc.len);

    while (elf->len & 7) buf_u8(elf, 0);
    uint64_t shoff = elf->len;
    buf_pad(elf, ELF64_SHDR_SIZE);
    write_shdr_exec(elf, shname_text, SHT_PROGBITS,
                    SHF_ALLOC | SHF_EXECINSTR, code_vaddr, text_offset,
                    text_len, 0, 0, 16, 0);
    write_shdr_exec(elf, shname_rodata, SHT_PROGBITS, SHF_ALLOC,
                    code_vaddr + text_len, text_offset + text_len,
                    rodata_len, 0, 0, 8, 0);
    write_shdr_exec(elf, shname_data, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE,
                    data_vaddr, data_file_offset, data_len, 0, 0, 8, 0);
    write_shdr_exec(elf, shname_bss, SHT_NOBITS, SHF_ALLOC | SHF_WRITE,
                    bss_vaddr, bss_file_offset,
                    bss_size, 0, 0, 8, 0);
    write_shdr_exec(elf, shname_symtab, SHT_SYMTAB, 0, 0, off_symtab,
                    symtab.len, 6, first_global, 8, ELF64_SYM_SIZE);
    write_shdr_exec(elf, shname_strtab, SHT_STRTAB, 0, 0, off_strtab,
                    strtab.len, 0, 0, 1, 0);
    write_shdr_exec(elf, shname_shstrtab, SHT_STRTAB, 0, 0, off_shstrtab,
                    shstrtab.len, 0, 0, 1, 0);
    if (have_dbg) {
        write_shdr_exec(elf, shname_debug_abbrev, SHT_PROGBITS, 0, 0,
                        off_debug_abbrev, debug_abbrev.len, 0, 0, 1, 0);
        write_shdr_exec(elf, shname_debug_info, SHT_PROGBITS, 0, 0,
                        off_debug_info, debug_info.len, 0, 0, 1, 0);
        write_shdr_exec(elf, shname_debug_str, SHT_PROGBITS, 0, 0,
                        off_debug_str, debug_str.len, 0, 0, 1, 0);
        write_shdr_exec(elf, shname_debug_line, SHT_PROGBITS, 0, 0,
                        off_debug_line, debug_line.len, 0, 0, 1, 0);
        write_shdr_exec(elf, shname_debug_frame, SHT_PROGBITS, 0, 0,
                        off_debug_frame, debug_frame.len, 0, 0, 8, 0);
        write_shdr_exec(elf, shname_debug_loc, SHT_PROGBITS, 0, 0,
                        off_debug_loc, debug_loc.len, 0, 0, 1, 0);
    }

    uint16_t shnum = (uint16_t)(have_dbg ? 14 : 8);
    uint16_t shstrndx = 7;
    memcpy(elf->data + 40, &shoff, sizeof(shoff));
    memcpy(elf->data + 60, &shnum, sizeof(shnum));
    memcpy(elf->data + 62, &shstrndx, sizeof(shstrndx));

    if (have_dbg) {
        dbg.text.len = 0;
        emit_module_free(&dbg);
    }
    buffer_free(&symtab); buffer_free(&strtab); buffer_free(&shstrtab);
    buffer_free(&debug_abbrev); buffer_free(&debug_info);
    buffer_free(&debug_str); buffer_free(&debug_line);
    buffer_free(&debug_frame); buffer_free(&debug_loc);
}

/* Emit the process entry stub.  `exit_plt_vaddr` is the address of the
 * `exit` PLT entry, or 0 for a static executable (no libc to call).
 *
 * Returning from main is not the same as calling exit(): libc buffers stdout,
 * and the buffer is drained by an atexit handler that only exit() runs.  A
 * raw exit_group syscall skips it, so a dynamically linked program that
 * printf()s and then returns from main writes nothing at all.  When libc is
 * present, hand control to its exit(); keep the syscall for static binaries.
 *
 * Both forms are exactly START_SIZE bytes so the layout above does not have to
 * know which one it gets. */
static void gen_start(Buffer *code, uint64_t call_vaddr, uint64_t main_vaddr,
                      uint64_t exit_plt_vaddr) {
    /* SysV ABI: main(argc @ edi, argv @ rsi). At process entry the kernel
     * leaves [rsp]=argc, [rsp+8]=argv. Load them before calling main. */
    uint8_t mov_edi[] = {0x8b, 0x3c, 0x24}; /* mov edi, [rsp] */
    buffer_append(code, (const char *)mov_edi, 3);
    uint8_t lea_rsi[] = {0x48, 0x8d, 0x74, 0x24, 0x08}; /* lea rsi, [rsp+8] */
    buffer_append(code, (const char *)lea_rsi, 5);
    /* call main (rel32 is relative to end of the 5-byte call, i.e. call_vaddr+8+5) */
    uint8_t call_opcode = 0xe8;
    buffer_append(code, (const char *)&call_opcode, 1);
    int32_t rel = (int32_t)(main_vaddr - (call_vaddr + 3 + 5 + CALL_SIZE));
    buffer_append(code, (const char *)&rel, 4);
    uint8_t mov_reg[] = {0x89, 0xc7}; /* mov edi, eax (exit code = main return) */
    buffer_append(code, (const char *)mov_reg, 2);
    if (exit_plt_vaddr != 0) {
        buffer_append(code, (const char *)&call_opcode, 1);
        int32_t erel = (int32_t)(exit_plt_vaddr -
                                 (call_vaddr + 3 + 5 + CALL_SIZE + 2 + CALL_SIZE));
        buffer_append(code, (const char *)&erel, 4);
        uint8_t ud2[] = {0x0f, 0x0b}; /* exit() does not return */
        buffer_append(code, (const char *)ud2, 2);
    } else {
        uint8_t mov_imm[] = {0xb8, 0x3c, 0x00, 0x00, 0x00}; /* mov eax, 60 */
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

/* ================================================================== */
/* PLT emission                                                        */
/* ================================================================== */

static size_t emit_plt0(Buffer *code, size_t got_fixup[2]) {
    size_t start = code->len;
    emit_byte(code, 0xFF); emit_byte(code, 0x35); /* push qword [rip+disp] */
    got_fixup[0] = code->len;
    emit_int32(code, 0);
    emit_byte(code, 0xFF); emit_byte(code, 0x25); /* jmp qword [rip+disp] */
    got_fixup[1] = code->len;
    emit_int32(code, 0);
    while ((code->len - start) < 16) emit_byte(code, 0x90);
    return start;
}

static size_t emit_plt_entry(Buffer *code, size_t idx, size_t plt0_off,
                             size_t *got_fixup) {
    size_t start = code->len;
    emit_byte(code, 0xFF); emit_byte(code, 0x25); /* jmp qword [rip+disp] */
    *got_fixup = code->len;
    emit_int32(code, 0);
    emit_byte(code, 0x68); /* push imm32 (relocation index) */
    emit_int32(code, (int32_t)idx);
    emit_byte(code, 0xE9); /* jmp rel32 */
    size_t pjmp = code->len;
    emit_int32(code, 0);
    int32_t rel = (int32_t)plt0_off - (int32_t)(pjmp + 4);
    memcpy(code->data + pjmp, &rel, 4);
    return start;
}

/* ================================================================== */
/* emit_link                                                           */
/* ================================================================== */

/* Find the PLT slot index for an undefined symbol name; create if absent. */
static int ext_find_or_add(char ***ext, int *num_ext, const char *name) {
    for (int e = 0; e < *num_ext; e++)
        if (strcmp((*ext)[e], name) == 0) return e;
    *ext = realloc(*ext, ((size_t)*num_ext + 1) * sizeof(char *));
    (*ext)[(*num_ext)++] = xstrdup(name);
    return *num_ext - 1;
}

/* Append a DT_NEEDED soname if it is not already present. */
static void needed_add(char ***needed, int *num, const char *soname) {
    for (int i = 0; i < *num; i++)
        if (strcmp((*needed)[i], soname) == 0) return;
    *needed = realloc(*needed, ((size_t)*num + 1) * sizeof(char *));
    if (!*needed) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
    (*needed)[(*num)++] = xstrdup(soname);
}

void emit_link(EmitModule **mods, size_t n, const char *path,
               const char **needed_in, size_t num_needed_in, int nodefaultlibs,
               const char **lib_paths, size_t num_lib_paths,
               int want_debug) {
    /* ---- Merge sections ---- */
    Buffer text, rodata, data;
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

    /* ---- Per-module symbol base indices ---- */
    size_t *mod_sym_base = xcalloc(n + 1, sizeof(size_t));
    for (size_t i = 0; i < n; i++)
        mod_sym_base[i + 1] = mod_sym_base[i] + mods[i]->num_syms;
    size_t total_syms = mod_sym_base[n];

    /* ---- Per-symbol metadata ---- */
    typedef struct { int defined; int shndx; size_t value; uint8_t binding; } SymInfo;
    SymInfo *sinfo = xcalloc(total_syms, sizeof(SymInfo));
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t j = 0; j < m->num_syms; j++) {
            size_t gsi = mod_sym_base[i] + j;
            sinfo[gsi].shndx = m->syms[j].shndx;
            sinfo[gsi].value = m->syms[j].value;
            sinfo[gsi].binding = m->syms[j].binding;
            sinfo[gsi].defined = (m->syms[j].shndx != SECT_UNDEF);
        }
    }

    /* ---- PLT slot assignment for undefined referenced symbols (functions)
     * Only symbols that are not defined as GLOBAL in ANY linked module are
     * truly external (e.g. libc).  Cross-module references stay static. */
    char **ext_list = NULL;
    int num_ext = 0;
    int *reloc_ext_idx = xcalloc(total_syms, sizeof(int));
    for (size_t i = 0; i < total_syms; i++) reloc_ext_idx[i] = -1;

    /* Helper: is `nm` defined GLOBAL in any module? */
    /* (open-coded below to avoid a nested function) */

    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t r = 0; r < m->num_relocs; r++) {
            if (m->relocs[r].type == R_X86_64_GOTPCREL) continue;
            size_t gsi = mod_sym_base[i] + m->relocs[r].sym;
            if (sinfo[gsi].defined) continue;
            const char *nm = m->syms[m->relocs[r].sym].name
                             ? m->syms[m->relocs[r].sym].name : "";
            int resolved = 0;
            for (size_t mi = 0; mi < n && !resolved; mi++) {
                EmitModule *om = mods[mi];
                for (size_t mj = 0; mj < om->num_syms; mj++) {
                    size_t ogsi = mod_sym_base[mi] + mj;
                    if (sinfo[ogsi].defined && sinfo[ogsi].binding == 1
                        && om->syms[mj].name
                        && strcmp(om->syms[mj].name, nm) == 0) {
                        resolved = 1;
                        break;
                    }
                }
            }
            if (!resolved)
                reloc_ext_idx[gsi] = ext_find_or_add(&ext_list, &num_ext, nm);
        }
    }

    /* ---- Data GOT slot assignment for external variables (GOTPCREL) ----
     * An IR_GADDR of a symbol undefined in the referencing module emits a
     * GOTPCREL reloc.  We assign each such referenced symbol a slot in the
     * data GOT (right after the PLT GOT slots).  The linker fills the slot
     * with the symbol's address: statically if the symbol is defined in
     * another module (cross-module global), or via a R_X86_64_GLOB_DAT
     * relocation if it is truly external (libc variable like `stderr`). */
    char **data_ext_list = NULL;
    int num_data_ext = 0;
    int *reloc_data_got_idx = xcalloc(total_syms, sizeof(int));
    for (size_t i = 0; i < total_syms; i++) reloc_data_got_idx[i] = -1;
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t r = 0; r < m->num_relocs; r++) {
            if (m->relocs[r].type != R_X86_64_GOTPCREL) continue;
            size_t gsi = mod_sym_base[i] + m->relocs[r].sym;
            if (reloc_data_got_idx[gsi] >= 0) continue;
            const char *nm = m->syms[m->relocs[r].sym].name
                             ? m->syms[m->relocs[r].sym].name : "";
            reloc_data_got_idx[gsi] = ext_find_or_add(&data_ext_list, &num_data_ext, nm);
        }
    }
    /* Determine, per data-external slot, whether the symbol is defined in any
     * module (cross-module global) or truly external (libc).  This only needs
     * symbol names + definitions, NOT final addresses, so it can run now —
     * before layout — which lets the .rela.dyn buffer be sized for layout. */
    int *data_got_external = num_data_ext ? xcalloc(num_data_ext, sizeof(int)) : NULL;
    int num_true_data_ext = 0;
    for (int j = 0; j < num_data_ext; j++) {
        const char *nm = data_ext_list[j];
        int found = 0;
        for (size_t mi = 0; mi < n && !found; mi++) {
            EmitModule *om = mods[mi];
            for (size_t mj = 0; mj < om->num_syms; mj++) {
                size_t ogsi = mod_sym_base[mi] + mj;
                if (sinfo[ogsi].defined && sinfo[ogsi].binding == 1 /* GLOBAL */
                    && om->syms[mj].name
                    && strcmp(om->syms[mj].name, nm) == 0) {
                    found = 1;
                    break;
                }
            }
        }
        data_got_external[j] = !found;
        if (!found) num_true_data_ext++;
    }

    /* Dynamic link only when something is truly undefined after merging. */
    int need_dynamic = (num_ext > 0 || num_true_data_ext > 0);

    /* ---- Shared-library DT_NEEDED list (from -l only; no automatic libc) ----
     * Builtin rt/ supplies the hosted stdlib.  System libs are opt-in via -l,
     * like cgo.  `-nodefaultlibs` is retained as a no-op alias for scripts
     * that still pass it (defaults already skip libc). */
    char **needed = NULL;
    int num_needed = 0;
    for (size_t i = 0; i < num_needed_in; i++)
        needed_add(&needed, &num_needed, needed_in[i]);
    (void)nodefaultlibs;

    /* Colon-separated DT_RUNPATH from -L directories (and dirs of explicit .so). */
    char *runpath = NULL;
    if (need_dynamic && num_lib_paths > 0) {
        size_t len = 1; /* NUL */
        for (size_t i = 0; i < num_lib_paths; i++)
            len += strlen(lib_paths[i]) + (i > 0 ? 1 : 0);
        runpath = malloc(len);
        if (!runpath) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        size_t pos = 0;
        for (size_t i = 0; i < num_lib_paths; i++) {
            if (i > 0) runpath[pos++] = ':';
            size_t n = strlen(lib_paths[i]);
            memcpy(runpath + pos, lib_paths[i], n);
            pos += n;
        }
        runpath[pos] = '\0';
    }

    /* ---- Reserve a PLT slot for exit() when libc is linked ----
     * The entry stub calls it instead of issuing exit_group directly, so libc
     * gets to run its atexit handlers (stdout is flushed by one of them).
     * Without libc in DT_NEEDED, keep the raw syscall exit path — the builtin
     * runtime's exit() is called directly when defined in a linked module. */
    int exit_ext_idx = -1;
    int have_libc = 0;
    for (int i = 0; i < num_needed; i++) {
        if (strcmp(needed[i], "libc.so.6") == 0) { have_libc = 1; break; }
    }
    if (need_dynamic && have_libc)
        exit_ext_idx = ext_find_or_add(&ext_list, &num_ext, "exit");

    /* ---- Build PLT at end of .text (only when truly external funcs) ---- */
    size_t *plt_entry_off = num_ext ? xcalloc(num_ext, sizeof(size_t)) : NULL;
    size_t *plt_got_fixup = num_ext ? xcalloc(num_ext, sizeof(size_t)) : NULL;
    size_t plt0_got_fixup[2] = {0, 0};
    size_t plt0_off = 0;
    if (num_ext > 0) {
        plt0_off = emit_plt0(&text, plt0_got_fixup);
        for (int e = 0; e < num_ext; e++)
            plt_entry_off[e] = emit_plt_entry(&text, e, plt0_off, &plt_got_fixup[e]);
    }
    /* ---- Build dynamic-linking section buffers (sizes needed for layout) ---- */
    Buffer dynstr, dynsym, hash, rela_plt, rela_dyn, dynamic;
    buffer_init(&dynstr); buffer_init(&dynsym); buffer_init(&hash);
    buffer_init(&rela_plt); buffer_init(&rela_dyn); buffer_init(&dynamic);
    size_t interp_len = need_dynamic ? sizeof(INTERP_PATH) : 0;

    /* External symbols that need dynsym entries: only TRULY external ones.
     * Cross-module data still occupies GOT slots (num_data_ext) but does not
     * appear in .dynsym.  dynstr layout when dynamic:
     * "\0" + DT_NEEDED sonames + [DT_RUNPATH] + true-ext func names
     * + true-ext data names. */
    int num_dynsym_ext = num_ext + num_true_data_ext;
    size_t needed_str_bytes = 0;
    size_t runpath_str_bytes = 0;
    size_t runpath_dynstr_off = 0;
    if (need_dynamic) {
        buf_u8(&dynstr, 0);
        for (int i = 0; i < num_needed; i++) {
            buf_bytes(&dynstr, needed[i], strlen(needed[i]) + 1);
            needed_str_bytes += strlen(needed[i]) + 1;
        }
        if (runpath) {
            runpath_dynstr_off = dynstr.len;
            buf_bytes(&dynstr, runpath, strlen(runpath) + 1);
            runpath_str_bytes = strlen(runpath) + 1;
        }
        for (int i = 0; i < num_ext; i++)
            buf_bytes(&dynstr, ext_list[i], strlen(ext_list[i]) + 1);
        for (int j = 0; j < num_data_ext; j++) {
            if (!data_got_external[j]) continue;
            buf_bytes(&dynstr, data_ext_list[j], strlen(data_ext_list[j]) + 1);
        }
        buf_pad(&dynsym, 24); /* [0] NULL symbol */
        for (int k = 0; k < num_dynsym_ext; k++) {
            buf_u32(&dynsym, 0); /* st_name patched below */
            buf_u8(&dynsym, STB_GLOBAL << 4);
            buf_u8(&dynsym, 0);
            buf_u16(&dynsym, SHN_UNDEF);
            buf_u64(&dynsym, 0);
            buf_u64(&dynsym, 0);
        }
        size_t nsyms = 1 + (size_t)num_dynsym_ext;
        size_t nbucket = (nsyms < 2) ? 1 : 3;
        uint32_t *bucket = calloc(nbucket, sizeof(uint32_t));
        uint32_t *chain = calloc(nsyms, sizeof(uint32_t));
        for (size_t i = 0; i < nsyms; i++) {
            const char *nm = "";
            if (i > 0) {
                int idx = (int)(i - 1);
                if (idx < num_ext) nm = ext_list[idx];
                else {
                    int want = idx - num_ext;
                    int seen = 0;
                    for (int j = 0; j < num_data_ext; j++) {
                        if (!data_got_external[j]) continue;
                        if (seen == want) { nm = data_ext_list[j]; break; }
                        seen++;
                    }
                }
            }
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
            buf_u64(&rela_plt, ((uint64_t)(i + 1) << 32) | R_X86_64_JUMP_SLOT);
            buf_u64(&rela_plt, 0);
        }
        {
            int dyn_sym_i = 1 + num_ext;
            for (int j = 0; j < num_data_ext; j++) {
                if (!data_got_external[j]) continue;
                buf_u64(&rela_dyn, 0);
                buf_u64(&rela_dyn, ((uint64_t)dyn_sym_i << 32) | R_X86_64_GLOB_DAT);
                buf_u64(&rela_dyn, 0);
                dyn_sym_i++;
            }
        }
    }

    /* ---- Compute layout ---- */
    /* phnum is finalized after we know whether a data segment is needed;
     * reserve header space for the maximum (4 phdrs: RX, RW, INTERP, DYNAMIC)
     * so that segment file offsets are stable regardless of which are used. */
    uint16_t phnum_max = 4;
    size_t hdr_size = ELF64_EHDR_SIZE + ELF64_PHDR_SIZE * phnum_max;
    size_t start_offset = hdr_size;
    size_t text_offset = start_offset + START_SIZE;
    /* .dynamic size: num_needed×DT_NEEDED + [DT_RUNPATH] + 9 fixed tags +
     * DT_NULL, plus DT_RELA/RELASZ/RELENT when truly-external DATA vars exist. */
    size_t dynamic_size = 0;
    if (need_dynamic) {
        dynamic_size = (size_t)(num_needed + 10 + (runpath ? 1 : 0)
                                + (num_true_data_ext > 0 ? 3 : 0)) * 16;
    }
    size_t dyn_sections_len = interp_len + dynstr.len + dynsym.len + hash.len
        + rela_plt.len + rela_dyn.len + dynamic_size;
    size_t rx_content_len = START_SIZE + text.len + rodata.len + dyn_sections_len;
    size_t rx_filesz = hdr_size + rx_content_len;
    size_t data_file_offset = rx_filesz;
    if (data_file_offset & (PAGE_SIZE - 1))
        data_file_offset = (data_file_offset + PAGE_SIZE - 1) & ~(size_t)(PAGE_SIZE - 1);
    uint64_t base = ELF_BASE;
    uint64_t data_vaddr = base + data_file_offset;
    size_t got_data_off = data.len;
    while (got_data_off & 7) got_data_off++;
    uint64_t got_vaddr = data_vaddr + got_data_off;
    size_t layout_got_bytes = need_dynamic
        ? (size_t)(3 + num_ext + num_data_ext) * 8
        : (num_data_ext > 0 ? (size_t)(3 + num_data_ext) * 8 : 0);
    size_t bss_data_off = layout_got_bytes
        ? got_data_off + layout_got_bytes : data.len;
    uint64_t bss_vaddr = data_vaddr + bss_data_off;
    size_t bss_file_offset = data_file_offset + bss_data_off;
    uint64_t code_vaddr = base + text_offset;

    /* ---- Compute final symbol addresses ---- */
    size_t *sym_addr = xcalloc(total_syms, sizeof(size_t));
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t j = 0; j < m->num_syms; j++) {
            size_t gsi = mod_sym_base[i] + j;
            const EmitSymbol *sym = &m->syms[j];
            if (sym->shndx == SECT_UNDEF) continue;
            switch (sym->shndx) {
            case SECT_TEXT:
                sym_addr[gsi] = code_vaddr + mod_text_off[i] + sym->value; break;
            case SECT_RODATA:
                sym_addr[gsi] = code_vaddr + text.len + mod_rodata_off[i] + sym->value; break;
            case SECT_DATA:
                sym_addr[gsi] = data_vaddr + mod_data_off[i] + sym->value; break;
            case SECT_BSS:
                sym_addr[gsi] = bss_vaddr + mod_bss_off[i] + sym->value; break;
            default:
                sym_addr[gsi] = sym->value; break;
            }
        }
    }

    /* ---- Resolve static data GOT fill values ----
     * For cross-module data globals (data_got_external[j] == 0) the GOT slot is
     * filled at link time with the symbol's final address.  Truly-external
     * slots (data_got_external[j] == 1) are left for the dynamic linker, so
     * they need no static address here.  This needs sym_addr, hence runs after
     * layout. */
    size_t *data_got_addr = num_data_ext ? xcalloc(num_data_ext, sizeof(size_t)) : NULL;
    for (int j = 0; j < num_data_ext; j++) {
        if (data_got_external[j]) continue; /* external: dynlinker fills it */
        const char *nm = data_ext_list[j];
        for (size_t mi = 0; mi < n; mi++) {
            EmitModule *om = mods[mi];
            for (size_t mj = 0; mj < om->num_syms; mj++) {
                size_t ogsi = mod_sym_base[mi] + mj;
                if (sinfo[ogsi].defined && sinfo[ogsi].binding == 1 /* GLOBAL */
                    && om->syms[mj].name
                    && strcmp(om->syms[mj].name, nm) == 0) {
                    data_got_addr[j] = sym_addr[ogsi];
                    break;
                }
            }
        }
    }

    /* ---- Apply relocations ---- */
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t r = 0; r < m->num_relocs; r++) {
            const EmitReloc *rel = &m->relocs[r];
            size_t patch_in_text = mod_text_off[i] + rel->offset;
            uint64_t P = code_vaddr + patch_in_text;
            if (rel->type == R_X86_64_GOTPCREL) {
                /* Load &global from a data GOT entry: the disp targets the GOT
                 * slot, whose qword holds the symbol's address (filled below,
                 * either statically or via R_X86_64_GLOB_DAT). */
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
                /* Defined in its own module. */
                S = sym_addr[gsi];
            } else {
                /* Undefined locally — look for a GLOBAL definition in another
                 * module with the same name. */
                const char *nm = m->syms[rel->sym].name
                                 ? m->syms[rel->sym].name : "";
                size_t global_addr = (size_t)-1;
                for (size_t mi = 0; mi < n && global_addr == (size_t)-1; mi++) {
                    EmitModule *om = mods[mi];
                    for (size_t mj = 0; mj < om->num_syms; mj++) {
                        size_t ogsi = mod_sym_base[mi] + mj;
                        if (sinfo[ogsi].defined && sinfo[ogsi].binding == 1 /* GLOBAL */
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
                    /* Truly external (libc) → PLT. */
                    int eidx = reloc_ext_idx[gsi];
                    S = code_vaddr + (eidx >= 0 ? plt_entry_off[eidx] : plt0_off);
                }
            }
            int32_t disp = (int32_t)(S + rel->addend - P);
            memcpy(text.data + patch_in_text, &disp, 4);
        }
    }

    /* ---- Apply data relocations (pointer fixups in .data) ---- */
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
                /* Undefined locally — look for a GLOBAL definition in another
                 * module with the same name. */
                const char *nm = m->syms[rel->sym].name
                                 ? m->syms[rel->sym].name : "";
                size_t global_addr = (size_t)-1;
                for (size_t mi = 0; mi < n && global_addr == (size_t)-1; mi++) {
                    EmitModule *om = mods[mi];
                    for (size_t mj = 0; mj < om->num_syms; mj++) {
                        size_t ogsi = mod_sym_base[mi] + mj;
                        if (sinfo[ogsi].defined && sinfo[ogsi].binding == 1 /* GLOBAL */
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
                    /* Truly external → PLT (should not happen for data fixups). */
                    int eidx = reloc_ext_idx[gsi];
                    S = code_vaddr + (eidx >= 0 ? plt_entry_off[eidx] : plt0_off);
                }
            }
            /* R_X86_64_64: absolute 64-bit, value = S + A. */
            uint64_t value = S + rel->addend;
            memcpy(data.data + patch_in_data, &value, 8);
        }
    }

    /* ---- Patch PLT GOT fixups ---- */
    if (num_ext > 0) {
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
    }

    /* ---- Find main (and optional static exit for the entry stub) ---- */
    uint64_t main_addr = 0;
    uint64_t exit_static_addr = 0;
    int found_main = 0;
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t j = 0; j < m->num_syms; j++) {
            const EmitSymbol *sym = &m->syms[j];
            if (!sym->name || sym->shndx != SECT_TEXT || sym->binding != 1)
                continue;
            if (strcmp(sym->name, "main") == 0) {
                main_addr = code_vaddr + mod_text_off[i] + sym->value;
                found_main = 1;
            }
            if (strcmp(sym->name, "exit") == 0)
                exit_static_addr = code_vaddr + mod_text_off[i] + sym->value;
        }
    }
    if (!found_main) {
        fprintf(stderr, "fakecc: no 'main' function found\n");
        exit(1);
    }

    /* ================================================================ */
    /* Finalize output                                                  */
    /* ================================================================ */
    if (need_dynamic) {
        /* Patch dynsym st_name fields. */
        {
            size_t acc = 1 + needed_str_bytes + runpath_str_bytes;
            int k = 0;
            for (int i = 0; i < num_ext; i++, k++) {
                uint32_t noff = (uint32_t)acc;
                memcpy(dynsym.data + 24 + (size_t)k * 24, &noff, 4);
                acc += strlen(ext_list[i]) + 1;
            }
            for (int j = 0; j < num_data_ext; j++) {
                if (!data_got_external[j]) continue;
                uint32_t noff = (uint32_t)acc;
                memcpy(dynsym.data + 24 + (size_t)k * 24, &noff, 4);
                acc += strlen(data_ext_list[j]) + 1;
                k++;
            }
        }
        /* Patch rela.plt r_offsets (function externals). */
        for (int i = 0; i < num_ext; i++) {
            uint64_t roff = got_vaddr + (3 + i) * 8;
            memcpy(rela_plt.data + (size_t)i * 24, &roff, 8);
        }
        /* Patch rela.dyn r_offsets (truly-external data variables). */
        {
            int rdj = 0;
            for (int j = 0; j < num_data_ext; j++) {
                if (!data_got_external[j]) continue;
                uint64_t roff = got_vaddr + (3 + num_ext + j) * 8;
                memcpy(rela_dyn.data + (size_t)rdj * 24, &roff, 8);
                rdj++;
            }
        }
        /* .dynamic */
        size_t rx_base_vaddr = base + hdr_size;
        size_t dynstr_off = START_SIZE + text.len + rodata.len + interp_len;
        size_t dynsym_off = dynstr_off + dynstr.len;
        size_t hash_off = dynsym_off + dynsym.len;
        size_t rela_plt_off = hash_off + hash.len;
        size_t rela_dyn_off = rela_plt_off + rela_plt.len;
        size_t dynamic_off = rela_dyn_off + rela_dyn.len;
        uint64_t dynstr_vaddr = rx_base_vaddr + dynstr_off;
        /* DT_NEEDED values are byte offsets into .dynstr. */
        {
            size_t off = 1; /* skip the leading NUL */
            for (int i = 0; i < num_needed; i++) {
                buf_u64(&dynamic, DT_NEEDED);
                buf_u64(&dynamic, off);
                off += strlen(needed[i]) + 1;
            }
        }
        if (runpath) {
            buf_u64(&dynamic, DT_RUNPATH);
            buf_u64(&dynamic, runpath_dynstr_off);
        }
        buf_u64(&dynamic, DT_STRTAB);   buf_u64(&dynamic, dynstr_vaddr);
        buf_u64(&dynamic, DT_SYMTAB);   buf_u64(&dynamic, rx_base_vaddr + dynsym_off);
        buf_u64(&dynamic, DT_SYMENT);   buf_u64(&dynamic, 24);
        buf_u64(&dynamic, DT_STRSZ);    buf_u64(&dynamic, dynstr.len);
        buf_u64(&dynamic, DT_HASH);     buf_u64(&dynamic, rx_base_vaddr + hash_off);
        buf_u64(&dynamic, DT_PLTGOT);   buf_u64(&dynamic, got_vaddr);
        buf_u64(&dynamic, DT_PLTRELSZ); buf_u64(&dynamic, rela_plt.len);
        buf_u64(&dynamic, DT_PLTREL);   buf_u64(&dynamic, DT_RELA);
        buf_u64(&dynamic, DT_JMPREL);   buf_u64(&dynamic, rx_base_vaddr + rela_plt_off);
        if (num_true_data_ext > 0) {
            buf_u64(&dynamic, DT_RELA);     buf_u64(&dynamic, rx_base_vaddr + rela_dyn_off);
            buf_u64(&dynamic, DT_RELASZ);   buf_u64(&dynamic, rela_dyn.len);
            buf_u64(&dynamic, DT_RELENT);   buf_u64(&dynamic, 24);
        }
        buf_u64(&dynamic, DT_NULL);     buf_u64(&dynamic, 0);

        /* ---- Assemble RX segment content ---- */
        Buffer rx;
        buffer_init(&rx);
        uint64_t exit_call = 0;
        if (exit_ext_idx >= 0)
            exit_call = code_vaddr + plt_entry_off[exit_ext_idx];
        else if (exit_static_addr)
            exit_call = exit_static_addr;
        gen_start(&rx, base + start_offset, main_addr, exit_call);
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
        buf_u64(&got, rx_base_vaddr + dynamic_off); /* GOT[0] */
        buf_u64(&got, 0); /* GOT[1] */
        buf_u64(&got, 0); /* GOT[2] */
        for (int i = 0; i < num_ext; i++)
            buf_u64(&got, code_vaddr + plt_entry_off[i] + 6); /* push $i addr */
        for (int j = 0; j < num_data_ext; j++)
            buf_u64(&got, data_got_external[j] ? 0 : data_got_addr[j]);

        /* ---- Build ELF ---- */
        /* NOTE: phnum computed AFTER buffer_init() to avoid a codegen
         * limitation where a value in a caller-saved register is not spilled
         * across a function call. */
        Buffer elf;
        buffer_init(&elf);
        uint16_t phnum = 4; /* RX, RW, INTERP, DYNAMIC */
        write_ehdr(&elf, entry, ELF64_EHDR_SIZE, phnum);
        write_phdr(&elf, PT_LOAD, PF_R | PF_X, 0, base, rx_filesz, rx_filesz, PAGE_SIZE);
        write_phdr(&elf, PT_LOAD, PF_R | PF_W, data_file_offset, data_vaddr,
                   got_data_off + got_bytes,
                   got_data_off + got_bytes + bss_size, PAGE_SIZE);
        write_phdr(&elf, PT_INTERP, PF_R, hdr_size + interp_off, rx_base_vaddr + interp_off,
                   interp_len, interp_len, 1);
        write_phdr(&elf, PT_DYNAMIC, PF_R, hdr_size + dynamic_off, rx_base_vaddr + dynamic_off,
                   dynamic.len, dynamic.len, 8);
        buf_bytes(&elf, rx.data, rx.len);
        while (elf.len < data_file_offset) buf_u8(&elf, 0);
        buf_bytes(&elf, data.data, data.len);
        while (elf.len < data_file_offset + got_data_off) buf_u8(&elf, 0);
        buf_bytes(&elf, got.data, got.len);
        SectionLayout lay;
        lay.code_vaddr = code_vaddr;
        lay.data_vaddr = data_vaddr;
        lay.bss_vaddr = bss_vaddr;
        lay.text_offset = text_offset;
        lay.data_file_offset = data_file_offset;
        lay.text_len = text.len;
        lay.rodata_len = rodata.len;
        lay.data_len = data.len;
        lay.bss_file_offset = bss_file_offset;
        lay.bss_size = bss_size;
        finalize_sections(&elf, mods, n, mod_text_off, mod_sym_base, sym_addr,
                          &lay, entry, want_debug);

        FILE *f = fopen(path, "wb");
        if (!f) { fprintf(stderr, "fakecc: cannot write '%s'\n", path); exit(1); }
        fwrite(elf.data, 1, elf.len, f);
        fclose(f);
        chmod(path, 0755);

        buffer_free(&rx); buffer_free(&got); buffer_free(&elf);
    } else {
        /* ---- Static executable (no truly-external symbols) ---- */
        uint64_t entry = base + start_offset;
        Buffer rx;
        buffer_init(&rx);
        gen_start(&rx, base + start_offset, main_addr, exit_static_addr);
        buf_bytes(&rx, text.data, text.len);
        buf_bytes(&rx, rodata.data, rodata.len);

        /* Optional GOT for cross-module data (GOTPCREL filled at link time). */
        size_t got_bytes = 0;
        Buffer got;
        buffer_init(&got);
        if (num_data_ext > 0) {
            got_bytes = (size_t)(3 + num_data_ext) * 8;
            buf_u64(&got, 0);
            buf_u64(&got, 0);
            buf_u64(&got, 0);
            for (int j = 0; j < num_data_ext; j++)
                buf_u64(&got, data_got_addr[j]);
        }

        Buffer elf;
        buffer_init(&elf);
        int has_rw = (data.len > 0 || bss_size > 0 || got_bytes > 0);
        uint16_t phnum = has_rw ? 2 : 1;
        write_ehdr(&elf, entry, ELF64_EHDR_SIZE, phnum);
        write_phdr(&elf, PT_LOAD, PF_R | PF_X, 0, base,
                   rx_filesz, rx_filesz, PAGE_SIZE);
        if (has_rw) {
            write_phdr(&elf, PT_LOAD, PF_R | PF_W, data_file_offset, data_vaddr,
                       bss_data_off, bss_data_off + bss_size, PAGE_SIZE);
        }
        while (elf.len < hdr_size)
            buf_u8(&elf, 0);
        buf_bytes(&elf, rx.data, rx.len);
        if (has_rw) {
            while (elf.len < data_file_offset) buf_u8(&elf, 0);
            buf_bytes(&elf, data.data, data.len);
            if (got_bytes > 0)
                while (elf.len < data_file_offset + got_data_off) buf_u8(&elf, 0);
            buf_bytes(&elf, got.data, got.len);
        }
        SectionLayout lay;
        lay.code_vaddr = code_vaddr;
        lay.data_vaddr = data_vaddr;
        lay.bss_vaddr = bss_vaddr;
        lay.text_offset = text_offset;
        lay.data_file_offset = data_file_offset;
        lay.text_len = text.len;
        lay.rodata_len = rodata.len;
        lay.data_len = data.len;
        lay.bss_file_offset = bss_file_offset;
        lay.bss_size = bss_size;
        finalize_sections(&elf, mods, n, mod_text_off, mod_sym_base, sym_addr,
                          &lay, entry, want_debug);

        FILE *f = fopen(path, "wb");
        if (!f) { fprintf(stderr, "fakecc: cannot write '%s'\n", path); exit(1); }
        fwrite(elf.data, 1, elf.len, f);
        fclose(f);
        chmod(path, 0755);

        buffer_free(&rx); buffer_free(&got); buffer_free(&elf);
    }

    buffer_free(&dynstr); buffer_free(&dynsym); buffer_free(&hash);
    buffer_free(&rela_plt); buffer_free(&rela_dyn); buffer_free(&dynamic);
    for (int i = 0; i < num_ext; i++) free(ext_list[i]);
    free(ext_list);
    for (int j = 0; j < num_data_ext; j++) free(data_ext_list[j]);
    free(data_ext_list);
    for (int i = 0; i < num_needed; i++) free(needed[i]);
    free(needed);
    free(runpath);
    buffer_free(&text); buffer_free(&rodata); buffer_free(&data);
    free(mod_text_off); free(mod_rodata_off); free(mod_data_off); free(mod_bss_off);
    free(mod_sym_base); free(sym_addr); free(sinfo); free(reloc_ext_idx);
    free(reloc_data_got_idx);
    free(plt_entry_off); free(plt_got_fixup);
    free(data_got_addr); free(data_got_external);
}
