#include "fakecc/target.h"

#include <string.h>

/* This module is the one sanctioned place for host-platform detection:
 * the #ifdef below answers only "which target is the host default", it
 * never leaks into the neutral compiler layers. */

static const TargetDesc TARGET_X86_64_LINUX = {
    TARGET_ARCH_X86_64,
    TOS_LINUX,
    TARGET_OBJFMT_ELF,
    64,
    32,                    /* YMM: 32-byte vectors by default (-mavx) */
    "x86_64-linux",
};

static const TargetDesc TARGET_ARM64_MACOS = {
    TARGET_ARCH_ARM64,
    TOS_MACOS,
    TARGET_OBJFMT_MACHO,
    64,
    16,                    /* NEON Q registers are 16 bytes */
    "arm64-macos",
};

const TargetDesc *target_x86_64_linux(void) { return &TARGET_X86_64_LINUX; }
const TargetDesc *target_arm64_macos(void)  { return &TARGET_ARM64_MACOS; }

static const TargetDesc *host_default(void) {
#if defined(__APPLE__) && defined(__aarch64__)
    return &TARGET_ARM64_MACOS;
#else
    /* Linux x86_64, Intel macOS, and unknown hosts keep the historical
     * default (x86-64 ELF) until a second host backend exists. */
    return &TARGET_X86_64_LINUX;
#endif
}

static const TargetDesc *g_current = NULL;

const TargetDesc *target_current(void) {
    return g_current ? g_current : host_default();
}

void target_set_current(const TargetDesc *t) {
    g_current = t;
}

const TargetDesc *target_default(void) {
    return host_default();
}

/* Architecture token recognition; returns the arch enum or UNKNOWN. */
static TargetArch parse_arch(const char *tok, size_t len) {
    if ((len == 6 && strncmp(tok, "x86_64", 6) == 0) ||
        (len == 5 && strncmp(tok, "amd64", 5) == 0))
        return TARGET_ARCH_X86_64;
    if ((len == 5 && strncmp(tok, "arm64", 5) == 0) ||
        (len == 7 && strncmp(tok, "aarch64", 7) == 0))
        return TARGET_ARCH_ARM64;
    return TARGET_ARCH_UNKNOWN;
}

/* OS token recognition within a triple's remaining components. */
static TargetOS parse_os_token(const char *tok, size_t len) {
    if (len == 5 && strncmp(tok, "linux", 5) == 0)
        return TOS_LINUX;
    /* Version suffixes are common: "macos26", "darwin24.0.0". */
    if (len >= 5 && strncmp(tok, "macos", 5) == 0)
        return TOS_MACOS;
    if (len >= 6 && strncmp(tok, "darwin", 6) == 0)
        return TOS_MACOS;
    return TOS_UNKNOWN;
}

const TargetDesc *target_parse(const char *triple) {
    if (!triple || !*triple) return NULL;

    TargetArch arch = TARGET_ARCH_UNKNOWN;
    TargetOS os = TOS_UNKNOWN;

    /* Split on '-' in place over the const string.  First component is the
     * architecture; scan the remaining components for an OS token (so both
     * "arm64-macos" and "aarch64-apple-darwin24" parse). */
    const char *p = triple;
    int first = 1;
    while (*p) {
        const char *dash = strchr(p, '-');
        size_t len = dash ? (size_t)(dash - p) : strlen(p);
        if (first) {
            arch = parse_arch(p, len);
            first = 0;
        } else if (os == TOS_UNKNOWN) {
            os = parse_os_token(p, len);
        }
        if (!dash) break;
        p = dash + 1;
    }

    /* Only the two delivered/planned combinations are accepted:
     * x86_64-macos (Rosetta) and aarch64-linux are explicit non-goals. */
    if (arch == TARGET_ARCH_X86_64 && os == TOS_LINUX)
        return &TARGET_X86_64_LINUX;
    if (arch == TARGET_ARCH_ARM64 && os == TOS_MACOS)
        return &TARGET_ARM64_MACOS;
    return NULL;
}

int target_backend_ready(const TargetDesc *t) {
    if (!t) return 0;
    /* The arm64 Mach-O backend can emit runnable freestanding executables
     * from T5; -c object support lands in T14.  (Used to gate the driver.) */
    return 1;
}
