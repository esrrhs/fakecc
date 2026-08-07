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
    buffer_init(&m->code);
    buffer_init(&m->data);
    m->symbols = NULL; m->num_symbols = 0; m->cap_symbols = 0;
    m->globals = NULL; m->num_globals = 0; m->cap_globals = 0;
    m->relocs  = NULL; m->num_relocs  = 0; m->cap_relocs  = 0;
    m->ext_names = NULL; m->num_ext = 0; m->cap_ext = 0;
    m->plt_entry_off = NULL;
    m->plt_got_fixups = NULL; m->num_plt_got_fixups = 0; m->cap_plt_got_fixups = 0;
}

void emit_module_free(EmitModule *m) {
    for (size_t i = 0; i < m->num_symbols; i++) free(m->symbols[i].name);
    free(m->symbols);
    for (size_t i = 0; i < m->num_globals; i++) free(m->globals[i].name);
    free(m->globals);
    for (size_t i = 0; i < m->num_relocs; i++) free(m->relocs[i].target_name);
    free(m->relocs);
    for (size_t i = 0; i < m->num_ext; i++) free(m->ext_names[i]);
    free(m->ext_names);
    free(m->plt_entry_off);
    free(m->plt_got_fixups);
    buffer_free(&m->code);
    buffer_free(&m->data);
}

void emit_module_add_symbol(EmitModule *m, const char *name, size_t offset, size_t size) {
    if (m->num_symbols >= m->cap_symbols) {
        size_t new_cap = m->cap_symbols ? m->cap_symbols * 2 : 8;
        m->symbols = realloc(m->symbols, new_cap * sizeof(EmitSymbol));
        if (!m->symbols) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->cap_symbols = new_cap;
    }
    m->symbols[m->num_symbols].name = xstrdup(name);
    m->symbols[m->num_symbols].offset = offset;
    m->symbols[m->num_symbols].size = size;
    m->num_symbols++;
}

void emit_module_add_global(EmitModule *m, const char *name, size_t offset,
                            size_t size, int is_readonly) {
    if (m->num_globals >= m->cap_globals) {
        size_t nc = m->cap_globals ? m->cap_globals * 2 : 8;
        m->globals = realloc(m->globals, nc * sizeof(EmitGlobal));
        if (!m->globals) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->cap_globals = nc;
    }
    m->globals[m->num_globals].name = xstrdup(name);
    m->globals[m->num_globals].offset = offset;
    m->globals[m->num_globals].size = size;
    m->globals[m->num_globals].is_readonly = is_readonly;
    m->num_globals++;
}

void emit_module_add_reloc(EmitModule *m, size_t patch_off, const char *target_name) {
    if (m->num_relocs >= m->cap_relocs) {
        size_t nc = m->cap_relocs ? m->cap_relocs * 2 : 8;
        m->relocs = realloc(m->relocs, nc * sizeof(EmitGlobalReloc));
        if (!m->relocs) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->cap_relocs = nc;
    }
    m->relocs[m->num_relocs].patch_off = patch_off;
    m->relocs[m->num_relocs].target_name = xstrdup(target_name);
    m->num_relocs++;
}

size_t emit_module_add_external(EmitModule *m, const char *name) {
    for (size_t i = 0; i < m->num_ext; i++)
        if (strcmp(m->ext_names[i], name) == 0) return i;
    if (m->num_ext >= m->cap_ext) {
        size_t nc = m->cap_ext ? m->cap_ext * 2 : 8;
        m->ext_names = realloc(m->ext_names, nc * sizeof(char *));
        m->plt_entry_off = realloc(m->plt_entry_off, nc * sizeof(size_t));
        if (!m->ext_names || !m->plt_entry_off) {
            fprintf(stderr, "fakecc: OOM\n"); exit(1);
        }
        m->cap_ext = nc;
    }
    size_t idx = m->num_ext++;
    m->ext_names[idx] = xstrdup(name);
    m->plt_entry_off[idx] = 0;
    return idx;
}

