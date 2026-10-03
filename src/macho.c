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
#define MH_OBJECT     1u
#define MH_EXECUTE    2u
#define MH_SUBSECTIONS_VIA_SYMBOLS 0x00002000u
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
#define S_ZEROFILL                 0x01u
#define S_ATTR_PURE_INSTRUCTIONS   0x80000000u
#define S_ATTR_SOME_INSTRUCTIONS   0x00000400u
#define N_EXT  0x01u
#define N_SECT 0x0eu

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

/* Packed via per-struct attribute instead of #pragma pack: the bootstrap
 * dialect has no preprocessor but its parser honors
 * __attribute__((packed)) (translate.py preserves it). */

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
} mach_header_64;

typedef struct __attribute__((packed)) {
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

typedef struct __attribute__((packed)) {
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

typedef struct __attribute__((packed)) {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t symoff;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;
} symtab_command;

typedef struct __attribute__((packed)) {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t ilocalsym;
    uint32_t nlocalsym;
    uint32_t iextdefsym;
    uint32_t nextdefsym;
    uint32_t iundefsym;
    uint32_t nundefsym;
    uint32_t tocoff;
    uint32_t ntoc;
    uint32_t modtaboff;
    uint32_t nmodtab;
    uint32_t extrefsymoff;
    uint32_t nextrefsyms;
    uint32_t indirectsymoff;
    uint32_t nindirectsyms;
    uint32_t extreloff;
    uint32_t nextrel;
    uint32_t locreloff;
    uint32_t nlocrel;
} dysymtab_command;

typedef struct __attribute__((packed)) {
    uint32_t n_strx;
    uint8_t  n_type;
    uint8_t  n_sect;
    uint16_t n_desc;
    uint64_t n_value;
} nlist_64;

typedef struct __attribute__((packed)) {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t platform;
    uint32_t minos;       /* packed x.y.z: (x<<16)|(y<<8)|z */
    uint32_t sdk;
    uint32_t ntools;
} build_version_command;

typedef struct __attribute__((packed)) {
    uint32_t cmd;
    uint32_t cmdsize;
    uint64_t entryoff;
    uint64_t stacksize;
} entry_point_command;

typedef struct __attribute__((packed)) {
    uint32_t cmd;
    uint32_t cmdsize;
    unsigned char uuid[16];
} uuid_command;

typedef struct __attribute__((packed)) {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t nameoff;
} dylinker_command;

typedef struct __attribute__((packed)) {
    uint32_t name_offset;
    uint32_t timestamp;
    uint32_t current_version;
    uint32_t compatibility_version;
} dylib;

typedef struct __attribute__((packed)) {
    uint32_t cmd;
    uint32_t cmdsize;
    dylib    dylib;
    char     name[];      /* offset 24, NUL terminated */
} dylib_command;

typedef struct __attribute__((packed)) {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t dataoff;
    uint32_t datasize;
} linkedit_data_command;


/* pad to a multiple of 8 for load-command sizes */
static uint32_t align8(uint32_t n) { return (n + 7u) & ~7u; }

static void put_u16(uint8_t *p, uint16_t v) { memcpy(p, &v, 2); }
static void put_u32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }
static void put_u64(uint8_t *p, uint64_t v) { memcpy(p, &v, 8); }

static int rebase_cmp(const void *a, const void *b) {
    const EmitRebase *x = a;
    const EmitRebase *y = b;
    if (x->slot < y->slot) return -1;
    if (x->slot > y->slot) return 1;
    return 0;
}

/* DYLD_CHAINED_PTR_64_OFFSET (pointer format 6).  `target` is the vm
 * offset from the mach header; dyld adds the actual load address.
 * Bit layout is dyld_chained_ptr_64_rebase (mach-o/fixup-chains.h):
 *   target 36 | high8 8 | reserved 7 | next 12 | bind 1
 * `next` counts 4-byte steps to the following pointer on the same page.
 * Checked against an ld64 binary on this OS: format 6, page_start is
 * the byte offset of the first pointer in the 16 KiB page. */
static uint64_t encode_chain_ptr(uint64_t target, uint32_t next) {
    return (target & 0xFFFFFFFFFull) | ((uint64_t)(next & 0xFFFu) << 51);
}

/* Encode em->rebases into __data and build the LC_DYLD_CHAINED_FIXUPS
 * payload plus an empty exports trie.  Only __DATA is chained; const
 * data that contains pointers is placed there by codegen64. */
static int macho_build_fixups(const EmitModule *em, uint64_t data_page,
                              uint64_t data_file, char **data_img_out,
                              Buffer *fixblob, Buffer *trie) {
    if (!em->data.len || data_file == 0) {
        fprintf(stderr, "fakecc: rebase with empty __data\n");
        return -1;
    }
    uint32_t page_count = (uint32_t)(data_file / MACHO_PAGE_SIZE);
    if (page_count == 0 || page_count > 0xFFFFu) {
        fprintf(stderr, "fakecc: __data page count out of range\n");
        return -1;
    }

    EmitRebase *r = xmalloc(em->num_rebases * sizeof *r);
    memcpy(r, em->rebases, em->num_rebases * sizeof *r);
    qsort(r, em->num_rebases, sizeof *r, rebase_cmp);

    char *img = xmalloc(em->data.len);
    memcpy(img, em->data.data, em->data.len);

    for (size_t i = 0; i < em->num_rebases; i++) {
        if (r[i].slot < data_page ||
            r[i].slot + 8 > data_page + em->data.len) {
            fprintf(stderr, "fakecc: rebase slot %#llx outside __data\n",
                    (unsigned long long)r[i].slot);
            free(r);
            free(img);
            return -1;
        }
        uint64_t off = r[i].slot - data_page;
        if ((off & 7) || (off % MACHO_PAGE_SIZE) + 8 > MACHO_PAGE_SIZE) {
            fprintf(stderr, "fakecc: rebase slot %#llx is not an in-page "
                    "8-byte pointer\n", (unsigned long long)r[i].slot);
            free(r);
            free(img);
            return -1;
        }
        if (r[i].target > 0xFFFFFFFFFull) {
            fprintf(stderr, "fakecc: rebase target exceeds 36 bits\n");
            free(r);
            free(img);
            return -1;
        }
        uint32_t next = 0;
        if (i + 1 < em->num_rebases) {
            uint64_t off2 = r[i + 1].slot - data_page;
            if ((off / MACHO_PAGE_SIZE) == (off2 / MACHO_PAGE_SIZE)) {
                uint64_t dist = r[i + 1].slot - r[i].slot;
                if (dist == 0 || (dist & 3) || dist / 4 > 0xFFF) {
                    fprintf(stderr, "fakecc: rebase chain stride out of range\n");
                    free(r);
                    free(img);
                    return -1;
                }
                next = (uint32_t)(dist / 4);
            }
        }
        uint64_t enc = encode_chain_ptr(r[i].target, next);
        memcpy(img + (size_t)off, &enc, 8);
    }

    /* header (28) padded to 32, then starts-in-image for the four
     * segments.  Segment info is 8-aligned, so it begins at starts+24
     * (ld64 does the same).  imports_count is 0; the trailing 8 zero
     * bytes match the ld64 blob. */
    uint32_t starts_off = 32;
    uint32_t seg_at = 56;
    uint32_t raw = 22u + 2u * page_count;
    uint32_t seg_size = (raw + 7u) & ~7u;
    uint32_t imports_off = seg_at + seg_size;
    uint32_t blob_size = imports_off + 8;
    uint8_t *blob = xmalloc(blob_size);
    memset(blob, 0, blob_size);
    put_u32(blob + 4, starts_off);
    put_u32(blob + 8, imports_off);
    put_u32(blob + 12, imports_off);
    put_u32(blob + 20, 1);                         /* DYLD_CHAINED_IMPORT */
    put_u32(blob + starts_off, 4);                 /* seg_count */
    put_u32(blob + starts_off + 12, 24);           /* seg_info_offset[2] */
    put_u32(blob + seg_at, seg_size);
    put_u16(blob + seg_at + 4, (uint16_t)MACHO_PAGE_SIZE);
    put_u16(blob + seg_at + 6, 6);                 /* PTR_64_OFFSET */
    put_u64(blob + seg_at + 8, data_page);
    put_u16(blob + seg_at + 20, (uint16_t)page_count);
    for (uint32_t p = 0; p < page_count; p++)
        put_u16(blob + seg_at + 22 + 2u * p, 0xFFFF);
    for (size_t i = 0; i < em->num_rebases; i++) {
        uint64_t off = r[i].slot - data_page;
        uint32_t pg = (uint32_t)(off / MACHO_PAGE_SIZE);
        uint16_t cur = 0;
        memcpy(&cur, blob + seg_at + 22 + 2u * pg, 2);
        if (cur == 0xFFFF)
            put_u16(blob + seg_at + 22 + 2u * pg,
                    (uint16_t)(off % MACHO_PAGE_SIZE));
    }
    buffer_append(fixblob, (const char *)blob, blob_size);
    free(blob);
    free(r);

    char empty_trie[2] = {0, 0};
    buffer_append(trie, empty_trie, 2);
    *data_img_out = img;
    return 0;
}

/* mach_header_64 (32) + load commands, then __text at MACHO_TEXT_OFF.
 * 1024 leaves ample room for codesign's injected LC_CODE_SIGNATURE. */
#define MACHO_TEXT_OFF 1024u

uint32_t macho_text_offset(void) { return MACHO_TEXT_OFF; }

/* Must stay in lock-step with the layout block in macho_write_exec. */
void macho_section_offsets(const EmitModule *em, size_t text_len,
                           uint64_t *ro_out, uint64_t *data_out,
                           uint64_t *bss_out) {
    uint64_t ro_off = 0;
    if (em->rodata.len) {
        ro_off = MACHO_TEXT_OFF + (uint64_t)text_len;
        size_t al = em->rodata_align > 16 ? em->rodata_align : 16;
        while (ro_off % al) ro_off++;
    }
    uint64_t text_used = ro_off ? ro_off + em->rodata.len
                                : (uint64_t)MACHO_TEXT_OFF + text_len;
    uint64_t text_pages = (text_used + MACHO_PAGE_SIZE - 1)
                          & ~(uint64_t)(MACHO_PAGE_SIZE - 1);
    uint64_t data_file = em->data.len
        ? ((uint64_t)em->data.len + MACHO_PAGE_SIZE - 1)
          & ~(uint64_t)(MACHO_PAGE_SIZE - 1)
        : 0;
    if (ro_out) *ro_out = ro_off;
    if (data_out) *data_out = em->data.len ? text_pages : 0;
    if (bss_out) *bss_out = em->bss_size ? text_pages + data_file : 0;
}

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

    char *data_img = NULL;
    Buffer fixblob, trieblob;
    buffer_init(&fixblob);
    buffer_init(&trieblob);
    int has_fix = em->num_rebases > 0;
    if (has_fix &&
        macho_build_fixups(em, data_page, data_file, &data_img,
                           &fixblob, &trieblob) != 0) {
        buffer_free(&fixblob);
        buffer_free(&trieblob);
        return -1;
    }
    uint64_t linkedit_payload = has_fix
        ? (uint64_t)fixblob.len + (uint64_t)trieblob.len : 0;

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
     * commands (build version, uuid, main, dylinker, dylib).
     * Pointer initializers add LC_DYLD_CHAINED_FIXUPS and an empty
     * LC_DYLD_EXPORTS_TRIE (dyld wants the pair). */
    const uint32_t ncmds = (data_present ? 4u : 3u) + 5u + (has_fix ? 2u : 0u);
    const uint32_t uuid_cmd = 24;
    const uint32_t sizeofcmds =
        seg_plain_cmd + seg_text_cmd + seg_data_cmd + seg_plain_cmd
        + build_cmd + uuid_cmd + main_cmd
        + dylinker_cmdsz + dylib_cmdsz
        + (has_fix ? 32u : 0u);

    const uint32_t cmds_end = (uint32_t)sizeof(mach_header_64) + sizeofcmds;
    if (cmds_end > MACHO_TEXT_OFF) {
        fprintf(stderr, "fakecc: Mach-O commands (%u) overflow header pad\n",
                cmds_end);
        free(data_img);
        buffer_free(&fixblob);
        buffer_free(&trieblob);
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
    /* Leave room for the ad-hoc signature codesign appends after the
     * chained-fixup payload. */
    if (linkedit_payload + 8192 > le.vmsize)
        le.vmsize = (linkedit_payload + 8192 + MACHO_PAGE_SIZE - 1)
                    & ~(uint64_t)(MACHO_PAGE_SIZE - 1);
    le.fileoff = link_off;
    le.filesize = linkedit_payload;
    le.maxprot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC;
    le.initprot = VM_PROT_READ;
    APPEND_BYTES(&le, sizeof le);

    if (has_fix) {
        linkedit_data_command fx = {
            LC_DYLD_CHAINED_FIXUPS, 16,
            (uint32_t)link_off, (uint32_t)fixblob.len,
        };
        linkedit_data_command tr = {
            LC_DYLD_EXPORTS_TRIE, 16,
            (uint32_t)(link_off + fixblob.len), (uint32_t)trieblob.len,
        };
        APPEND_BYTES(&fx, sizeof fx);
        APPEND_BYTES(&tr, sizeof tr);
    }

    /* ── LC_BUILD_VERSION (macOS 11.0, no tool entries) ── */
    build_version_command bv = {
        LC_BUILD_VERSION, build_cmd, PLATFORM_MACOS,
        (11u << 16), 0, 0,
    };
    APPEND_BYTES(&bv, sizeof bv);

    /* ── LC_UUID (dyld requires one; deterministic stage-1 value) ── */
    uuid_command uc;
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
    APPEND_BYTES(&dc, 24); /* offsetof(dylib_command, name) — fakecc dialect has no offsetof */
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
        buffer_append(&out, data_img ? data_img : data->data, data->len);
        while ((uint64_t)out.len < link_off)
            APPEND_ZERO((size_t)(link_off - out.len));
    }
    if (has_fix) {
        buffer_append(&out, fixblob.data, fixblob.len);
        buffer_append(&out, trieblob.data, trieblob.len);
    }
    free(data_img);
    buffer_free(&fixblob);
    buffer_free(&trieblob);
    data_img = NULL;

#undef APPEND_BYTES
#undef APPEND_ZERO

    size_t total_size = out.len;

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

static uint64_t align_up_u64(uint64_t v, uint64_t a) {
    if (a <= 1) return v;
    return (v + a - 1) & ~(a - 1);
}

/* MH_OBJECT with __text/__const/__data/__bss and a symbol table.
 * Relocations are left empty: callers that need a global address still
 * die in codegen. */
int macho_write_object(const EmitModule *em, const char *path) {
    int has_ro = em->rodata.len > 0;
    int has_data = em->data.len > 0;
    int has_bss = em->bss_size > 0;
    uint32_t nsects = 1u + (uint32_t)has_ro + (uint32_t)has_data
                    + (uint32_t)has_bss;
    uint32_t seg_cmdsize = (uint32_t)sizeof(segment_command_64)
                         + nsects * (uint32_t)sizeof(section_64);
    uint32_t sizeofcmds = seg_cmdsize
                        + (uint32_t)sizeof(build_version_command)
                        + (uint32_t)sizeof(symtab_command)
                        + (uint32_t)sizeof(dysymtab_command);
    uint64_t content_off = align_up_u64(32u + sizeofcmds, 8);

    uint8_t sect_of[9];
    uint64_t sect_addr[9];
    memset(sect_of, 0, sizeof sect_of);
    memset(sect_addr, 0, sizeof sect_addr);
    uint32_t nsec = 1;
    sect_of[SECT_TEXT] = (uint8_t)nsec++;
    if (has_ro) sect_of[SECT_RODATA] = (uint8_t)nsec++;
    if (has_data) sect_of[SECT_DATA] = (uint8_t)nsec++;
    if (has_bss) sect_of[SECT_BSS] = (uint8_t)nsec++;

    uint64_t vm = 0;
    uint64_t file = content_off;
    sect_addr[SECT_TEXT] = 0;
    uint64_t text_off = file;
    vm += em->text.len;
    file += em->text.len;

    uint64_t ro_off = 0, data_off = 0;
    if (has_ro) {
        uint64_t al = em->rodata_align ? em->rodata_align : 8;
        vm = align_up_u64(vm, al);
        file = align_up_u64(file, al);
        sect_addr[SECT_RODATA] = vm;
        ro_off = file;
        vm += em->rodata.len;
        file += em->rodata.len;
    }
    if (has_data) {
        uint64_t al = em->data_align ? em->data_align : 8;
        vm = align_up_u64(vm, al);
        file = align_up_u64(file, al);
        sect_addr[SECT_DATA] = vm;
        data_off = file;
        vm += em->data.len;
        file += em->data.len;
    }
    if (has_bss) {
        uint64_t al = em->bss_align ? em->bss_align : 8;
        vm = align_up_u64(vm, al);
        sect_addr[SECT_BSS] = vm;
        vm += em->bss_size;
    }
    uint64_t content_end = file;
    uint64_t vm_end = vm;

    size_t nlocal = 0, nglobal = 0;
    for (size_t i = 0; i < em->num_syms; i++) {
        const EmitSymbol *s = &em->syms[i];
        if (!s->name || s->shndx >= 9 || !sect_of[s->shndx]) continue;
        if (s->binding == 0) nlocal++;
        else nglobal++;
    }
    size_t nsyms = nlocal + nglobal;
    size_t strsize = 1;
    for (size_t i = 0; i < em->num_syms; i++) {
        const EmitSymbol *s = &em->syms[i];
        if (!s->name || s->shndx >= 9 || !sect_of[s->shndx]) continue;
        strsize += 1 + strlen(s->name) + 1; /* leading '_' */
    }
    uint64_t symoff = align_up_u64(content_end, 8);
    uint64_t stroff = symoff + nsyms * sizeof(nlist_64);

    Buffer out;
    buffer_init(&out);
    mach_header_64 hdr;
    memset(&hdr, 0, sizeof hdr);
    hdr.magic = MH_MAGIC_64;
    hdr.cputype = CPU_TYPE_ARM64;
    hdr.cpusubtype = CPU_SUBTYPE_ARM64_ALL;
    hdr.filetype = MH_OBJECT;
    hdr.ncmds = 4;
    hdr.sizeofcmds = sizeofcmds;
    hdr.flags = MH_SUBSECTIONS_VIA_SYMBOLS;
    buffer_append(&out, (const char *)&hdr, sizeof hdr);

    segment_command_64 seg;
    memset(&seg, 0, sizeof seg);
    seg.cmd = LC_SEGMENT_64;
    seg.cmdsize = seg_cmdsize;
    seg.vmsize = vm_end;
    seg.fileoff = content_off;
    seg.filesize = content_end - content_off;
    seg.maxprot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC;
    seg.initprot = seg.maxprot;
    seg.nsects = nsects;
    buffer_append(&out, (const char *)&seg, sizeof seg);

    section_64 sec;
    memset(&sec, 0, sizeof sec);
    memcpy(sec.sectname, "__text", 6);
    memcpy(sec.segname, "__TEXT", 6);
    sec.addr = 0;
    sec.size = em->text.len;
    sec.offset = (uint32_t)text_off;
    sec.align = 2;
    sec.flags = S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS;
    buffer_append(&out, (const char *)&sec, sizeof sec);
    if (has_ro) {
        memset(&sec, 0, sizeof sec);
        memcpy(sec.sectname, "__const", 7);
        memcpy(sec.segname, "__TEXT", 6);
        sec.addr = sect_addr[SECT_RODATA];
        sec.size = em->rodata.len;
        sec.offset = (uint32_t)ro_off;
        sec.align = 3;
        buffer_append(&out, (const char *)&sec, sizeof sec);
    }
    if (has_data) {
        memset(&sec, 0, sizeof sec);
        memcpy(sec.sectname, "__data", 6);
        memcpy(sec.segname, "__DATA", 6);
        sec.addr = sect_addr[SECT_DATA];
        sec.size = em->data.len;
        sec.offset = (uint32_t)data_off;
        sec.align = 3;
        buffer_append(&out, (const char *)&sec, sizeof sec);
    }
    if (has_bss) {
        memset(&sec, 0, sizeof sec);
        memcpy(sec.sectname, "__bss", 5);
        memcpy(sec.segname, "__DATA", 6);
        sec.addr = sect_addr[SECT_BSS];
        sec.size = em->bss_size;
        sec.align = 3;
        sec.flags = S_ZEROFILL;
        buffer_append(&out, (const char *)&sec, sizeof sec);
    }

    build_version_command bv;
    memset(&bv, 0, sizeof bv);
    bv.cmd = LC_BUILD_VERSION;
    bv.cmdsize = (uint32_t)sizeof bv;
    bv.platform = PLATFORM_MACOS;
    bv.minos = 0x000E0000u; /* 14.0.0 */
    bv.sdk = 0x000E0000u;
    buffer_append(&out, (const char *)&bv, sizeof bv);

    symtab_command sy;
    memset(&sy, 0, sizeof sy);
    sy.cmd = LC_SYMTAB;
    sy.cmdsize = (uint32_t)sizeof sy;
    sy.symoff = (uint32_t)symoff;
    sy.nsyms = (uint32_t)nsyms;
    sy.stroff = (uint32_t)stroff;
    sy.strsize = (uint32_t)strsize;
    buffer_append(&out, (const char *)&sy, sizeof sy);

    dysymtab_command dy;
    memset(&dy, 0, sizeof dy);
    dy.cmd = LC_DYSYMTAB;
    dy.cmdsize = (uint32_t)sizeof dy;
    dy.ilocalsym = 0;
    dy.nlocalsym = (uint32_t)nlocal;
    dy.iextdefsym = (uint32_t)nlocal;
    dy.nextdefsym = (uint32_t)nglobal;
    dy.iundefsym = (uint32_t)nsyms;
    buffer_append(&out, (const char *)&dy, sizeof dy);

    while (out.len < content_off) {
        char z = 0;
        buffer_append(&out, &z, 1);
    }
    if (em->text.len)
        buffer_append(&out, em->text.data, em->text.len);
    if (has_ro) {
        while (out.len < ro_off) { char z = 0; buffer_append(&out, &z, 1); }
        buffer_append(&out, em->rodata.data, em->rodata.len);
    }
    if (has_data) {
        while (out.len < data_off) { char z = 0; buffer_append(&out, &z, 1); }
        buffer_append(&out, em->data.data, em->data.len);
    }
    while (out.len < symoff) { char z = 0; buffer_append(&out, &z, 1); }

    /* Locals first, then globals, matching LC_DYSYMTAB.  String offsets
     * are assigned in the same order. */
    size_t str_at = 1;
    for (int pass = 0; pass < 2; pass++) {
        for (size_t i = 0; i < em->num_syms; i++) {
            const EmitSymbol *s = &em->syms[i];
            if (!s->name || s->shndx >= 9 || !sect_of[s->shndx]) continue;
            int local = s->binding == 0;
            if (pass == 0 && !local) continue;
            if (pass == 1 && local) continue;
            nlist_64 nl;
            memset(&nl, 0, sizeof nl);
            nl.n_strx = (uint32_t)str_at;
            nl.n_type = (uint8_t)(local ? N_SECT : (N_SECT | N_EXT));
            nl.n_sect = sect_of[s->shndx];
            nl.n_value = sect_addr[s->shndx] + (uint64_t)s->value;
            buffer_append(&out, (const char *)&nl, sizeof nl);
            str_at += 1 + strlen(s->name) + 1;
        }
    }
    char nul = 0;
    buffer_append(&out, &nul, 1);
    for (int pass = 0; pass < 2; pass++) {
        for (size_t i = 0; i < em->num_syms; i++) {
            const EmitSymbol *s = &em->syms[i];
            if (!s->name || s->shndx >= 9 || !sect_of[s->shndx]) continue;
            int local = s->binding == 0;
            if (pass == 0 && !local) continue;
            if (pass == 1 && local) continue;
            buffer_append(&out, "_", 1);
            buffer_append(&out, s->name, strlen(s->name) + 1);
        }
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "fakecc: cannot create '%s'\n", path);
        buffer_free(&out);
        return -1;
    }
    size_t written = fwrite(out.data, 1, out.len, f);
    size_t total = out.len;
    fclose(f);
    chmod(path, 0644);
    buffer_free(&out);
    if (written != total) {
        fprintf(stderr, "fakecc: short write on '%s'\n", path);
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
