#ifndef FAKECC_PE_H
#define FAKECC_PE_H

#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* PE32+ executable linker + AMD64 COFF relocatable objects            */
/* ------------------------------------------------------------------ */

struct EmitModule;

/* One imported symbol resolved from an import library or a DLL export
 * directory.  `ordinal` is 0 when the import is by name.  `is_data`
 * selects an IAT slot with no code thunk (the loader stores the address
 * of the remote object). */
typedef struct PeImport {
    char *dll;
    char *name;
    int is_data;
    int ordinal;
    int hint;
} PeImport;

typedef struct PeLinkOpt {
    int shared;                 /* produce a DLL instead of an EXE */
    const PeImport *imports;
    size_t nimports;
    /* Import library path.  NULL writes lib<stem>.dll.a (and <stem>.lib)
     * next to the DLL.  Ignored for executables. */
    const char *implib_path;
} PeLinkOpt;

/* Link already-lowered modules into one PE32+ console executable.
 * Returns 0 on success.  On failure a message is printed and the
 * return value is non-zero. */
int pe_link(struct EmitModule **mods, size_t n, const char *path);
int pe_link_ex(struct EmitModule **mods, size_t n, const char *path,
               const PeLinkOpt *opt);

/* Lazily pull COFF archive / import-library members that define symbols
 * still undefined in `mods`.  `needed` entries are -l stems, ":exact"
 * filenames, or paths ending in .a/.lib/.dll/.dll.a.  Extracted objects
 * are appended to `*extra` (caller frees each with emit_module_free).
 * Import members become `*imports` instead of objects. */
int pe_pull_libraries(struct EmitModule **mods, size_t nmods,
                      const char **needed, size_t nneeded,
                      const char **lib_paths, size_t npaths,
                      struct EmitModule ***extra, size_t *nextra,
                      PeImport **imports, size_t *nimports);
void pe_imports_free(PeImport *imps, size_t n);

/* One symbol to publish in an import library. */
typedef struct PeImplibSym {
    const char *name;
    int is_data;
} PeImplibSym;

/* Write a MinGW GNU-style import library (lib<stem>.dll.a) and an MSVC
 * short-import library (<stem>.lib) for `dll_name` (e.g. "foo.dll").
 * `implib_path` overrides the .dll.a location; NULL places both next to
 * `dll_path`. */
int pe_write_import_libs(const char *dll_path, const char *implib_path,
                         const char *dll_name,
                         const PeImplibSym *syms, size_t nsyms);

/* Write / read an AMD64 COFF relocatable object (IMAGE_FILE_MACHINE_AMD64). */
int coff_write_object(const struct EmitModule *m, const char *path);
int coff_read_object(const char *path, struct EmitModule *m);
int coff_read_object_mem(const void *buf, size_t n, const char *name,
                         struct EmitModule *m);

/* ------------------------------------------------------------------ */
/* COFF / PE constants used by the object writer and tests             */
/* ------------------------------------------------------------------ */

#define IMAGE_FILE_MACHINE_AMD64           0x8664

#define IMAGE_SCN_CNT_CODE                 0x00000020u
#define IMAGE_SCN_CNT_INITIALIZED_DATA     0x00000040u
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA   0x00000080u
#define IMAGE_SCN_LNK_INFO                 0x00000200u
#define IMAGE_SCN_LNK_REMOVE               0x00000800u
#define IMAGE_SCN_LNK_COMDAT               0x00001000u
#define IMAGE_SCN_MEM_EXECUTE              0x20000000u
#define IMAGE_SCN_MEM_READ                 0x40000000u
#define IMAGE_SCN_MEM_WRITE                0x80000000u
#define IMAGE_SCN_ALIGN_1BYTES             0x00100000u
#define IMAGE_SCN_ALIGN_2BYTES             0x00200000u
#define IMAGE_SCN_ALIGN_4BYTES             0x00300000u
#define IMAGE_SCN_ALIGN_8BYTES             0x00400000u
#define IMAGE_SCN_ALIGN_16BYTES            0x00500000u
#define IMAGE_SCN_ALIGN_32BYTES            0x00600000u
#define IMAGE_SCN_ALIGN_64BYTES            0x00700000u

#define IMAGE_SYM_UNDEFINED                0
#define IMAGE_SYM_ABSOLUTE                 0xFFFF
#define IMAGE_SYM_DEBUG                    0xFFFE

#define IMAGE_SYM_CLASS_EXTERNAL           2
#define IMAGE_SYM_CLASS_STATIC             3
#define IMAGE_SYM_CLASS_LABEL              6
#define IMAGE_SYM_CLASS_FILE               103
#define IMAGE_SYM_CLASS_SECTION            104
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL      105

#define IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY 1
#define IMAGE_WEAK_EXTERN_SEARCH_LIBRARY   2
#define IMAGE_WEAK_EXTERN_SEARCH_ALIAS     3

#define IMAGE_COMDAT_SELECT_NODUPLICATES   1
#define IMAGE_COMDAT_SELECT_ANY            2
#define IMAGE_COMDAT_SELECT_SAME_SIZE      3
#define IMAGE_COMDAT_SELECT_EXACT_MATCH    4
#define IMAGE_COMDAT_SELECT_ASSOCIATIVE   5
#define IMAGE_COMDAT_SELECT_LARGEST       6

#define IMAGE_REL_AMD64_ABSOLUTE           0x0000
#define IMAGE_REL_AMD64_ADDR64             0x0001
#define IMAGE_REL_AMD64_ADDR32             0x0002
#define IMAGE_REL_AMD64_ADDR32NB           0x0003
#define IMAGE_REL_AMD64_REL32              0x0004
#define IMAGE_REL_AMD64_REL32_1            0x0005
#define IMAGE_REL_AMD64_REL32_2            0x0006
#define IMAGE_REL_AMD64_REL32_3            0x0007
#define IMAGE_REL_AMD64_REL32_4            0x0008
#define IMAGE_REL_AMD64_REL32_5            0x0009
#define IMAGE_REL_AMD64_SECTION            0x000A
#define IMAGE_REL_AMD64_SECREL             0x000B
#define IMAGE_REL_AMD64_SECREL7            0x000C
#define IMAGE_REL_AMD64_TOKEN              0x000D
#define IMAGE_REL_AMD64_SREL32             0x000E
#define IMAGE_REL_AMD64_PAIR               0x000F
#define IMAGE_REL_AMD64_SSPAN32            0x0010

/* Fakecc-private COFF reloc type: preserves R_X86_64_GOTPCREL across a
 * fakecc .obj round-trip (PE pe_link still consumes ELF-shaped types). */
#define IMAGE_REL_AMD64_FAKECC_GOTPCREL    0x8009

#endif /* FAKECC_PE_H */