void emit_module_set_plt_entry(EmitModule *m, size_t idx, size_t code_off) {
    if (idx < m->num_ext) m->plt_entry_off[idx] = code_off;
}

void emit_module_add_plt_got_fixup(EmitModule *m, size_t patch_off, size_t got_slot) {
    if (m->num_plt_got_fixups >= m->cap_plt_got_fixups) {
        size_t nc = m->cap_plt_got_fixups ? m->cap_plt_got_fixups * 2 : 8;
        m->plt_got_fixups = realloc(m->plt_got_fixups, nc * sizeof(PltGOTFixup));
        if (!m->plt_got_fixups) { fprintf(stderr, "fakecc: OOM\n"); exit(1); }
        m->cap_plt_got_fixups = nc;
    }
    m->plt_got_fixups[m->num_plt_got_fixups].patch_off = patch_off;
    m->plt_got_fixups[m->num_plt_got_fixups].got_slot = got_slot;
    m->num_plt_got_fixups++;
}

/* Find a global by name; returns index or -1. */
static int find_global(const EmitModule *m, const char *name) {
    for (size_t i = 0; i < m->num_globals; i++)
        if (strcmp(m->globals[i].name, name) == 0) return (int)i;
    return -1;
}

/* Find a symbol by name, return index or -1 */
static int find_symbol(const EmitModule *m, const char *name) {
    for (size_t i = 0; i < m->num_symbols; i++) {
        if (strcmp(m->symbols[i].name, name) == 0) return (int)i;
    }
    return -1;
}

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

/* Sizes */
#define ELF64_EHDR_SIZE  64
#define ELF64_PHDR_SIZE  56

/* Dynamic-linking tag / type constants (SysV AMD64 ABI). */
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
#define DT_DEBUG        21
#define R_X86_64_JUMP_SLOT 7
#define STB_GLOBAL      1
#define SHN_UNDEF       0

/* The interpreter (dynamic linker) path. */
static const char INTERP_PATH[] = "/lib64/ld-linux-x86-64.so.2";

/* ------------------------------------------------------------------ */
/* ELF header writer                                                   */
/* ------------------------------------------------------------------ */

static void write_ehdr(Buffer *b, uint64_t entry, uint64_t phoff,
                       uint16_t phnum) {
    uint16_t val16;
    uint32_t val32;
    uint64_t val64;

    /* e_ident (16 bytes) */
    buffer_append(b, "\x7f" "ELF", 4);      /* EI_MAG */
    char ident[12];
    memset(ident, 0, 12);
    ident[0] = ELFCLASS64;                    /* EI_CLASS */
    ident[1] = ELFDATA2LSB;                   /* EI_DATA */
    ident[2] = EV_CURRENT;                    /* EI_VERSION */
    ident[3] = ELFOSABI_NONE;                 /* EI_OSABI */
    buffer_append(b, ident, 12);

    /* e_type */
    val16 = ET_EXEC;
    buffer_append(b, (const char *)&val16, 2);
    /* e_machine */
    val16 = EM_X86_64;
    buffer_append(b, (const char *)&val16, 2);
    /* e_version */
    val32 = EV_CURRENT;
    buffer_append(b, (const char *)&val32, 4);
    /* e_entry */
    buffer_append(b, (const char *)&entry, 8);
    /* e_phoff */
    buffer_append(b, (const char *)&phoff, 8);
    /* e_shoff = 0 */
    val64 = 0;
    buffer_append(b, (const char *)&val64, 8);
    /* e_flags */
    val32 = 0;
    buffer_append(b, (const char *)&val32, 4);
    /* e_ehsize */
    val16 = ELF64_EHDR_SIZE;
    buffer_append(b, (const char *)&val16, 2);
    /* e_phentsize */
    val16 = ELF64_PHDR_SIZE;
    buffer_append(b, (const char *)&val16, 2);
    /* e_phnum */
    buffer_append(b, (const char *)&phnum, 2);
    /* e_shentsize */
    val16 = 64;   /* standard ELF64 section header entry size */
    buffer_append(b, (const char *)&val16, 2);
    /* e_shnum */
    val16 = 0;
    buffer_append(b, (const char *)&val16, 2);
    /* e_shstrndx */
    val16 = 0;
    buffer_append(b, (const char *)&val16, 2);
}

