#include "fakecc/emit.h"
#include "fakecc/common.h"

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

static void gen_start(Buffer *code, uint64_t call_vaddr, uint64_t main_vaddr) {
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
    uint8_t mov_imm[] = {0xb8, 0x3c, 0x00, 0x00, 0x00}; /* mov eax, 60 */
    buffer_append(code, (const char *)mov_imm, 5);
    uint8_t syscall[] = {0x0f, 0x05};
    buffer_append(code, (const char *)syscall, 2);
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

void emit_link(EmitModule **mods, size_t n, const char *path) {
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

    /* ---- PLT slot assignment for undefined referenced symbols (functions) */
    char **ext_list = NULL;
    int num_ext = 0;
    int *reloc_ext_idx = xcalloc(total_syms, sizeof(int));
    for (size_t i = 0; i < total_syms; i++) reloc_ext_idx[i] = -1;
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t r = 0; r < m->num_relocs; r++) {
            if (m->relocs[r].type == R_X86_64_GOTPCREL) continue; /* data, handled below */
            size_t gsi = mod_sym_base[i] + m->relocs[r].sym;
            if (!sinfo[gsi].defined) {
                const char *nm = m->syms[m->relocs[r].sym].name
                                 ? m->syms[m->relocs[r].sym].name : "";
                reloc_ext_idx[gsi] = ext_find_or_add(&ext_list, &num_ext, nm);
            }
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
    }

    /* ---- Build PLT at end of .text ---- */
    size_t *plt_entry_off = num_ext ? xcalloc(num_ext, sizeof(size_t)) : NULL;
    size_t *plt_got_fixup = num_ext ? xcalloc(num_ext, sizeof(size_t)) : NULL;
    size_t plt0_got_fixup[2];
    size_t plt0_off = emit_plt0(&text, plt0_got_fixup);
    for (int e = 0; e < num_ext; e++)
        plt_entry_off[e] = emit_plt_entry(&text, e, plt0_off, &plt_got_fixup[e]);

    /* ---- Build dynamic-linking section buffers (sizes needed for layout) ---- */
    Buffer dynstr, dynsym, hash, rela_plt, rela_dyn, dynamic;
    buffer_init(&dynstr); buffer_init(&dynsym); buffer_init(&hash);
    buffer_init(&rela_plt); buffer_init(&rela_dyn); buffer_init(&dynamic);
    size_t interp_len = (num_ext > 0 || num_data_ext > 0) ? sizeof(INTERP_PATH) : 0;

    /* External symbols that need dynsym entries: function externals (which get
     * PLT stubs) followed by data externals (which get GOT data slots).  Both
     * appear in dynstr/dynsym/hash so the dynamic linker can resolve them. */
    int num_dynsym_ext = num_ext + num_data_ext;
    if (num_dynsym_ext > 0) {
        buf_u8(&dynstr, 0);
        buf_bytes(&dynstr, "libc.so.6", sizeof("libc.so.6"));
        for (int i = 0; i < num_ext; i++)
            buf_bytes(&dynstr, ext_list[i], strlen(ext_list[i]) + 1);
        for (int j = 0; j < num_data_ext; j++)
            buf_bytes(&dynstr, data_ext_list[j], strlen(data_ext_list[j]) + 1);
        buf_pad(&dynsym, 24); /* [0] NULL symbol */
        for (int k = 0; k < num_dynsym_ext; k++) {
            buf_u32(&dynsym, 0); /* st_name patched below */
            buf_u8(&dynsym, STB_GLOBAL << 4);
            buf_u8(&dynsym, 0);
            buf_u16(&dynsym, SHN_UNDEF);
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
            /* r_offset patched after layout known */
            buf_u64(&rela_plt, 0);
            buf_u64(&rela_plt, ((uint64_t)(i + 1) << 32) | R_X86_64_JUMP_SLOT);
            buf_u64(&rela_plt, 0);
        }
        /* rela.dyn: R_X86_64_GLOB_DAT for each TRULY external data variable
         * (cross-module globals are filled statically, so need no reloc). */
        for (int j = 0; j < num_data_ext; j++) {
            if (!data_got_external[j]) continue;
            buf_u64(&rela_dyn, 0); /* r_offset patched after layout known */
            buf_u64(&rela_dyn, ((uint64_t)(1 + num_ext + j) << 32) | R_X86_64_GLOB_DAT);
            buf_u64(&rela_dyn, 0);
        }
        /* dynamic assembled after layout */
    }

    /* ---- Compute layout ---- */
    /* phnum is finalized after we know whether a data segment is needed;
     * reserve header space for the maximum (4 phdrs: RX, RW, INTERP, DYNAMIC)
     * so that segment file offsets are stable regardless of which are used. */
    uint16_t phnum_max = 4;
    size_t hdr_size = ELF64_EHDR_SIZE + ELF64_PHDR_SIZE * phnum_max;
    size_t start_offset = hdr_size;
    size_t text_offset = start_offset + START_SIZE;
    /* RX content: _start, .text, .rodata, .interp, .dynstr, .dynsym, .hash,
     * .rela.plt, .rela.dyn, .dynamic.  When there are no externals, the dynamic
     * sections are empty and only _start+.text+.rodata are loaded.  The .dynamic
     * section is assembled after layout (it needs vaddrs), so its size is
     * accounted for here: 11 DT_ entries for function externals (DT_NEEDED,
     * DT_STRTAB, DT_SYMTAB, DT_SYMENT, DT_STRSZ, DT_HASH, DT_PLTGOT,
     * DT_PLTRELSZ, DT_PLTREL, DT_JMPREL, DT_NULL) plus 3 more (DT_RELA,
     * DT_RELASZ, DT_RELENT) when external DATA variables are referenced. */
    size_t dynamic_size = 0;
    if (num_ext > 0 || num_data_ext > 0) {
        dynamic_size = (size_t)(11 + (num_data_ext > 0 ? 3 : 0)) * 16;
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
                sym_addr[gsi] = data_vaddr + data.len + mod_bss_off[i] + sym->value; break;
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

    /* ---- Patch PLT GOT fixups ---- */
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

    /* ---- Find main ---- */
    uint64_t main_addr = 0;
    int found_main = 0;
    for (size_t i = 0; i < n; i++) {
        EmitModule *m = mods[i];
        for (size_t j = 0; j < m->num_syms; j++) {
            const EmitSymbol *sym = &m->syms[j];
            if (sym->name && strcmp(sym->name, "main") == 0 &&
                sym->shndx == SECT_TEXT && sym->binding == 1 /* GLOBAL */) {
                main_addr = code_vaddr + mod_text_off[i] + sym->value;
                found_main = 1;
            }
        }
    }
    if (!found_main) {
        fprintf(stderr, "fakecc: no 'main' function found\n");
        exit(1);
    }

    /* ================================================================ */
    /* Finalize dynamic-linking sections now that layout is known        */
    /* ================================================================ */
    if (num_dynsym_ext > 0) {
        /* Patch dynsym st_name fields.  dynstr layout: "\0" + "libc.so.6\0"
         * + func names + data names, so name_k is at offset
         * 1 + sizeof("libc.so.6") + sum(len(name_j)+1, j<k). */
        {
            size_t acc = 1 + sizeof("libc.so.6");
            for (int k = 0; k < num_dynsym_ext; k++) {
                uint32_t noff = (uint32_t)acc;
                memcpy(dynsym.data + 24 + (size_t)k * 24, &noff, 4);
                const char *nm = (k < num_ext) ? ext_list[k] : data_ext_list[k - num_ext];
                acc += strlen(nm) + 1;
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
        buf_u64(&dynamic, DT_NEEDED);   buf_u64(&dynamic, 1);
        buf_u64(&dynamic, DT_STRTAB);   buf_u64(&dynamic, dynstr_vaddr);
        buf_u64(&dynamic, DT_SYMTAB);   buf_u64(&dynamic, rx_base_vaddr + dynsym_off);
        buf_u64(&dynamic, DT_SYMENT);   buf_u64(&dynamic, 24);
        buf_u64(&dynamic, DT_STRSZ);    buf_u64(&dynamic, dynstr.len);
        buf_u64(&dynamic, DT_HASH);     buf_u64(&dynamic, rx_base_vaddr + hash_off);
        buf_u64(&dynamic, DT_PLTGOT);   buf_u64(&dynamic, got_vaddr);
        buf_u64(&dynamic, DT_PLTRELSZ); buf_u64(&dynamic, rela_plt.len);
        buf_u64(&dynamic, DT_PLTREL);   buf_u64(&dynamic, DT_RELA);
        buf_u64(&dynamic, DT_JMPREL);   buf_u64(&dynamic, rx_base_vaddr + rela_plt_off);
        if (num_data_ext > 0) {
            buf_u64(&dynamic, DT_RELA);     buf_u64(&dynamic, rx_base_vaddr + rela_dyn_off);
            buf_u64(&dynamic, DT_RELASZ);   buf_u64(&dynamic, rela_dyn.len);
            buf_u64(&dynamic, DT_RELENT);   buf_u64(&dynamic, 24);
        }
        buf_u64(&dynamic, DT_NULL);     buf_u64(&dynamic, 0);

        /* ---- Assemble RX segment content ---- */
        Buffer rx;
        buffer_init(&rx);
        gen_start(&rx, base + start_offset, main_addr);
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
        uint16_t phnum = 4; /* RX, RW, INTERP, DYNAMIC */
        Buffer elf;
        buffer_init(&elf);
        write_ehdr(&elf, entry, ELF64_EHDR_SIZE, phnum);
        write_phdr(&elf, PT_LOAD, PF_R | PF_X, 0, base, rx_filesz, rx_filesz, PAGE_SIZE);
        write_phdr(&elf, PT_LOAD, PF_R | PF_W, data_file_offset, data_vaddr,
                   data.len + got_bytes, data.len + got_bytes, PAGE_SIZE);
        write_phdr(&elf, PT_INTERP, PF_R, hdr_size + interp_off, rx_base_vaddr + interp_off,
                   interp_len, interp_len, 1);
        write_phdr(&elf, PT_DYNAMIC, PF_R, hdr_size + dynamic_off, rx_base_vaddr + dynamic_off,
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
        /* ---- Static executable (no undefined symbols) ---- */
        uint64_t entry = base + start_offset;
        /* RX content: _start, .text, .rodata (read-only data lives in RX) */
        Buffer rx;
        buffer_init(&rx);
        gen_start(&rx, base + start_offset, main_addr);
        buf_bytes(&rx, text.data, text.len);
        buf_bytes(&rx, rodata.data, rodata.len);

        /* 1 phdr if no data, 2 if data/bss present.  The ehdr is written with
         * the real phnum, but we pad the header area out to hdr_size so that
         * the content (_start) lands at start_offset regardless. */
        uint16_t phnum = (data.len > 0 || bss_size > 0) ? 2 : 1;
        Buffer elf;
        buffer_init(&elf);
        write_ehdr(&elf, entry, ELF64_EHDR_SIZE, phnum);
        write_phdr(&elf, PT_LOAD, PF_R | PF_X, 0, base,
                   rx_filesz, rx_filesz, PAGE_SIZE);
        if (data.len > 0 || bss_size > 0) {
            /* data segment includes bss in memsz */
            write_phdr(&elf, PT_LOAD, PF_R | PF_W, data_file_offset, data_vaddr,
                       data.len, data.len + bss_size, PAGE_SIZE);
        }
        while (elf.len < hdr_size)
            buf_u8(&elf, 0);
        buf_bytes(&elf, rx.data, rx.len);
        if (data.len > 0 || bss_size > 0) {
            while (elf.len < data_file_offset) buf_u8(&elf, 0);
            buf_bytes(&elf, data.data, data.len);
            /* bss is zero-initialized by the OS; extend to data_file_offset + data.len + bss_size */
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
