#include "fakecc/macho.h"
#include "fakecc/emit.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
/* The bootstrap (FAKECC_SELFHOST) preprocesses with the minimal v0/fakeinc
 * stubs, which lack these headers; the selfhost image never invokes
 * codesign (see macho_codesign). */
#ifndef FAKECC_SELFHOST
#include <sys/wait.h>
#include <unistd.h>
#endif

/* Mach-O 64-bit structures and constants (Apple loader/mach-o headers).
 * Layout below was validated against lld/ld64 output and by executing
 * hand-built images (see .trae/specs/arm64-macos-backend spike notes). */

#define MH_MAGIC_64   0xFEEDFACFu
#define MH_EXECUTE    2u
#define MH_NOUNDEFS   0x00000001u
#define MH_DYLDLINK   0x00000004u
#define MH_TWOLEVEL   0x00000080u
#define MH_PIE        0x00200000u

#define CPU_TYPE_ARM64        0x0100000C
#define CPU_SUBTYPE_ARM64_ALL 0

#define LC_SEGMENT_64      0x19u
#define LC_BUILD_VERSION   0x32u
#define LC_UUID           0x1Bu
#define LC_LOAD_DYLINKER   0x0Eu
#define LC_LOAD_DYLIB      0x0Cu
#define LC_MAIN            0x80000028u
#define LC_DYLD_EXPORTS_TRIE 0x80000033u
#define LC_DYLD_CHAINED_FIXUPS 0x80000034u
#define LC_SYMTAB         0x02u
#define LC_DYSYMTAB       0x0Bu

#define PLATFORM_MACOS 1u

#define VM_PROT_READ  1
#define VM_PROT_WRITE 2
#define VM_PROT_EXEC  4

#define S_REGULAR                  0x00u
#define S_ATTR_PURE_INSTRUCTIONS   0x80000000u
#define S_ATTR_SOME_INSTRUCTIONS   0x00000400u

/* Fixed load-image geometry:
 *   mach_header_64 (32) + load commands, then __text at MACHO_TEXT_OFF.
 *   __PAGEZERO guards the low 4 GiB; __TEXT is one 16 KiB page and
 *   __LINKEDIT starts on the next page empty, populated by ad-hoc
 *   codesigning. */
/* arm64 macOS uses 16 KiB pages on current hardware/kernels; segment
 * offsets and file padding follow that. */
#define MACHO_BASE_VA   0x100000000ull
#define MACHO_PAGE_SIZE 0x4000u

#define DYLINKER_PATH "/usr/lib/dyld"
#define LIBSYSTEM_PATH "/usr/lib/libSystem.B.dylib"

#pragma pack(push, 1)

typedef struct {
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
} mach_header_64;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    char     segname[16];
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    int32_t  maxprot;
    int32_t  initprot;
    uint32_t nsects;
    uint32_t flags;
} segment_command_64;

typedef struct {
    char     sectname[16];
    char     segname[16];
    uint64_t addr;
    uint64_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
} section_64;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t platform;
    uint32_t minos;       /* packed x.y.z: (x<<16)|(y<<8)|z */
    uint32_t sdk;
    uint32_t ntools;
} build_version_command;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    uint64_t entryoff;
    uint64_t stacksize;
} entry_point_command;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t nameoff;
} dylinker_command;

typedef struct {
    uint32_t name_offset;
    uint32_t timestamp;
    uint32_t current_version;
    uint32_t compatibility_version;
} dylib;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    dylib    dylib;
    char     name[];      /* offset 24, NUL terminated */
} dylib_command;

#pragma pack(pop)

/* pad to a multiple of 8 for load-command sizes */
static uint32_t align8(uint32_t n) { return (n + 7u) & ~7u; }

/* mach_header_64 (32) + load commands, then __text at MACHO_TEXT_OFF.
 * 1024 leaves ample room for codesign's injected LC_CODE_SIGNATURE. */
#define MACHO_TEXT_OFF 1024u

uint32_t macho_text_offset(void) { return MACHO_TEXT_OFF; }

int macho_write_exec_text(const Buffer *text, uint64_t entry_off,
                          const char *path) {
    EmitModule em;
    memset(&em, 0, sizeof em);
    buffer_init(&em.text);
    buffer_append(&em.text, text->data, text->len);
    int rc = macho_write_exec(&em, entry_off, path);
    buffer_free(&em.text);
    return rc;
}