/* ------------------------------------------------------------------ */
/* Program header writer                                               */
/* ------------------------------------------------------------------ */

static void write_phdr(Buffer *b, uint32_t type, uint32_t flags,
                       uint64_t offset, uint64_t vaddr,
                       uint64_t filesz, uint64_t memsz,
                       uint64_t align) {
    buffer_append(b, (const char *)&type, 4);
    buffer_append(b, (const char *)&flags, 4);
    buffer_append(b, (const char *)&offset, 8);
    buffer_append(b, (const char *)&vaddr, 8);
    uint64_t paddr = vaddr;
    buffer_append(b, (const char *)&paddr, 8);
    buffer_append(b, (const char *)&filesz, 8);
    buffer_append(b, (const char *)&memsz, 8);
    buffer_append(b, (const char *)&align, 8);
}

/* ------------------------------------------------------------------ */
/* _start stub generation                                              */
/* ------------------------------------------------------------------ */

/*
 * _start:
 *   call main        ; e8 <rel32>
 *   mov  %eax, %edi  ; 89 c7
 *   mov  $60, %eax   ; b8 3c 00 00 00
 *   syscall          ; 0f 05
 *
 * Total: 14 bytes
 */
#define START_SIZE  14
#define CALL_SIZE   5   /* e8 + rel32 */

static void gen_start(Buffer *code, size_t call_addr, size_t main_addr) {
    /* call main — relative offset from next instruction to main */
    uint8_t call_opcode = 0xe8;
    buffer_append(code, (const char *)&call_opcode, 1);

    int32_t rel = (int32_t)(main_addr - (call_addr + CALL_SIZE));
    buffer_append(code, (const char *)&rel, 4);

    /* mov %eax, %edi */
    uint8_t mov_reg[] = {0x89, 0xc7};
    buffer_append(code, (const char *)mov_reg, 2);

    /* mov $60, %eax */
    uint8_t mov_imm[] = {0xb8, 0x3c, 0x00, 0x00, 0x00};
    buffer_append(code, (const char *)mov_imm, 5);

    /* syscall */
    uint8_t syscall[] = {0x0f, 0x05};
    buffer_append(code, (const char *)syscall, 2);
}

/* ------------------------------------------------------------------ */
/* Dynamic-linking section builders & emitter                          */
/* ------------------------------------------------------------------ */

static void buf_u8(Buffer *b, uint8_t v) { buffer_append(b, (const char *)&v, 1); }
static void buf_bytes(Buffer *b, const char *d, size_t n) { buffer_append(b, d, n); }
static void buf_u16(Buffer *b, uint16_t v) { buffer_append(b, (const char *)&v, 2); }
static void buf_u32(Buffer *b, uint32_t v) { buffer_append(b, (const char *)&v, 4); }
static void buf_u64(Buffer *b, uint64_t v) { buffer_append(b, (const char *)&v, 8); }
static void buf_pad(Buffer *b, size_t n) { while (n--) buf_u8(b, 0); }

/* SYSV symbol hash. */
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

/* .dynstr: "\0libc.so.6\0<name0>\0<name1>\0...".  Records each external
 * name's offset (for .dynstr and the DT_NEEDED entry). */
static void build_dynstr(const EmitModule *m, Buffer *out, size_t *name_offsets) {
    buf_u8(out, 0);  /* empty string at index 0 */
    buf_bytes(out, "libc.so.6", sizeof("libc.so.6"));  /* incl. NUL; DT_NEEDED=1 */
    for (size_t i = 0; i < m->num_ext; i++) {
        name_offsets[i] = out->len;
        buf_bytes(out, m->ext_names[i], strlen(m->ext_names[i]) + 1);
    }
}

