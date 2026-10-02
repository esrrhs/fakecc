#ifndef FAKECC_MACHO_H
#define FAKECC_MACHO_H

/* Minimal Mach-O writer for arm64 (Apple Silicon) executables.
 *
 * Stage 1 scope (T4): a single-module PIE executable with one __TEXT
 * segment (__text section, which may embed read-only string data after
 * the code), one LC_MAIN entrypoint, dyld + a libSystem load command,
 * and an empty __LINKEDIT segment filled in by ad-hoc codesigning.
 * There is intentionally no nlist/reloc/bind machinery yet — freestanding
 * code needs no imports; the symbol table, external binds and dylib
 * support arrive in T14/T19. */

#include "fakecc/common.h"

#include <stdint.h>

struct EmitModule;

/* Write an arm64 Mach-O PIE executable from an emitted module:
 *   __TEXT : __text + __const (read-only globals/string literals)
 *   __DATA : __data (initialized globals) + __bss (zero-fill)
 * LC_MAIN entry_off is a file offset within __text. */
int macho_write_exec(const struct EmitModule *em, uint64_t entry_off,
                     const char *path);

/* Single-__text convenience used by low-level encoder tests. */
int macho_write_exec_text(const Buffer *text, uint64_t entry_off,
                          const char *path);

/* File offset at which the __text section starts (mach header + cmds). */
uint32_t macho_text_offset(void);

/* Apply an ad-hoc code signature in place via /usr/bin/codesign.
 * Required: arm64 macOS refuses to launch unsigned static(-ish) images.
 * Returns 0 on success. */
int macho_codesign(const char *path);

#endif /* FAKECC_MACHO_H */
