#ifndef FAKECC_LDSCRIPT_H
#define FAKECC_LDSCRIPT_H

#include <stdint.h>

/* Subset GNU ld linker-script parser for fakeos S1 (-T / --script=).
 * Understands only the constructs used by boot/linker.ld and user/user.ld.
 * Arbitrary scripts beyond this subset are rejected. */

typedef enum {
    LDSCRIPT_KIND_UNKNOWN = 0,
    LDSCRIPT_KIND_KERNEL_HH, /* higher-half dual segment (boot/linker.ld) */
    LDSCRIPT_KIND_USER       /* fixed USER_VMA + PHDRS (user/user.ld) */
} LdScriptKind;

typedef struct {
    LdScriptKind kind;
    char entry[64];          /* ENTRY(sym); empty if absent */
    char output_format[64];  /* OUTPUT_FORMAT(...); informational */
    uint64_t kernel_vma;
    uint64_t kernel_lma;
    uint64_t user_vma;
    int has_kernel_vma;
    int has_kernel_lma;
    int has_user_vma;
    int has_phdrs;
    int text_phdr_flags;     /* PHDRS text FLAGS(n); -1 if absent */
    int data_phdr_flags;     /* PHDRS data FLAGS(n); -1 if absent */
    int saw_dot_plus_kernel_vma;
    int saw_at_addr_minus_kernel_vma;
    int saw_boot_section;
    int saw_keep_multiboot;
} LdScriptInfo;

/* Parse a script file or in-memory source. Returns 0 on success, -1 on
 * I/O / syntax error (message on stderr). Fills *out and classifies kind. */
int ldscript_parse_file(const char *path, LdScriptInfo *out);
int ldscript_parse_string(const char *src, const char *name, LdScriptInfo *out);

/* Configure existing link backends from a parsed script:
 *   KERNEL_HH → *out_kernel_multiboot = 1 (and nostdlib implied by caller)
 *   USER      → link_set_image_base(user_vma)
 * Returns 0 on success, -1 if kind is unknown / unsupported constants. */
int ldscript_configure_link(const LdScriptInfo *info, int *out_kernel_multiboot);

#endif /* FAKECC_LDSCRIPT_H */