/* .dynsym: NULL symbol + one STB_GLOBAL/SHN_UNDEF entry per external. */
static void build_dynsym(const EmitModule *m, Buffer *out, const size_t *name_offsets) {
    buf_pad(out, 24);  /* NULL symbol (index 0) */
    for (size_t i = 0; i < m->num_ext; i++) {
        buf_u32(out, (uint32_t)name_offsets[i]);  /* st_name */
        buf_u8(out, STB_GLOBAL << 4);             /* st_info */
        buf_u8(out, 0);                            /* st_other */
        buf_u16(out, SHN_UNDEF);                   /* st_shndx */
        buf_u64(out, 0);                           /* st_value */
        buf_u64(out, 0);                           /* st_size */
    }
}

/* .hash: SYSV hash table.  nbucket is a small prime; each symbol is placed
 * in bucket[elf_hash(name) % nbucket] and linked via chain[]. */
static void build_hash(const EmitModule *m, Buffer *out) {
    size_t nsyms = 1 + m->num_ext;
    size_t nbucket = (nsyms < 2) ? 1 : 3;  /* tiny prime */
    uint32_t *bucket = calloc(nbucket, sizeof(uint32_t));
    uint32_t *chain = calloc(nsyms, sizeof(uint32_t));
    for (size_t i = 0; i < nsyms; i++) {
        const char *nm = (i == 0) ? "" : m->ext_names[i - 1];
        uint32_t b = (uint32_t)(elf_hash(nm) % nbucket);
        chain[i] = bucket[b];
        bucket[b] = (uint32_t)i;
    }
    buf_u32(out, (uint32_t)nbucket);
    buf_u32(out, (uint32_t)nsyms);
    for (size_t i = 0; i < nbucket; i++) buf_u32(out, bucket[i]);
    for (size_t i = 0; i < nsyms; i++) buf_u32(out, chain[i]);
    free(bucket);
    free(chain);
}

/* .rela.plt: one R_X86_64_JUMP_SLOT relocation per external. */
static void build_rela_plt(const EmitModule *m, Buffer *out, uint64_t got_vaddr) {
    for (size_t i = 0; i < m->num_ext; i++) {
        buf_u64(out, got_vaddr + (3 + i) * 8);  /* r_offset -> GOT[3+i] */
        buf_u64(out, ((uint64_t)(i + 1) << 32) | R_X86_64_JUMP_SLOT);  /* r_info */
        buf_u64(out, 0);                         /* r_addend */
    }
}

/* .dynamic: the DT_ entries ld.so needs. */
static void build_dynamic(const EmitModule *m, Buffer *out,
                          uint64_t dynstr_vaddr, size_t dynstr_sz,
                          uint64_t dynsym_vaddr, uint64_t hash_vaddr,
                          uint64_t rela_plt_vaddr, size_t rela_plt_sz,
                          uint64_t got_vaddr) {
    buf_u64(out, DT_NEEDED);   buf_u64(out, 1);  /* libc.so.6 at dynstr[1] */
    buf_u64(out, DT_STRTAB);   buf_u64(out, dynstr_vaddr);
    buf_u64(out, DT_SYMTAB);   buf_u64(out, dynsym_vaddr);
    buf_u64(out, DT_SYMENT);   buf_u64(out, 24);
    buf_u64(out, DT_STRSZ);    buf_u64(out, dynstr_sz);
    buf_u64(out, DT_HASH);     buf_u64(out, hash_vaddr);
    buf_u64(out, DT_PLTGOT);   buf_u64(out, got_vaddr);
    buf_u64(out, DT_PLTRELSZ); buf_u64(out, rela_plt_sz);
    buf_u64(out, DT_PLTREL);   buf_u64(out, DT_RELA);
    buf_u64(out, DT_JMPREL);   buf_u64(out, rela_plt_vaddr);
    buf_u64(out, DT_NULL);     buf_u64(out, 0);
}

/* Emit a dynamically-linked ELF executable.  Layout (RX segment at offset 0):
 *   ELF header, program headers, _start, code(+PLT), .interp, .dynsym,
 *   .dynstr, .hash, .rela.plt, .dynamic.
 * RW segment (page-aligned): .data, .got.plt. */
