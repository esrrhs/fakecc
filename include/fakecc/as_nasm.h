#ifndef FAKECC_AS_NASM_H
#define FAKECC_AS_NASM_H

/* NASM-syntax subset assembler for fakeos real-mode / mixed-bits sources.
 *
 * Enough for arch/x86_64/ap_tramp.asm (-f bin) and arch/x86_64/boot.asm
 * (-f elf64): BITS 16/32/64, ORG, sections, equ, times, far jumps,
 * lgdt, CR/MSR moves, and the ALU/branch forms those files use.
 */

typedef enum {
    NASM_FMT_BIN = 0,   /* flat binary (ORG-based) */
    NASM_FMT_ELF64 = 1  /* relocatable ELF64 with named sections */
} NasmFormat;

/* Assemble `src` and write `out_path`. Returns 0 on success, -1 on error. */
int assemble_nasm(const char *src, const char *filename,
                  NasmFormat fmt, const char *out_path);

/* True if path looks like NASM Intel-syntax input (.asm / .nasm). */
int is_nasm_input(const char *path);

#endif /* FAKECC_AS_NASM_H */
