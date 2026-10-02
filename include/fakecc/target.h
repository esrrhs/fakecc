#ifndef FAKECC_TARGET_H
#define FAKECC_TARGET_H

/* ------------------------------------------------------------------ */
/* Compilation target description                                      */
/*                                                                      */
/* Every platform-specific decision in the compiler (instruction set,  */
/* syscall ABI, object file format, native vector width) hangs off a    */
/* single TargetDesc, selected once per invocation.  Neutral layers     */
/* (lexer/parser/sema/IR/optimizers) never inspect it; only the backend */
/* boundary (codegen / object emit / link / platform runtime headers)  */
/* dispatches on it.                                                    */
/* ------------------------------------------------------------------ */

/* Instruction-set architecture of the code generator backend. */
typedef enum {
    TARGET_ARCH_UNKNOWN = 0,
    TARGET_ARCH_X86_64  = 1,
    TARGET_ARCH_ARM64   = 2,   /* AArch64 (A64), Darwin variant */
} TargetArch;

/* Operating-system syscall/runtime ABI family.
 * Note: constants use the TOS_ prefix (not TARGET_OS_*) because the
 * TARGET_OS_* namespace is reserved by Apple's TargetConditionals.h. */
typedef enum {
    TOS_UNKNOWN = 0,
    TOS_LINUX   = 1,
    TOS_MACOS   = 2,
} TargetOS;

/* Object/executable file format produced by the object writer/linker. */
typedef enum {
    TARGET_OBJFMT_UNKNOWN = 0,
    TARGET_OBJFMT_ELF     = 1,
    TARGET_OBJFMT_MACHO   = 2,
} TargetObjFmt;

typedef struct {
    TargetArch   arch;
    TargetOS     os;
    TargetObjFmt objfmt;
    int          ptr_bits;             /* pointer width; 64 for both current targets */
    int          native_vector_bytes;  /* width of one native vector register/ABI slot:
                                          x86-64 default 32 (YMM; -mavx512f widens to 64),
                                          arm64 NEON 16 */
    const char  *triple;               /* canonical triple string */
} TargetDesc;

/* The process-wide current target.  Defaults to the host default
 * (target_default()) before target_set_current() is called.  The driver
 * sets it from --target (or the host default). */
const TargetDesc *target_current(void);
void              target_set_current(const TargetDesc *t);

/* Host-default target: arm64-macos on Apple Silicon, x86_64-linux on
 * Linux x86_64 (and on any other host, preserving the historical behavior
 * of always producing x86-64 ELF). */
const TargetDesc *target_default(void);

/* Parse a target triple.  Returns NULL if unrecognized or unsupported.
 *
 * Accepted spellings (case-sensitive):
 *   x86_64-linux | x86_64-unknown-linux[-gnu] | amd64[-...]-linux
 *   arm64-macos  | arm64-apple-macos[-...]    | aarch64-apple-darwin
 *                | arm64-darwin
 *
 * The only supported combinations are x86_64+linux and arm64+macos. */
const TargetDesc *target_parse(const char *triple);

/* Singleton descriptors. */
const TargetDesc *target_x86_64_linux(void);
const TargetDesc *target_arm64_macos(void);

/* True when this build of fakecc can actually compile and link the target.
 * The driver uses this to fail early with a clear message while a backend
 * is under construction. */
int target_backend_ready(const TargetDesc *t);

#endif /* FAKECC_TARGET_H */