static void emit_elf_dynamic(const EmitModule *m, const char *output_path) {
    size_t num_ext = m->num_ext;
    uint16_t phnum = 4;  /* RX PT_LOAD, RW PT_LOAD, PT_INTERP, PT_DYNAMIC */
    size_t hdr_size = ELF64_EHDR_SIZE + ELF64_PHDR_SIZE * phnum;
    size_t start_offset = hdr_size;
    size_t code_offset = start_offset + START_SIZE;

    int main_idx = find_symbol(m, "main");
    if (main_idx < 0) {
        fprintf(stderr, "fakecc: no 'main' function found\n");
        exit(1);
    }
    size_t main_file_offset = code_offset + m->symbols[main_idx].offset;

    /* Build the size-only dynamic sections first so we can compute the full
     * layout (dynstr/dynsym/hash don't depend on any vaddr). */
    size_t *name_offsets = malloc(num_ext * sizeof(size_t));
    Buffer dynstr, dynsym, hash, rela_plt, dynamic;
    buffer_init(&dynstr); buffer_init(&dynsym); buffer_init(&hash);
    buffer_init(&rela_plt); buffer_init(&dynamic);

    build_dynstr(m, &dynstr, name_offsets);
    build_dynsym(m, &dynsym, name_offsets);
    build_hash(m, &hash);
    size_t interp_len = sizeof(INTERP_PATH);  /* includes NUL */

    /* Compute the RX segment size analytically (section sizes are known;
     * only rela_plt/dynamic *content* needs vaddrs, computed afterward). */
    size_t rx_content_len = START_SIZE + m->code.len + interp_len
        + dynstr.len + dynsym.len + hash.len
        + 24 * num_ext   /* rela_plt */
        + 11 * 16;       /* dynamic (11 DT_ entries) */
    size_t rx_filesz = hdr_size + rx_content_len;
    size_t data_file_offset = rx_filesz;
    if (data_file_offset & (PAGE_SIZE - 1))
        data_file_offset = (data_file_offset + PAGE_SIZE - 1) & ~(size_t)(PAGE_SIZE - 1);
    uint64_t base = ELF_BASE;
    uint64_t data_vaddr = base + data_file_offset;
    size_t got_data_off = m->data.len;
    while (got_data_off & 7) got_data_off++;  /* 8-align GOT */
    uint64_t got_vaddr = data_vaddr + got_data_off;
    uint64_t code_vaddr = base + code_offset;

    /* Patch PLT `jmp *GOT(%rip)` rel32 fields in the code buffer NOW, before
     * the buffer is copied into the RX segment.  target = GOT slot vaddr. */
    for (size_t i = 0; i < m->num_plt_got_fixups; i++) {
        size_t patch_off = m->plt_got_fixups[i].patch_off;
        size_t slot = m->plt_got_fixups[i].got_slot;
        uint64_t target = got_vaddr + slot * 8;
        uint64_t rip_next = code_vaddr + patch_off + 4;
        int32_t disp = (int32_t)((int64_t)target - (int64_t)(rip_next));
        memcpy(((Buffer *)&m->code)->data + patch_off, &disp, 4);
    }

    /* Patch global relocs (IR_GADDR -> .data) while we still mutate code. */
    for (size_t i = 0; i < m->num_relocs; i++) {
        int gi = find_global(m, m->relocs[i].target_name);
        if (gi < 0) {
            fprintf(stderr, "fakecc: unresolved global '%s'\n", m->relocs[i].target_name);
            exit(1);
        }
        uint64_t target = data_vaddr + m->globals[gi].offset;
        uint64_t patch_vaddr = base + code_offset + m->relocs[i].patch_off;
        int32_t disp = (int32_t)((int64_t)target - (int64_t)(patch_vaddr + 4));
        memcpy(((Buffer *)&m->code)->data + m->relocs[i].patch_off, &disp, 4);
    }

    /* Build rela_plt and dynamic (their content needs the vaddrs). */
    uint64_t rx_base_vaddr = base + hdr_size;
    size_t dynstr_off = START_SIZE + m->code.len + interp_len;
    size_t dynsym_off = dynstr_off + dynstr.len;
    size_t hash_off = dynsym_off + dynsym.len;
    size_t rela_plt_off = hash_off + hash.len;
    size_t dynamic_off = rela_plt_off + 24 * num_ext;

    build_rela_plt(m, &rela_plt, got_vaddr);
    build_dynamic(m, &dynamic, rx_base_vaddr + dynstr_off, dynstr.len,
                  rx_base_vaddr + dynsym_off, rx_base_vaddr + hash_off,
                  rx_base_vaddr + rela_plt_off, rela_plt.len, got_vaddr);

    /* Build the GOT. */
    size_t got_count = 3 + num_ext;
    size_t got_bytes = got_count * 8;
    Buffer got;
    buffer_init(&got);
    buf_u64(&got, rx_base_vaddr + dynamic_off);  /* GOT[0] = .dynamic vaddr */
    buf_u64(&got, 0);                    /* GOT[1] = link_map (runtime) */
    buf_u64(&got, 0);                    /* GOT[2] = _dl_runtime_resolve (runtime) */
    for (size_t i = 0; i < num_ext; i++)
        buf_u64(&got, code_vaddr + m->plt_entry_off[i] + 6);  /* push $i addr */

    /* Assemble the RX segment content. */
    Buffer rx;
    buffer_init(&rx);
    gen_start(&rx, start_offset, main_file_offset);
    buf_bytes(&rx, m->code.data, m->code.len);  /* code + PLT (now patched) */
    size_t interp_off = rx.len;
    buf_bytes(&rx, INTERP_PATH, interp_len);
    buf_bytes(&rx, dynstr.data, dynstr.len);
    buf_bytes(&rx, dynsym.data, dynsym.len);
    buf_bytes(&rx, hash.data, hash.len);
    buf_bytes(&rx, rela_plt.data, rela_plt.len);
    buf_bytes(&rx, dynamic.data, dynamic.len);

    uint64_t entry = base + start_offset;

    /* Assemble the ELF. */
    Buffer elf;
    buffer_init(&elf);
    write_ehdr(&elf, entry, ELF64_EHDR_SIZE, phnum);

    /* Program headers. */
    write_phdr(&elf, PT_LOAD, PF_R | PF_X, 0, base, rx_filesz, rx_filesz, PAGE_SIZE);
    write_phdr(&elf, PT_LOAD, PF_R | PF_W, data_file_offset, data_vaddr,
               m->data.len + got_bytes, m->data.len + got_bytes, PAGE_SIZE);
    write_phdr(&elf, PT_INTERP, PF_R, hdr_size + interp_off, rx_base_vaddr + interp_off,
               interp_len, interp_len, 1);
    write_phdr(&elf, PT_DYNAMIC, PF_R, hdr_size + dynamic_off, rx_base_vaddr + dynamic_off,
               dynamic.len, dynamic.len, 8);

    /* RX content. */
    buf_bytes(&elf, rx.data, rx.len);

    /* Pad + RW content (.data + .got.plt). */
    while (elf.len < data_file_offset) buf_u8(&elf, 0);
    buf_bytes(&elf, m->data.data, m->data.len);
    buf_bytes(&elf, got.data, got.len);

    FILE *f = fopen(output_path, "wb");
    if (!f) { fprintf(stderr, "fakecc: cannot write '%s'\n", output_path); exit(1); }
    fwrite(elf.data, 1, elf.len, f);
    fclose(f);
    chmod(output_path, 0755);

    free(name_offsets);
    buffer_free(&dynstr); buffer_free(&dynsym); buffer_free(&hash);
    buffer_free(&rela_plt); buffer_free(&dynamic);
    buffer_free(&rx); buffer_free(&got); buffer_free(&elf);
}