int macho_write_exec(const EmitModule *em, uint64_t entry_off,
                     const char *path) {
    const Buffer *text = &em->text;
    const Buffer *rodata = &em->rodata;
    const Buffer *data = &em->data;
    uint64_t bss_bytes = em->bss_size;

    /* ── Section/segment layout ── */
    const uint32_t text_off = MACHO_TEXT_OFF;
    uint32_t ro_off = 0;
    if (rodata->len) {
        ro_off = text_off + (uint32_t)text->len;
        size_t al = em->rodata_align > 16 ? em->rodata_align : 16;
        while (ro_off % al) ro_off++;
    }
    uint64_t text_used = ro_off ? (uint64_t)ro_off + rodata->len
                                : (uint64_t)text_off + text->len;
    uint64_t text_pages = (text_used + MACHO_PAGE_SIZE - 1)
                          & ~(uint64_t)(MACHO_PAGE_SIZE - 1);

    int data_present = (data->len > 0 || bss_bytes > 0);
    uint64_t data_page = text_pages;
    /* File-backed __data bytes occupy whole pages; __bss is a zerofill VM
     * tail with no file representation. */
    uint64_t data_file = data->len
        ? ((uint64_t)data->len + MACHO_PAGE_SIZE - 1)
          & ~(uint64_t)(MACHO_PAGE_SIZE - 1)
        : 0;
    uint64_t bss_vm = (bss_bytes + 15) & ~(uint64_t)15;
    uint64_t data_vm = data_file + bss_vm;
    /* __LINKEDIT begins on the next page after the whole __DATA VM image
     * (its file offset doubles as the codesignature injection site). */
    uint64_t link_off = data_present
        ? data_page + ((data_vm + MACHO_PAGE_SIZE - 1)
                       & ~(uint64_t)(MACHO_PAGE_SIZE - 1))
        : text_pages;

    /* ── Load command sizes ── */
    /* dyld rejects zero-size sections (their null addr sorts before the
     * segment), so each section — and the whole __DATA segment — is
     * emitted only when it has content. */
    const int has_const_sec = rodata->len > 0;
    const int has_data_sec  = data->len > 0;
    const int has_bss_sec   = bss_bytes > 0;
    const int n_text_secs   = 1 + (has_const_sec ? 1 : 0);
    const int n_data_secs   = (has_data_sec ? 1 : 0) + (has_bss_sec ? 1 : 0);

    const uint32_t seg_plain_cmd = sizeof(segment_command_64);
    const uint32_t seg_text_cmd  =
        sizeof(segment_command_64) + (uint32_t)n_text_secs * sizeof(section_64);
    const uint32_t seg_data_cmd  = data_present
        ? sizeof(segment_command_64) + (uint32_t)n_data_secs * sizeof(section_64)
        : 0;
    const uint32_t build_cmd = sizeof(build_version_command);
    const uint32_t main_cmd = sizeof(entry_point_command);

    const uint32_t dylinker_name_off = 12u;
    const uint32_t dylinker_cmdsz =
        align8(dylinker_name_off + (uint32_t)strlen(DYLINKER_PATH) + 1);

    const uint32_t dylib_name_off = 24u;
    const uint32_t dylib_cmdsz =
        align8(dylib_name_off + (uint32_t)strlen(LIBSYSTEM_PATH) + 1);

    /* PAGEZERO, __TEXT, [__DATA], __LINKEDIT + the five non-segment
     * commands (build version, uuid, main, dylinker, dylib). */
    const uint32_t ncmds = (data_present ? 4u : 3u) + 5u;
    const uint32_t uuid_cmd = 24;
    const uint32_t sizeofcmds =
        seg_plain_cmd + seg_text_cmd + seg_data_cmd + seg_plain_cmd
        + build_cmd + uuid_cmd + main_cmd
        + dylinker_cmdsz + dylib_cmdsz;

    const uint32_t cmds_end = (uint32_t)sizeof(mach_header_64) + sizeofcmds;
    if (cmds_end > MACHO_TEXT_OFF) {
        fprintf(stderr, "fakecc: Mach-O commands (%u) overflow header pad\n",
                cmds_end);
        return -1;
    }

    Buffer out;
    buffer_init(&out);
    char zero[MACHO_PAGE_SIZE];
    memset(zero, 0, sizeof zero);

#define APPEND_BYTES(p, n) buffer_append(&out, (const char *)(p), (n))
/* Loop form: padding requests may exceed one page. */
#define APPEND_ZERO(n) do {                                                    \
        size_t _left = (n);                                                   \
        while (_left) {                                                       \
            size_t _chunk = _left < sizeof zero ? _left : sizeof zero;        \
            buffer_append(&out, zero, _chunk);                                \
            _left -= _chunk;                                                  \
        }                                                                     \
    } while (0)

    /* ── mach_header_64 ── */
    mach_header_64 mh = {
        MH_MAGIC_64,
        CPU_TYPE_ARM64,
        CPU_SUBTYPE_ARM64_ALL,
        MH_EXECUTE,
        ncmds,
        sizeofcmds,
        MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE,
        0,
    };
    APPEND_BYTES(&mh, sizeof mh);

    /* ── LC_SEGMENT_64 __PAGEZERO (guard the low 4 GiB; no file bytes) ── */
    segment_command_64 pz = {0};
    pz.cmd = LC_SEGMENT_64;
    pz.cmdsize = seg_plain_cmd;
    memcpy(pz.segname, "__PAGEZERO", 10);
    pz.vmaddr = 0;
    pz.vmsize = MACHO_BASE_VA;
    pz.maxprot = 0;
    pz.initprot = 0;
    APPEND_BYTES(&pz, sizeof pz);

    /* ── LC_SEGMENT_64 __TEXT (r-x): __text + __const ── */
    segment_command_64 seg = {0};
    seg.cmd = LC_SEGMENT_64;
    seg.cmdsize = seg_text_cmd;
    memcpy(seg.segname, "__TEXT", 6);
    seg.vmaddr = MACHO_BASE_VA;
    seg.vmsize = text_pages;
    seg.fileoff = 0;
    seg.filesize = text_pages;
    seg.maxprot = VM_PROT_READ | VM_PROT_EXEC;
    seg.initprot = VM_PROT_READ | VM_PROT_EXEC;
    seg.nsects = (uint32_t)n_text_secs;
    APPEND_BYTES(&seg, sizeof seg);

    section_64 sec = {0};
    memcpy(sec.sectname, "__text", 6);
    memcpy(sec.segname, "__TEXT", 6);
    sec.addr = MACHO_BASE_VA + text_off;
    sec.size = text->len;
    sec.offset = text_off;
    sec.align = 3;   /* 2^3 = 8-byte instruction alignment */
    sec.flags = S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS;
    APPEND_BYTES(&sec, sizeof sec);

    if (has_const_sec) {
        section_64 csec = {0};
        memcpy(csec.sectname, "__const", 7);
        memcpy(csec.segname, "__TEXT", 6);
        csec.addr = MACHO_BASE_VA + ro_off;
        csec.size = rodata->len;
        csec.offset = ro_off;
        csec.align = 4;   /* ro_off is aligned to at least 2^4 = 16 bytes */
        csec.flags = 0;                  /* S_REGULAR read-only data */
        APPEND_BYTES(&csec, sizeof csec);
    }

    /* ── LC_SEGMENT_64 __DATA (rw-): __data + __bss (only if nonempty) ── */
    if (data_present) {
        segment_command_64 ds = {0};
        ds.cmd = LC_SEGMENT_64;
        ds.cmdsize = seg_data_cmd;
        memcpy(ds.segname, "__DATA", 6);
        ds.vmaddr = MACHO_BASE_VA + data_page;
        ds.vmsize = data_vm;
        ds.fileoff = data_page;
        ds.filesize = data_file;
        ds.maxprot = VM_PROT_READ | VM_PROT_WRITE;
        ds.initprot = VM_PROT_READ | VM_PROT_WRITE;
        ds.nsects = (uint32_t)n_data_secs;
        APPEND_BYTES(&ds, sizeof ds);

        if (has_data_sec) {
            section_64 dsec = {0};
            memcpy(dsec.sectname, "__data", 6);
            memcpy(dsec.segname, "__DATA", 6);
            dsec.addr = MACHO_BASE_VA + data_page;
            dsec.size = data->len;
            dsec.offset = (uint32_t)data_page;
            dsec.align = 3;
            APPEND_BYTES(&dsec, sizeof dsec);
        }

        if (has_bss_sec) {
            section_64 bsec = {0};
            memcpy(bsec.sectname, "__bss", 5);
            memcpy(bsec.segname, "__DATA", 6);
            /* bss starts after the page-rounded file-backed data image. */
            bsec.addr = MACHO_BASE_VA + data_page + data_file;
            bsec.size = bss_bytes;
            bsec.offset = 0;            /* zerofill: no file bytes */
            bsec.align = 3;
            bsec.flags = 0x1;           /* S_ZEROFILL */
            APPEND_BYTES(&bsec, sizeof bsec);
        }
    }

    /* ── LC_SEGMENT_64 __LINKEDIT (codesign fills it) ── */
    segment_command_64 le = {0};
    le.cmd = LC_SEGMENT_64;
    le.cmdsize = seg_plain_cmd;
    memcpy(le.segname, "__LINKEDIT", 10);
    le.vmaddr = MACHO_BASE_VA + link_off;
    le.vmsize = MACHO_PAGE_SIZE;
    le.fileoff = link_off;
    le.filesize = 0;
    le.maxprot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC;
    le.initprot = VM_PROT_READ;
    APPEND_BYTES(&le, sizeof le);

    /* ── LC_BUILD_VERSION (macOS 11.0, no tool entries) ── */
    build_version_command bv = {
        LC_BUILD_VERSION, build_cmd, PLATFORM_MACOS,
        (11u << 16), 0, 0,
    };
    APPEND_BYTES(&bv, sizeof bv);

    /* ── LC_UUID (dyld requires one; deterministic stage-1 value) ── */
    struct { uint32_t cmd; uint32_t cmdsize; unsigned char uuid[16]; } uc;
    uc.cmd = LC_UUID;
    uc.cmdsize = uuid_cmd;
    for (int i = 0; i < 16; i++)
        uc.uuid[i] = (unsigned char)(0xFA + i);
    APPEND_BYTES(&uc, sizeof uc);

    /* ── LC_MAIN ── */
    entry_point_command ep = {
        LC_MAIN, main_cmd, entry_off, 0,
    };
    APPEND_BYTES(&ep, sizeof ep);

    /* ── LC_LOAD_DYLINKER ── */
    dylinker_command dl = { LC_LOAD_DYLINKER, dylinker_cmdsz, dylinker_name_off };
    APPEND_BYTES(&dl, sizeof dl);
    APPEND_BYTES(DYLINKER_PATH, strlen(DYLINKER_PATH) + 1);
    APPEND_ZERO(dylinker_cmdsz - dylinker_name_off - strlen(DYLINKER_PATH) - 1);

    /* ── LC_LOAD_DYLIB (libSystem; zero imports are legal) ── */
    dylib_command dc;
    memset(&dc, 0, sizeof dc);
    dc.cmd = LC_LOAD_DYLIB;
    dc.cmdsize = dylib_cmdsz;
    dc.dylib.name_offset = dylib_name_off;
    APPEND_BYTES(&dc, offsetof(dylib_command, name));
    APPEND_BYTES(LIBSYSTEM_PATH, strlen(LIBSYSTEM_PATH) + 1);
    APPEND_ZERO(dylib_cmdsz - dylib_name_off - strlen(LIBSYSTEM_PATH) - 1);

    /* ── __TEXT segment file bytes: pad → text → __const → page ── */
    while (out.len < text_off) APPEND_ZERO((size_t)(text_off - out.len));
    buffer_append(&out, text->data, text->len);
    if (ro_off) {
        while (out.len < ro_off) APPEND_ZERO((size_t)(ro_off - out.len));
        buffer_append(&out, rodata->data, rodata->len);
    }
    while ((uint64_t)out.len < text_pages)
        APPEND_ZERO((size_t)(text_pages - out.len));

    /* ── __DATA segment file bytes: __data, padded through the link edit
     * page so codesign can append its signature at link_off.  __bss has
     * no file bytes; the kernel zero-fills the vm tail. ── */
    if (data_present) {
        buffer_append(&out, data->data, data->len);
        while ((uint64_t)out.len < link_off)
            APPEND_ZERO((size_t)(link_off - out.len));
    }

#undef APPEND_BYTES
#undef APPEND_ZERO

    size_t total_size = (size_t)link_off;

    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "fakecc: cannot create '%s'\n", path);
        buffer_free(&out);
        return -1;
    }
    size_t written = fwrite(out.data, 1, out.len, f);
    fclose(f);
    chmod(path, 0755);
    buffer_free(&out);
    if (written != total_size) {
        fprintf(stderr, "fakecc: short write on '%s' (%zu != %zu)\n",
                path, written, total_size);
        return -1;
    }
    return 0;
}

int macho_codesign(const char *path) {
#ifndef FAKECC_SELFHOST
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fakecc: fork failed for codesign\n");
        return -1;
    }
    if (pid == 0) {
        execl("/usr/bin/codesign", "codesign", "-s", "-", "-f", path,
              (char *)NULL);
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "fakecc: ad-hoc codesign failed for '%s'\n", path);
        return -1;
    }
    return 0;
#else
    /* Selfhost images never emit a Mach-O that needs a host codesign. */
    (void)path;
    return -1;
#endif
}
