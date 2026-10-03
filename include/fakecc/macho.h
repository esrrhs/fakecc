#ifndef FAKECC_MACHO_H
#define FAKECC_MACHO_H

/* Minimal Mach-O writer for arm64 (Apple Silicon) executables.
 *
 * Stage 1 scope (T4): a single-module PIE executable with one __TEXT
 * segment (__text section, which may embed read-only string data after
 * the code), one LC_MAIN entrypoint, dyld + a libSystem load command,
 * and an empty __LINKEDIT segment filled in by ad-hoc codesigning.
 * Pointer-valued initializers are dyld chained fixups
 * (LC_DYLD_CHAINED_FIXUPS, DYLD_CHAINED_PTR_64_OFFSET) in __DATA.
 * External binds, the symbol table and dylibs arrive in T14/T19. */

#include "fakecc/common.h"

#include <stdint.h>

struct EmitModule;

/* Write an arm64 Mach-O PIE executable from an emitted module:
 *   __TEXT : __text + __const (read-only globals/string literals)
 *   __DATA : __data (initialized globals) + __bss (zero-fill)
 * LC_MAIN entry_off is a file offset within __text. */
int macho_write_exec(const struct EmitModule *em, uint64_t entry_off,
                     const char *path);

/* Relocatable MH_OBJECT.  Defined symbols are recorded; intra-section
 * branches are already resolved.  Global-address relocations are not
 * emitted yet. */
int macho_write_object(const struct EmitModule *em, const char *path);

/* Read an MH_OBJECT written by macho_write_object back into a module.
 * Symbol names drop one leading underscore.  Returns 0 on success. */
int macho_read_object(const char *path, struct EmitModule *em);

/* Link object modules (each produced by -c) into one PIE executable.
 * Resolves BRANCH26, PAGE21/PAGEOFF12 and UNSIGNED against defined
 * symbols, then writes and returns 0.  The caller still ad-hoc signs. */
int macho_link_objects(struct EmitModule **mods, size_t n, const char *path);

/* Single-__text convenience used by low-level encoder tests. */
int macho_write_exec_text(const Buffer *text, uint64_t entry_off,
                          const char *path);

/* File offset at which the __text section starts (mach header + cmds). */
uint32_t macho_text_offset(void);

/* Image offsets (relative to the Mach-O base VA == file offsets for
 * __TEXT/__DATA file-backed content) at which __const, __data and __bss
 * land, given the final __text length.  Same layout macho_write_exec
 * uses.  An absent section yields 0. */
void macho_section_offsets(const struct EmitModule *em, size_t text_len,
                           uint64_t *ro_off, uint64_t *data_off,
                           uint64_t *bss_off);

/* Apply an ad-hoc code signature in place via /usr/bin/codesign.
 * Required: arm64 macOS refuses to launch unsigned static(-ish) images.
 * Returns 0 on success. */
int macho_codesign(const char *path);

#endif /* FAKECC_MACHO_H */