/* ------------------------------------------------------------------ */
/* emit_elf — write complete ELF executable                            */
/* ------------------------------------------------------------------ */

void emit_elf(const EmitModule *m, const char *output_path) {
    /* External calls present -> emit a dynamically-linked executable. */
    if (m->num_ext > 0) {
        emit_elf_dynamic(m, output_path);
        return;
    }
    int main_idx = find_symbol(m, "main");
    if (main_idx < 0) {
        fprintf(stderr, "fakecc: no 'main' function found\n");
        exit(1);
    }

    int has_data = m->data.len > 0;

    /* Layout:
     *   ELF header:      64 bytes  (offset 0)
     *   Program headers: 56 bytes × phnum (offset 64)
     *   _start stub:     14 bytes  (right after headers)
     *   function code:   follows
     *   [padding to page-aligned vaddr, then]  data segment
     */
    uint16_t phnum = has_data ? 2 : 1;
    size_t hdr_size = ELF64_EHDR_SIZE + ELF64_PHDR_SIZE * phnum;
    size_t start_offset = hdr_size;
    size_t code_offset = start_offset + START_SIZE;
    size_t main_file_offset = code_offset + m->symbols[main_idx].offset;
    size_t total_code_size = START_SIZE + m->code.len;
    size_t code_end = hdr_size + total_code_size;

    /* Data segment: aligned to the next page inside the file AND same
     * congruence class modulo PAGE_SIZE for vaddr.  Simplest: pad file
     * offset up to PAGE_SIZE boundary, place data at
     * data_vaddr = base + data_file_offset (same page-aligned value). */
    size_t data_file_offset = code_end;
    if (has_data && (data_file_offset & (PAGE_SIZE - 1)) != 0)
        data_file_offset = (data_file_offset + PAGE_SIZE - 1) & ~(size_t)(PAGE_SIZE - 1);

    uint64_t base = ELF_BASE;
    uint64_t entry = base + start_offset;
    uint64_t data_vaddr = base + data_file_offset;

    /* Resolve global relocations: patch code buffer's rel32 slots so
     * disp = target_vaddr - (patch_vaddr + 4).
     * patch_vaddr = base + code_offset + m->relocs[i].patch_off. */
    for (size_t i = 0; i < m->num_relocs; i++) {
        int gi = find_global(m, m->relocs[i].target_name);
        if (gi < 0) {
            fprintf(stderr, "fakecc: unresolved global '%s'\n",
                    m->relocs[i].target_name);
            exit(1);
        }
        uint64_t target = data_vaddr + m->globals[gi].offset;
        uint64_t patch_vaddr = base + code_offset + m->relocs[i].patch_off;
        int32_t disp = (int32_t)((int64_t)target - (int64_t)(patch_vaddr + 4));
        /* Write to code buffer.  Cast away const — the emitter treats the
         * code buffer as mutable during ELF finalize; caller passes const
         * only by convention. */
        char *dst = ((Buffer *)&m->code)->data + m->relocs[i].patch_off;
        memcpy(dst, &disp, 4);
    }

    Buffer elf;
    buffer_init(&elf);

    /* 1. ELF header */
    write_ehdr(&elf, entry, ELF64_EHDR_SIZE, phnum);

    /* 2. Program headers */
    write_phdr(&elf, PT_LOAD, PF_R | PF_X, 0, base,
               code_end, code_end, PAGE_SIZE);
    if (has_data) {
        write_phdr(&elf, PT_LOAD, PF_R | PF_W /* rw */,
                   data_file_offset, data_vaddr,
                   m->data.len, m->data.len, PAGE_SIZE);
    }

    /* 3. _start stub */
    gen_start(&elf, start_offset, main_file_offset);

    /* 4. Function machine code */
    buffer_append(&elf, m->code.data, m->code.len);

    /* 5. Pad + data segment (if any) */
    if (has_data) {
        while (elf.len < data_file_offset) {
            char zero = 0;
            buffer_append(&elf, &zero, 1);
        }
        buffer_append(&elf, m->data.data, m->data.len);
    }

    FILE *f = fopen(output_path, "wb");
    if (!f) {
        fprintf(stderr, "fakecc: cannot write '%s'\n", output_path);
        exit(1);
    }
    fwrite(elf.data, 1, elf.len, f);
    fclose(f);
    chmod(output_path, 0755);
    buffer_free(&elf);
}
