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
    m->symbols = NULL;
    m->num_symbols = 0;
    m->cap_symbols = 0;
}

void emit_module_free(EmitModule *m) {
    for (size_t i = 0; i < m->num_symbols; i++) {
        free(m->symbols[i].name);
    }
    free(m->symbols);
    buffer_free(&m->code);
}

/* Push a symbol into the table */
void emit_module_add_symbol(EmitModule *m, const char *name, size_t offset, size_t size) {
    if (m->num_symbols >= m->cap_symbols) {
        size_t new_cap = m->cap_symbols ? m->cap_symbols * 2 : 8;
        m->symbols = realloc(m->symbols, new_cap * sizeof(EmitSymbol));
        if (!m->symbols) {
            fprintf(stderr, "fakecc: out of memory\n");
            exit(1);
        }
        m->cap_symbols = new_cap;
    }
    m->symbols[m->num_symbols].name = xstrdup(name);
    m->symbols[m->num_symbols].offset = offset;
    m->symbols[m->num_symbols].size = size;
    m->num_symbols++;
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
#define PF_X            1
#define PF_R            4

#define ELF_BASE        0x400000
#define PAGE_SIZE       0x1000

/* Sizes */
#define ELF64_EHDR_SIZE  64
#define ELF64_PHDR_SIZE  56

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
/* emit_elf — write complete ELF executable                            */
/* ------------------------------------------------------------------ */

void emit_elf(const EmitModule *m, const char *output_path) {
    int main_idx = find_symbol(m, "main");
    if (main_idx < 0) {
        fprintf(stderr, "fakecc: no 'main' function found\n");
        exit(1);
    }

    /* Compute layout:
     *   ELF header:    64 bytes  (offset 0)
     *   Program header: 56 bytes (offset 64)
     *   _start stub:   14 bytes  (offset 120 = 0x78)
     *   function code: follows   (offset 134 = 0x86)
     */
    size_t hdr_size = ELF64_EHDR_SIZE + ELF64_PHDR_SIZE;
    size_t start_offset = hdr_size;
    size_t code_offset = start_offset + START_SIZE;

    /* Adjust main symbol offset: originally relative to code buffer start,
     * now shifted by _start size */
    size_t main_file_offset = code_offset + m->symbols[main_idx].offset;
    size_t total_code_size = START_SIZE + m->code.len;
    size_t total_file_size = hdr_size + total_code_size;

    uint64_t base = ELF_BASE;
    uint64_t entry = base + start_offset;

    /* Build the ELF file in a buffer */
    Buffer elf;
    buffer_init(&elf);

    /* 1. ELF header */
    write_ehdr(&elf, entry, ELF64_EHDR_SIZE, 1);

    /* 2. Program header — load entire file as one readable+executable segment */
    write_phdr(&elf,
               PT_LOAD,                            /* type */
               PF_R | PF_X,                        /* flags */
               0,                                   /* offset */
               base,                                /* vaddr */
               total_file_size,                     /* filesz */
               total_file_size,                     /* memsz */
               PAGE_SIZE);                          /* align */

    /* 3. _start stub */
    gen_start(&elf, start_offset, main_file_offset);

    /* 4. Function machine code */
    buffer_append(&elf, m->code.data, m->code.len);

    /* Write to file */
    FILE *f = fopen(output_path, "wb");
    if (!f) {
        fprintf(stderr, "fakecc: cannot write '%s'\n", output_path);
        exit(1);
    }
    fwrite(elf.data, 1, elf.len, f);
    fclose(f);

    /* Make executable */
    chmod(output_path, 0755);

    buffer_free(&elf);
}
