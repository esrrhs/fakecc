#ifndef FAKECC_IASM_H
#define FAKECC_IASM_H

#include "fakecc/common.h"
#include "fakecc/emit.h"

/* Resolved GNU-asm operand ready for encoding. */
typedef struct {
    int kind;       /* 0=reg, 1=imm, 2=mem([reg]), 3=label/symbol name */
    int reg;        /* GP encoding 0..15 when kind is reg/mem */
    int64_t imm;
    int width;      /* 1/2/4/8 — preferred access width */
    char *sym;      /* owned copy when kind==3; else NULL */
} IAsmOperand;

/* Assemble one AT&T x86-64 GNU-asm template into `text`.
 * Operand placeholders %0/%1/… (and %b0/%w0/%k0/%q0 size overrides) are
 * replaced from `ops`.  `%%` becomes a literal `%`.
 *
 * Returns 0 on success.  Unknown mnemonics are skipped (return 0) so the
 * existing GCC compile-only corpus with dummy templates like asm("f") keeps
 * compiling; real fakeos templates are covered by the supported subset.
 * Hard errors (bad operand index, OOM) return -1 after die_at when loc set.
 *
 * For `jmp`/`call` to external symbols, emits PC32/PLT32 relocs via `out`.
 */
int iasm_emit_x86(Buffer *text, EmitModule *out,
                  const char *template,
                  const IAsmOperand *ops, int nops,
                  const char *file, int line, int col);

/* Assemble a gas-subset AT&T body into out->text and register globals
 * / externs on the EmitModule (fakeos S3 / fakecc -c foo.S). */
int iasm_assemble_body(EmitModule *out, const char *body,
                       const char **globals, int nglobals,
                       const char **externs, int nexterns);

/* Parse a gas-compatible .S / .s / .fakecc.S file into an EmitModule. */
int assemble_s_file(const char *src, const char *filename, EmitModule *out);

#endif /* FAKECC_IASM_H */
