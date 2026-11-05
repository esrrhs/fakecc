/* Optional stubs when FAKECC_NO_MACHO is set (fakeos N1 Linux-only bootstrap).
 * Mach-O entry points are unreachable for --target=x86_64-linux; these exist
 * so a static link without src/macho.c still resolves. Signatures match
 * include/fakecc/macho.h. */
#include "fakecc/macho.h"
#include "fakecc/emit.h"

#include <stddef.h>

uint32_t macho_text_offset(void) { return 0; }

void macho_section_offsets(const EmitModule *em, size_t text_len,
                           MachoSectOffsets *off) {
    (void)em; (void)text_len;
    if (off) {
        off->ro = off->data = off->bss = 0;
        off->thread_vars = off->tdata = off->tbss = 0;
    }
}

int macho_write_exec_text(const Buffer *text, uint64_t entry_off,
                          const char *path) {
    (void)text; (void)entry_off; (void)path;
    return -1;
}

int macho_write_exec(const EmitModule *em, uint64_t entry_off,
                     const char *path, int skip_libsystem,
                     const MachoDebug *dbg) {
    (void)em; (void)entry_off; (void)path; (void)skip_libsystem; (void)dbg;
    return -1;
}

int macho_write_object(const EmitModule *em, const char *path) {
    (void)em; (void)path;
    return -1;
}

int macho_read_object(const char *path, EmitModule *em) {
    (void)path; (void)em;
    return -1;
}

int macho_read_dylib(const char *path, char **install_name_out,
                     char ***syms_out, size_t *nsyms_out) {
    (void)path;
    if (install_name_out) *install_name_out = NULL;
    if (syms_out) *syms_out = NULL;
    if (nsyms_out) *nsyms_out = 0;
    return -1;
}

size_t macho_tlv_block_tbss_base(size_t tdata_len, size_t max_align) {
    (void)tdata_len; (void)max_align;
    return 0;
}

int macho_link_objects(EmitModule **mods, size_t n, const char *path,
                       int skip_libsystem) {
    (void)mods; (void)n; (void)path; (void)skip_libsystem;
    return -1;
}

int macho_codesign(const char *path) {
    (void)path;
    return -1;
}
