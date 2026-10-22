/* PE32+ linker for the x86_64-windows target.
 *
 * Codegen speaks the Microsoft x64 convention, so process entry and a
 * DLL's DllMain entry can call user functions directly.  The one
 * translation left in here is __fakecc_win_syscall: codegen still emits
 * the Linux syscall register assignment, and the trampoline reshuffles
 * it before calling kernel32.
 *
 * Imports, thunks and the IAT are built from the linker's own kernel32
 * list plus whatever import libraries the driver pulled in.  Absolute
 * addresses are baked against ImageBase and listed in a base relocation
 * table so ASLR can slide an EXE or a DLL.  RIP-relative references do
 * not need one.
 */

#include "fakecc/pe.h"
#include "fakecc/emit.h"
#include "fakecc/debug.h"
#include "fakecc/common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* True when `name` is a COMDAT defining symbol with a selection that permits
 * discarding duplicates (ANY / LARGEST / SAME_SIZE / EXACT_MATCH). */
static int comdat_allows_dup(EmitModule **mods, size_t nmods, const char *name) {
    if (!name) return 0;
    for (size_t mi = 0; mi < nmods; mi++) {
        EmitModule *m = mods[mi];
        for (size_t c = 0; c < m->num_comdats; c++) {
            if (!m->comdats[c].symbol) continue;
            if (strcmp(m->comdats[c].symbol, name) != 0) continue;
            uint8_t s = m->comdats[c].selection;
            if (s == IMAGE_COMDAT_SELECT_ANY
                || s == IMAGE_COMDAT_SELECT_LARGEST
                || s == IMAGE_COMDAT_SELECT_SAME_SIZE
                || s == IMAGE_COMDAT_SELECT_EXACT_MATCH)
                return 1;
        }
    }
    return 0;
}

#define IMAGE_BASE  0x140000000ULL
#define SECT_ALIGN  0x1000u
#define FILE_ALIGN  0x200u

#define SCN_CODE    0x00000020u
#define SCN_IDATA   0x00000040u
#define SCN_UDATA   0x00000080u
#define SCN_DISCARD 0x02000000u
#define SCN_EXEC    0x20000000u
#define SCN_READ    0x40000000u
#define SCN_WRITE   0x80000000u

static uint32_t align_chars(size_t a) {
    unsigned e = 0;
    size_t v = a ? a : 1;
    while ((size_t)1 << e < v && e < 12) e++;
    if (e < 1) e = 1;
    return (uint32_t)(e + 1) << 20; /* IMAGE_SCN_ALIGN_* */
}

enum {
    IMP_GetStdHandle = 0,
    IMP_GetCommandLineA,
    IMP_GetEnvironmentStringsA,
    IMP_WriteFile,
    IMP_ReadFile,
    IMP_CreateFileA,
    IMP_CloseHandle,
    IMP_SetFilePointerEx,
    IMP_VirtualAlloc,
    IMP_VirtualFree,
    IMP_ExitProcess,
    IMP_DeleteFileA,
    IMP_MoveFileA,
    IMP_CreateDirectoryA,
    IMP_GetCurrentProcessId,
    IMP_CreateThread,
    IMP_WaitForSingleObject,
    IMP_ExitThread,
    IMP_GetCurrentThreadId,
    IMP_FindFirstFileA,
    IMP_FindNextFileA,
    IMP_FindClose,
    IMP_COUNT
};

static const char *const IMP_NAMES[IMP_COUNT] = {
    "GetStdHandle",
    "GetCommandLineA",
    "GetEnvironmentStringsA",
    "WriteFile",
    "ReadFile",
    "CreateFileA",
    "CloseHandle",
    "SetFilePointerEx",
    "VirtualAlloc",
    "VirtualFree",
    "ExitProcess",
    "DeleteFileA",
    "MoveFileA",
    "CreateDirectoryA",
    "GetCurrentProcessId",
    "CreateThread",
    "WaitForSingleObject",
    "ExitThread",
    "GetCurrentThreadId",
    "FindFirstFileA",
    "FindNextFileA",
    "FindClose",
};

/* Linker-synthesized bytes at the end of .data.  The stub addresses them
 * RIP-relative; user code never names them. */
#define SYN_FD      0          /* HANDLE fd[64]        */
#define SYN_APPEND  512        /* uint8_t append[64]   */
#define SYN_CMDLINE 576        /* char cmdline[4096]   */
#define SYN_ARGV    4672       /* char *argv[65]       */
#define SYN_SIZE    5192

enum {
    FK_IAT = 1,   /* disp32 of `call qword [rip+d]` -> IAT slot `extra` */
    FK_SYNTH,     /* disp32 of `lea reg, [rip+d]`   -> synth + extra     */
    FK_CALL,      /* disp32 of `call rel32`         -> symbol `name`     */
    FK_STORE,     /* disp32 of `mov [rip+d], rax`   -> symbol `name`     */
    FK_RDATA      /* disp32 of `lea reg, [rip+d]`   -> rdata + extra     */
};

typedef struct {
    size_t off;
    int kind;
    int extra;
    const char *name;
} Fixup;

typedef struct {
    Buffer buf;
    Fixup *fix;
    size_t nfix, capfix;
} Asm;

static uint32_t align_up_u32(uint32_t v, uint32_t a) {
    return (v + a - 1u) & ~(a - 1u);
}

static void *zalloc(size_t n) {
    void *p = calloc(1, n ? n : 1);
    if (!p) {
        fprintf(stderr, "fakecc: OOM\n");
        exit(1);
    }
    return p;
}

static void bpad(Buffer *b, size_t n) {
    char z[256];
    memset(z, 0, sizeof z);
    while (n) {
        size_t c = n < sizeof z ? n : sizeof z;
        buffer_append(b, z, c);
        n -= c;
    }
}

static void bu8(Buffer *b, uint8_t v)  { buffer_append(b, (const char *)&v, 1); }
static void bu16(Buffer *b, uint16_t v) { buffer_append(b, (const char *)&v, 2); }
static void bu32(Buffer *b, uint32_t v) { buffer_append(b, (const char *)&v, 4); }
static void bu64(Buffer *b, uint64_t v) { buffer_append(b, (const char *)&v, 8); }

static void ab(Asm *a, uint8_t x) { bu8(&a->buf, x); }

static void au32(Asm *a, uint32_t v) { bu32(&a->buf, v); }

static void add_fix(Asm *a, int kind, int extra, const char *name) {
    if (a->nfix == a->capfix) {
        a->capfix = a->capfix ? a->capfix * 2 : 16;
        a->fix = xrealloc(a->fix, a->capfix * sizeof(Fixup));
    }
    a->fix[a->nfix].off = a->buf.len;
    a->fix[a->nfix].kind = kind;
    a->fix[a->nfix].extra = extra;
    a->fix[a->nfix].name = name;
    a->nfix++;
    au32(a, 0);
}

static size_t jrel(Asm *a, uint8_t op, int is_jcc) {
    ab(a, is_jcc ? 0x0F : op);
    if (is_jcc) ab(a, op);
    size_t p = a->buf.len;
    au32(a, 0);
    return p;
}

static void apatch(Asm *a, size_t disp_off, size_t target) {
    int32_t rel = (int32_t)((int64_t)target - (int64_t)(disp_off + 4));
    memcpy(a->buf.data + disp_off, &rel, 4);
}

static size_t j32(Asm *a)             { return jrel(a, 0xE9, 0); }
static size_t jcc(Asm *a, uint8_t cc) { return jrel(a, cc, 1); }

static void call_iat(Asm *a, int imp) {
    ab(a, 0xFF); ab(a, 0x15);
    add_fix(a, FK_IAT, imp, NULL);
}

/* lea r, [rip+d] for rax..rdi (reg 0..7). */
static void lea_rip(Asm *a, int reg, int kind, int extra) {
    ab(a, 0x48); ab(a, 0x8D);
    ab(a, (uint8_t)((reg << 3) | 0x05));
    add_fix(a, kind, extra, NULL);
}

static void call_sym(Asm *a, const char *name) {
    ab(a, 0xE8);
    add_fix(a, FK_CALL, 0, name);
}

static void store_rax_sym(Asm *a, const char *name) {
    ab(a, 0x48); ab(a, 0x89); ab(a, 0x05); /* mov [rip+d], rax */
    add_fix(a, FK_STORE, 0, name);
}

/* cmp rdi, 63; ja `fail` (fail patched by the caller).  Returns the ja disp. */
static size_t cmp_fd(Asm *a) {
    ab(a, 0x48); ab(a, 0x83); ab(a, 0xFF); ab(a, 63);
    return jcc(a, 0x87); /* ja */
}

/* mov r12, [rbx+rdi*8] */
static void load_handle(Asm *a) {
    ab(a, 0x4C); ab(a, 0x8B); ab(a, 0x24); ab(a, 0xFB);
}

/* test r12, r12; jz disp.  Returns the jz disp. */
static size_t test_r12(Asm *a) {
    ab(a, 0x4D); ab(a, 0x85); ab(a, 0xE4);
    return jcc(a, 0x84);
}

static void zero_slot38(Asm *a) {
    /* mov dword [rsp+0x38], 0 */
    ab(a, 0xC7); ab(a, 0x44); ab(a, 0x24); ab(a, 0x38);
    au32(a, 0);
}

static void zero_arg5(Asm *a) {
    /* mov qword [rsp+0x20], 0 */
    ab(a, 0x48); ab(a, 0xC7); ab(a, 0x44); ab(a, 0x24); ab(a, 0x20);
    au32(a, 0);
}

/* ------------------------------------------------------------------ */
/* __fakecc_win_syscall                                                */
/* ------------------------------------------------------------------ */

static void emit_trampoline(Asm *a, size_t *fail_slots, int *nfail) {
    ab(a, 0x53);                              /* push rbx */
    ab(a, 0x41); ab(a, 0x54);                 /* push r12 */
    ab(a, 0x41); ab(a, 0x55);                 /* push r13 */
    ab(a, 0x48); ab(a, 0x83); ab(a, 0xEC); ab(a, 0x40);

    struct { uint8_t num; size_t slot; } br[32];
    int nb = 0;
    static const uint8_t nums[] = {
        0, 1, 2, 3, 8, 9, 11, 39, 60, 82, 83, 87, 90, 186, 200, 201, 202, 231
    };
    for (size_t i = 0; i < sizeof nums; i++) {
        /* imm32, not imm8: 186 and 231 do not fit in a sign-extended byte. */
        ab(a, 0x81); ab(a, 0xF8); au32(a, nums[i]);
        br[nb].num = nums[i];
        br[nb].slot = jcc(a, 0x84);
        nb++;
    }
    size_t to_fail = j32(a);

    size_t handler[256];
    memset(handler, 0, sizeof handler);
    *nfail = 0;

#define FAIL() do { fail_slots[(*nfail)++] = j32(a); } while (0)
#define JA_FAIL() do { fail_slots[(*nfail)++] = cmp_fd(a); } while (0)

    /* read */
    handler[0] = a->buf.len;
    JA_FAIL();
    ab(a, 0x49); ab(a, 0x89); ab(a, 0xD5);             /* mov r13, rdx */
    lea_rip(a, 3, FK_SYNTH, SYN_FD);                  /* lea rbx, fd */
    load_handle(a);
    fail_slots[(*nfail)++] = test_r12(a);
    ab(a, 0x4C); ab(a, 0x89); ab(a, 0xE1);             /* mov rcx, r12 */
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF2);             /* mov rdx, rsi */
    ab(a, 0x4D); ab(a, 0x89); ab(a, 0xE8);             /* mov r8, r13 */
    ab(a, 0x4C); ab(a, 0x8D); ab(a, 0x4C); ab(a, 0x24); ab(a, 0x38);
    zero_arg5(a);
    zero_slot38(a);
    call_iat(a, IMP_ReadFile);
    ab(a, 0x85); ab(a, 0xC0);                          /* test eax, eax */
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x8B); ab(a, 0x44); ab(a, 0x24); ab(a, 0x38); /* mov eax, [rsp+0x38] */
    size_t rd_done = j32(a);

    /* write, honouring the append flag recorded by open */
    handler[1] = a->buf.len;
    JA_FAIL();
    ab(a, 0x49); ab(a, 0x89); ab(a, 0xD5);             /* mov r13, rdx (count) */
    lea_rip(a, 3, FK_SYNTH, SYN_FD);
    load_handle(a);
    fail_slots[(*nfail)++] = test_r12(a);
    lea_rip(a, 0, FK_SYNTH, SYN_APPEND);              /* lea rax, append */
    ab(a, 0x80); ab(a, 0x3C); ab(a, 0x38); ab(a, 0x00); /* cmp byte [rax+rdi], 0 */
    size_t no_app = jcc(a, 0x84);
    ab(a, 0x4C); ab(a, 0x89); ab(a, 0xE1);             /* mov rcx, r12 */
    ab(a, 0x31); ab(a, 0xD2);                          /* xor edx, edx */
    ab(a, 0x45); ab(a, 0x31); ab(a, 0xC0);             /* xor r8d, r8d */
    ab(a, 0x41); ab(a, 0xB9); au32(a, 2);              /* mov r9d, FILE_END */
    call_iat(a, IMP_SetFilePointerEx);
    apatch(a, no_app, a->buf.len);
    ab(a, 0x4C); ab(a, 0x89); ab(a, 0xE1);
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF2);
    ab(a, 0x4D); ab(a, 0x89); ab(a, 0xE8);
    ab(a, 0x4C); ab(a, 0x8D); ab(a, 0x4C); ab(a, 0x24); ab(a, 0x38);
    zero_arg5(a);
    zero_slot38(a);
    call_iat(a, IMP_WriteFile);
    ab(a, 0x85); ab(a, 0xC0);
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x8B); ab(a, 0x44); ab(a, 0x24); ab(a, 0x38);
    size_t wr_done = j32(a);

    /* open: rdi=path rsi=flags.  Returns a small fd, or -1. */
    handler[2] = a->buf.len;
    ab(a, 0x41); ab(a, 0xBC); au32(a, 0x80000000u);    /* mov r12d, GENERIC_READ */
    ab(a, 0xF7); ab(a, 0xC6); au32(a, 2);              /* test esi, O_RDWR */
    size_t not_rdwr = jcc(a, 0x84);
    ab(a, 0x41); ab(a, 0x81); ab(a, 0xCC); au32(a, 0x40000000u); /* or r12d, GENERIC_WRITE */
    size_t acc_done = j32(a);
    apatch(a, not_rdwr, a->buf.len);
    ab(a, 0xF7); ab(a, 0xC6); au32(a, 1);              /* test esi, O_WRONLY */
    size_t acc_keep = jcc(a, 0x84);
    ab(a, 0x41); ab(a, 0xBC); au32(a, 0x40000000u);    /* mov r12d, GENERIC_WRITE */
    apatch(a, acc_keep, a->buf.len);
    apatch(a, acc_done, a->buf.len);
    ab(a, 0x41); ab(a, 0xBD); au32(a, 3);              /* mov r13d, OPEN_EXISTING */
    ab(a, 0xF7); ab(a, 0xC6); au32(a, 64);             /* test esi, O_CREAT */
    size_t no_creat = jcc(a, 0x84);
    ab(a, 0x41); ab(a, 0xBD); au32(a, 4);              /* OPEN_ALWAYS */
    ab(a, 0xF7); ab(a, 0xC6); au32(a, 512);            /* test esi, O_TRUNC */
    size_t no_trunc = jcc(a, 0x84);
    ab(a, 0x41); ab(a, 0xBD); au32(a, 2);              /* CREATE_ALWAYS */
    apatch(a, no_trunc, a->buf.len);
    apatch(a, no_creat, a->buf.len);
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF9);             /* mov rcx, rdi */
    ab(a, 0x44); ab(a, 0x89); ab(a, 0xE2);             /* mov edx, r12d */
    ab(a, 0x41); ab(a, 0xB8); au32(a, 3);              /* mov r8d, FILE_SHARE_READ|WRITE */
    ab(a, 0x45); ab(a, 0x31); ab(a, 0xC9);             /* xor r9d, r9d */
    ab(a, 0x44); ab(a, 0x89); ab(a, 0x6C); ab(a, 0x24); ab(a, 0x20); /* mov [rsp+0x20], r13d */
    ab(a, 0xC7); ab(a, 0x44); ab(a, 0x24); ab(a, 0x28); au32(a, 0x80); /* FILE_ATTRIBUTE_NORMAL */
    ab(a, 0x48); ab(a, 0xC7); ab(a, 0x44); ab(a, 0x24); ab(a, 0x30); au32(a, 0);
    call_iat(a, IMP_CreateFileA);
    ab(a, 0x48); ab(a, 0x83); ab(a, 0xF8); ab(a, 0xFF); /* cmp rax, -1 */
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x49); ab(a, 0x89); ab(a, 0xC4);             /* mov r12, rax */
    ab(a, 0x31); ab(a, 0xC9);                          /* xor ecx, ecx */
    lea_rip(a, 3, FK_SYNTH, SYN_FD);
    size_t scan = a->buf.len;
    ab(a, 0x83); ab(a, 0xF9); ab(a, 64);               /* cmp ecx, 64 */
    size_t ja_full = jcc(a, 0x83);                     /* jae close+fail */
    ab(a, 0x48); ab(a, 0x83); ab(a, 0x3C); ab(a, 0xCB); ab(a, 0x00);
    size_t je_found = jcc(a, 0x84);
    ab(a, 0xFF); ab(a, 0xC1);                          /* inc ecx */
    size_t back = j32(a);
    apatch(a, back, scan);
    size_t full = a->buf.len;
    apatch(a, ja_full, full);
    ab(a, 0x4C); ab(a, 0x89); ab(a, 0xE1);             /* mov rcx, r12 */
    call_iat(a, IMP_CloseHandle);
    FAIL();
    size_t found = a->buf.len;
    apatch(a, je_found, found);
    ab(a, 0x4C); ab(a, 0x89); ab(a, 0x24); ab(a, 0xCB); /* mov [rbx+rcx*8], r12 */
    lea_rip(a, 3, FK_SYNTH, SYN_APPEND);
    ab(a, 0xC6); ab(a, 0x04); ab(a, 0x0B); ab(a, 0x00); /* mov byte [rbx+rcx], 0 */
    ab(a, 0xF7); ab(a, 0xC6); au32(a, 1024);           /* test esi, O_APPEND */
    size_t no_ap = jcc(a, 0x84);
    ab(a, 0xC6); ab(a, 0x04); ab(a, 0x0B); ab(a, 0x01);
    apatch(a, no_ap, a->buf.len);
    ab(a, 0x89); ab(a, 0xC8);                          /* mov eax, ecx */
    size_t open_done = j32(a);

    /* close */
    handler[3] = a->buf.len;
    JA_FAIL();
    lea_rip(a, 3, FK_SYNTH, SYN_FD);
    ab(a, 0x48); ab(a, 0x8B); ab(a, 0x0C); ab(a, 0xFB); /* mov rcx, [rbx+rdi*8] */
    ab(a, 0x48); ab(a, 0x85); ab(a, 0xC9);
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x48); ab(a, 0xC7); ab(a, 0x04); ab(a, 0xFB); au32(a, 0);
    lea_rip(a, 3, FK_SYNTH, SYN_APPEND);
    ab(a, 0xC6); ab(a, 0x04); ab(a, 0x3B); ab(a, 0x00); /* mov byte [rbx+rdi], 0 */
    call_iat(a, IMP_CloseHandle);
    ab(a, 0x85); ab(a, 0xC0);
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x31); ab(a, 0xC0);
    size_t close_done = j32(a);

    /* lseek: rdi=fd rsi=off rdx=whence */
    handler[8] = a->buf.len;
    ab(a, 0x49); ab(a, 0x89); ab(a, 0xD5);             /* mov r13, rdx */
    JA_FAIL();
    lea_rip(a, 3, FK_SYNTH, SYN_FD);
    ab(a, 0x48); ab(a, 0x8B); ab(a, 0x0C); ab(a, 0xFB);
    ab(a, 0x48); ab(a, 0x85); ab(a, 0xC9);
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF2);             /* mov rdx, rsi */
    ab(a, 0x4C); ab(a, 0x8D); ab(a, 0x44); ab(a, 0x24); ab(a, 0x38); /* lea r8, [rsp+0x38] */
    ab(a, 0x4D); ab(a, 0x89); ab(a, 0xE9);             /* mov r9, r13 */
    zero_slot38(a);
    call_iat(a, IMP_SetFilePointerEx);
    ab(a, 0x85); ab(a, 0xC0);
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x48); ab(a, 0x8B); ab(a, 0x44); ab(a, 0x24); ab(a, 0x38);
    size_t lsk_done = j32(a);

    /* mmap: rsi = length.  addr/prot/flags/fd/offset are ignored. */
    handler[9] = a->buf.len;
    ab(a, 0x31); ab(a, 0xC9);                          /* xor ecx, ecx */
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF2);             /* mov rdx, rsi */
    ab(a, 0x41); ab(a, 0xB8); au32(a, 0x3000);         /* MEM_COMMIT|MEM_RESERVE */
    ab(a, 0x41); ab(a, 0xB9); au32(a, 4);              /* PAGE_READWRITE */
    call_iat(a, IMP_VirtualAlloc);
    ab(a, 0x48); ab(a, 0x85); ab(a, 0xC0);
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    size_t map_done = j32(a);

    /* munmap */
    handler[11] = a->buf.len;
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF9);             /* mov rcx, rdi */
    ab(a, 0x31); ab(a, 0xD2);
    ab(a, 0x41); ab(a, 0xB8); au32(a, 0x8000);         /* MEM_RELEASE */
    call_iat(a, IMP_VirtualFree);
    ab(a, 0x85); ab(a, 0xC0);
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x31); ab(a, 0xC0);
    size_t unmap_done = j32(a);

    /* getpid */
    handler[39] = a->buf.len;
    call_iat(a, IMP_GetCurrentProcessId);
    ab(a, 0x89); ab(a, 0xC0);                          /* mov eax, eax */
    size_t pid_done = j32(a);

    /* gettid */
    handler[186] = a->buf.len;
    call_iat(a, IMP_GetCurrentThreadId);
    ab(a, 0x89); ab(a, 0xC0);                          /* mov eax, eax */
    size_t tid_done = j32(a);

    /* exit / exit_group */
    handler[60] = a->buf.len;
    handler[231] = handler[60];
    /* Linux reports only the low 8 bits of the exit status.  ExitProcess
     * keeps all 32, and MSYS then turns any status above 255 into 127, so
     * `return -3` would not come back as 253 the way the suites expect. */
    ab(a, 0x89); ab(a, 0xF9);                          /* mov ecx, edi */
    ab(a, 0x81); ab(a, 0xE1); au32(a, 0xFFu);          /* and ecx, 0xff */
    call_iat(a, IMP_ExitProcess);
    size_t exit_done = j32(a);

    /* rename */
    handler[82] = a->buf.len;
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF9);
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF2);
    call_iat(a, IMP_MoveFileA);
    ab(a, 0x85); ab(a, 0xC0);
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x31); ab(a, 0xC0);
    size_t ren_done = j32(a);

    /* mkdir */
    handler[83] = a->buf.len;
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF9);
    ab(a, 0x31); ab(a, 0xD2);
    call_iat(a, IMP_CreateDirectoryA);
    ab(a, 0x85); ab(a, 0xC0);
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x31); ab(a, 0xC0);
    size_t mk_done = j32(a);

    /* unlink */
    handler[87] = a->buf.len;
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF9);
    call_iat(a, IMP_DeleteFileA);
    ab(a, 0x85); ab(a, 0xC0);
    fail_slots[(*nfail)++] = jcc(a, 0x84);
    ab(a, 0x31); ab(a, 0xC0);
    size_t ul_done = j32(a);

    /* chmod: no Unix mode bits.  Success keeps portable callers moving. */
    handler[90] = a->buf.len;
    ab(a, 0x31); ab(a, 0xC0);
    size_t ch_done = j32(a);

    /* FindFirstFileA(path, find_data) -> handle, or -1. */
    handler[200] = a->buf.len;
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF9);             /* mov rcx, rdi */
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF2);             /* mov rdx, rsi */
    call_iat(a, IMP_FindFirstFileA);
    size_t find_done = j32(a);

    /* FindNextFileA(handle, find_data) -> BOOL, zero-extended. */
    handler[201] = a->buf.len;
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF9);
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF2);
    call_iat(a, IMP_FindNextFileA);
    ab(a, 0x89); ab(a, 0xC0);                          /* mov eax, eax */
    size_t next_done = j32(a);

    /* FindClose(handle). */
    handler[202] = a->buf.len;
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xF9);
    call_iat(a, IMP_FindClose);
    ab(a, 0x89); ab(a, 0xC0);
    size_t fclose_done = j32(a);

    size_t done = a->buf.len;
    ab(a, 0x48); ab(a, 0x83); ab(a, 0xC4); ab(a, 0x40); /* add rsp, 0x40 */
    ab(a, 0x41); ab(a, 0x5D);                          /* pop r13 */
    ab(a, 0x41); ab(a, 0x5C);                          /* pop r12 */
    ab(a, 0x5B);                                       /* pop rbx */
    ab(a, 0xC3);                                       /* ret */

    size_t fail = a->buf.len;
    ab(a, 0x48); ab(a, 0xC7); ab(a, 0xC0); au32(a, 0xFFFFFFFFu); /* mov rax, -1 */
    size_t to_done = j32(a);

    apatch(a, to_fail, fail);
    apatch(a, to_done, done);
    apatch(a, rd_done, done);
    apatch(a, wr_done, done);
    apatch(a, open_done, done);
    apatch(a, close_done, done);
    apatch(a, lsk_done, done);
    apatch(a, map_done, done);
    apatch(a, unmap_done, done);
    apatch(a, pid_done, done);
    apatch(a, tid_done, done);
    apatch(a, exit_done, done);
    apatch(a, ren_done, done);
    apatch(a, mk_done, done);
    apatch(a, ul_done, done);
    apatch(a, ch_done, done);
    apatch(a, find_done, done);
    apatch(a, next_done, done);
    apatch(a, fclose_done, done);
    for (int i = 0; i < *nfail; i++) apatch(a, fail_slots[i], fail);
    for (int i = 0; i < nb; i++) apatch(a, br[i].slot, handler[br[i].num]);

#undef FAIL
#undef JA_FAIL
}

/* ------------------------------------------------------------------ */
/* Process entry                                                       */
/* ------------------------------------------------------------------ */

static void emit_dyn_copy(Asm *a, uint32_t dyn_off, uint32_t dyn_n) {
    if (!dyn_n) return;
    /* lea r10, [rip+table]; mov r11d, count; copy *iat into each slot. */
    ab(a, 0x4C); ab(a, 0x8D); ab(a, 0x15);
    add_fix(a, FK_RDATA, (int)dyn_off, NULL);
    ab(a, 0x41); ab(a, 0xBB); au32(a, dyn_n);          /* mov r11d, count */
    size_t loop = a->buf.len;
    ab(a, 0x49); ab(a, 0x8B); ab(a, 0x02);             /* mov rax, [r10] */
    ab(a, 0x49); ab(a, 0x8B); ab(a, 0x4A); ab(a, 0x08);/* mov rcx, [r10+8] */
    ab(a, 0x48); ab(a, 0x8B); ab(a, 0x09);             /* mov rcx, [rcx] */
    ab(a, 0x48); ab(a, 0x89); ab(a, 0x08);             /* mov [rax], rcx */
    ab(a, 0x49); ab(a, 0x83); ab(a, 0xC2); ab(a, 0x10);/* add r10, 16 */
    ab(a, 0x41); ab(a, 0xFF); ab(a, 0xCB);             /* dec r11d */
    size_t jnz = jcc(a, 0x85);
    apatch(a, jnz, loop);
}

static void emit_winmain(Asm *a, int have_exit, int have_env,
                         uint32_t init_off, uint32_t init_n,
                         uint32_t fini_off, uint32_t fini_n,
                         uint32_t dyn_off, uint32_t dyn_n) {
    ab(a, 0x55);                                      /* push rbp */
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xE5);             /* mov rbp, rsp */
    ab(a, 0x48); ab(a, 0x83); ab(a, 0xEC); ab(a, 0x20); /* sub rsp, 32 */
    emit_dyn_copy(a, dyn_off, dyn_n);

    /* Standard handles into fd[0..2].  -10/-11/-12. */
    static const uint32_t stdh[3] = { 0xFFFFFFF6u, 0xFFFFFFF5u, 0xFFFFFFF4u };
    for (int i = 0; i < 3; i++) {
        ab(a, 0xB9); au32(a, stdh[i]);                /* mov ecx, imm */
        call_iat(a, IMP_GetStdHandle);
        lea_rip(a, 2, FK_SYNTH, SYN_FD);              /* lea rdx, fd */
        if (i == 0) {
            ab(a, 0x48); ab(a, 0x89); ab(a, 0x02);    /* mov [rdx], rax */
        } else {
            ab(a, 0x48); ab(a, 0x89); ab(a, 0x42); ab(a, (uint8_t)(i * 8));
        }
    }

    if (have_env) {
        call_iat(a, IMP_GetEnvironmentStringsA);
        store_rax_sym(a, "__fakecc_win_env");
    }

    call_iat(a, IMP_GetCommandLineA);
    ab(a, 0x48); ab(a, 0x85); ab(a, 0xC0);
    size_t have_cmd = jcc(a, 0x85);                   /* jnz */
    lea_rip(a, 6, FK_SYNTH, SYN_CMDLINE);             /* lea rsi, cmdline (zeros) */
    size_t skip_copy = j32(a);
    apatch(a, have_cmd, a->buf.len);
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xC6);             /* mov rsi, rax */
    lea_rip(a, 7, FK_SYNTH, SYN_CMDLINE);             /* lea rdi, cmdline */
    ab(a, 0x31); ab(a, 0xC9);                          /* xor ecx, ecx */
    size_t copy = a->buf.len;
    ab(a, 0x8A); ab(a, 0x06);                          /* mov al, [rsi] */
    ab(a, 0x88); ab(a, 0x07);                          /* mov [rdi], al */
    ab(a, 0x84); ab(a, 0xC0);
    size_t copied = jcc(a, 0x84);                      /* jz */
    ab(a, 0x48); ab(a, 0xFF); ab(a, 0xC6);             /* inc rsi */
    ab(a, 0x48); ab(a, 0xFF); ab(a, 0xC7);             /* inc rdi */
    ab(a, 0xFF); ab(a, 0xC1);                          /* inc ecx */
    ab(a, 0x81); ab(a, 0xF9); au32(a, 4094);
    size_t more = jcc(a, 0x82);                        /* jb copy */
    apatch(a, more, copy);
    ab(a, 0xC6); ab(a, 0x07); ab(a, 0x00);             /* mov byte [rdi], 0 */
    apatch(a, copied, a->buf.len);
    apatch(a, skip_copy, a->buf.len);

    lea_rip(a, 6, FK_SYNTH, SYN_CMDLINE);
    lea_rip(a, 7, FK_SYNTH, SYN_ARGV);
    ab(a, 0x31); ab(a, 0xDB);                          /* xor ebx, ebx */
    size_t skip = a->buf.len;
    ab(a, 0x8A); ab(a, 0x06);
    ab(a, 0x3C); ab(a, 0x20);
    size_t sp1 = jcc(a, 0x84);
    ab(a, 0x3C); ab(a, 0x09);
    size_t sp2 = jcc(a, 0x84);
    ab(a, 0x84); ab(a, 0xC0);
    size_t end1 = jcc(a, 0x84);
    size_t tok = j32(a);
    size_t space = a->buf.len;
    apatch(a, sp1, space);
    apatch(a, sp2, space);
    ab(a, 0x48); ab(a, 0xFF); ab(a, 0xC6);
    size_t back_skip = j32(a);
    apatch(a, back_skip, skip);
    size_t tok_at = a->buf.len;
    apatch(a, tok, tok_at);
    ab(a, 0x83); ab(a, 0xFB); ab(a, 63);
    size_t end2 = jcc(a, 0x83);                        /* jae */
    /* A leading '"' starts a quoted token.  r13d is 1 for that token so
     * the scan stops at the closer instead of at whitespace, and the
     * quote characters themselves are not part of argv. */
    ab(a, 0x45); ab(a, 0x31); ab(a, 0xED);             /* xor r13d, r13d */
    ab(a, 0x80); ab(a, 0x3E); ab(a, 0x22);             /* cmp byte [rsi], '"' */
    size_t unquoted = jcc(a, 0x85);                    /* jnz */
    ab(a, 0x48); ab(a, 0xFF); ab(a, 0xC6);             /* inc rsi */
    ab(a, 0x41); ab(a, 0xB5); ab(a, 0x01);             /* mov r13b, 1 */
    apatch(a, unquoted, a->buf.len);
    ab(a, 0x48); ab(a, 0x89); ab(a, 0x34); ab(a, 0xDF); /* mov [rdi+rbx*8], rsi */
    ab(a, 0xFF); ab(a, 0xC3);                          /* inc ebx */
    size_t tloop = a->buf.len;
    ab(a, 0x8A); ab(a, 0x06);
    ab(a, 0x84); ab(a, 0xC0);
    size_t end3 = jcc(a, 0x84);
    ab(a, 0x45); ab(a, 0x84); ab(a, 0xED);             /* test r13b, r13b */
    size_t plain = jcc(a, 0x84);                       /* jz */
    ab(a, 0x3C); ab(a, 0x22);                          /* cmp al, '"' */
    size_t tendq = jcc(a, 0x84);                       /* je */
    size_t adv = j32(a);
    apatch(a, plain, a->buf.len);
    ab(a, 0x3C); ab(a, 0x20);
    size_t tend1 = jcc(a, 0x84);
    ab(a, 0x3C); ab(a, 0x09);
    size_t tend2 = jcc(a, 0x84);
    apatch(a, adv, a->buf.len);
    ab(a, 0x48); ab(a, 0xFF); ab(a, 0xC6);
    size_t back_tok = j32(a);
    apatch(a, back_tok, tloop);
    size_t tend = a->buf.len;
    apatch(a, tend1, tend);
    apatch(a, tend2, tend);
    apatch(a, tendq, tend);
    ab(a, 0xC6); ab(a, 0x06); ab(a, 0x00);             /* mov byte [rsi], 0 */
    ab(a, 0x48); ab(a, 0xFF); ab(a, 0xC6);
    size_t back_ws = j32(a);
    apatch(a, back_ws, skip);
    size_t parsed = a->buf.len;
    apatch(a, end1, parsed);
    apatch(a, end2, parsed);
    apatch(a, end3, parsed);
    ab(a, 0x48); ab(a, 0xC7); ab(a, 0x04); ab(a, 0xDF); au32(a, 0);
    ab(a, 0x41); ab(a, 0x89); ab(a, 0xDC);             /* mov r12d, ebx */

    if (init_n) {
        lea_rip(a, 3, FK_RDATA, (int)init_off);       /* lea rbx, init */
        ab(a, 0x41); ab(a, 0xBD); au32(a, init_n);    /* mov r13d, count */
        size_t cl = a->buf.len;
        ab(a, 0xFF); ab(a, 0x13);                      /* call [rbx] */
        ab(a, 0x48); ab(a, 0x83); ab(a, 0xC3); ab(a, 0x08);
        ab(a, 0x49); ab(a, 0xFF); ab(a, 0xCD);         /* dec r13 */
        size_t jnz = jcc(a, 0x85);
        apatch(a, jnz, cl);
    }

    ab(a, 0x44); ab(a, 0x89); ab(a, 0xE1);             /* mov ecx, r12d */
    lea_rip(a, 2, FK_SYNTH, SYN_ARGV);                /* lea rdx, argv */
    call_sym(a, "main");
    ab(a, 0x41); ab(a, 0x89); ab(a, 0xC4);             /* mov r12d, eax */

    if (fini_n) {
        lea_rip(a, 3, FK_RDATA, (int)fini_off);
        ab(a, 0x41); ab(a, 0xBD); au32(a, fini_n);
        size_t fl = a->buf.len;
        ab(a, 0xFF); ab(a, 0x13);
        ab(a, 0x48); ab(a, 0x83); ab(a, 0xEB); ab(a, 0x08); /* sub rbx, 8 */
        ab(a, 0x49); ab(a, 0xFF); ab(a, 0xCD);
        size_t jnz = jcc(a, 0x85);
        apatch(a, jnz, fl);
    }

    ab(a, 0x44); ab(a, 0x89); ab(a, 0xE1);             /* mov ecx, r12d */
    if (have_exit) call_sym(a, "exit");
    ab(a, 0x44); ab(a, 0x89); ab(a, 0xE1);             /* mov ecx, r12d */
    ab(a, 0x81); ab(a, 0xE1); au32(a, 0xFFu);          /* and ecx, 0xff */
    call_iat(a, IMP_ExitProcess);
    ab(a, 0x0F); ab(a, 0x0B);                          /* ud2 */
}

/* call [r10] in a counted loop.  `off` is the rdata offset of the first
 * (or, when walking backwards, the last) function pointer. */
static void emit_r10_loop(Asm *a, uint32_t off, uint32_t n, int backward) {
    if (!n) return;
    ab(a, 0x4C); ab(a, 0x8D); ab(a, 0x15);             /* lea r10, [rip+d] */
    add_fix(a, FK_RDATA, (int)off, NULL);
    ab(a, 0x41); ab(a, 0xBB); au32(a, n);              /* mov r11d, count */
    size_t loop = a->buf.len;
    ab(a, 0x41); ab(a, 0xFF); ab(a, 0x12);             /* call [r10] */
    if (backward) {
        ab(a, 0x49); ab(a, 0x83); ab(a, 0xEA); ab(a, 0x08);
    } else {
        ab(a, 0x49); ab(a, 0x83); ab(a, 0xC2); ab(a, 0x08);
    }
    ab(a, 0x41); ab(a, 0xFF); ab(a, 0xCB);             /* dec r11d */
    size_t jnz = jcc(a, 0x85);
    apatch(a, jnz, loop);
}

/* Loader calls this with the Win64 convention (rcx = hinst, rdx = reason,
 * r8 = reserved).  Copy imported-data fixups and run .init_array only on
 * process attach; call DllMain for every reason; run .fini_array on
 * process detach.  Returns 1 when the image has no DllMain. */
static void emit_dllentry(Asm *a, int have_dllmain,
                          uint32_t init_off, uint32_t init_n,
                          uint32_t fini_off, uint32_t fini_n,
                          uint32_t dyn_off, uint32_t dyn_n) {
    ab(a, 0x55);                                      /* push rbp */
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xE5);             /* mov rbp, rsp */
    ab(a, 0x53);                                      /* push rbx */
    ab(a, 0x41); ab(a, 0x54);                          /* push r12 */
    ab(a, 0x41); ab(a, 0x55);                          /* push r13 */
    ab(a, 0x41); ab(a, 0x56);                          /* push r14 */
    ab(a, 0x48); ab(a, 0x83); ab(a, 0xEC); ab(a, 0x20);/* sub rsp, 32 */
    ab(a, 0x48); ab(a, 0x89); ab(a, 0xCB);             /* mov rbx, rcx */
    ab(a, 0x49); ab(a, 0x89); ab(a, 0xD4);             /* mov r12, rdx */
    ab(a, 0x4D); ab(a, 0x89); ab(a, 0xC5);             /* mov r13, r8 */
    ab(a, 0x41); ab(a, 0xBE); au32(a, 1);              /* mov r14d, 1 */
    ab(a, 0x41); ab(a, 0x83); ab(a, 0xFC); ab(a, 1);   /* cmp r12d, 1 */
    size_t not_attach = jcc(a, 0x85);                  /* jne */
    emit_dyn_copy(a, dyn_off, dyn_n);
    emit_r10_loop(a, init_off, init_n, 0);
    apatch(a, not_attach, a->buf.len);
    if (have_dllmain) {
        ab(a, 0x48); ab(a, 0x89); ab(a, 0xD9);         /* mov rcx, rbx */
        ab(a, 0x4C); ab(a, 0x89); ab(a, 0xE2);         /* mov rdx, r12 */
        ab(a, 0x4D); ab(a, 0x89); ab(a, 0xE8);         /* mov r8, r13 */
        call_sym(a, "DllMain");
        ab(a, 0x41); ab(a, 0x89); ab(a, 0xC6);         /* mov r14d, eax */
    }
    ab(a, 0x45); ab(a, 0x85); ab(a, 0xE4);             /* test r12d, r12d */
    size_t not_detach = jcc(a, 0x85);                  /* jnz */
    emit_r10_loop(a, fini_off, fini_n, 1);
    apatch(a, not_detach, a->buf.len);
    ab(a, 0x44); ab(a, 0x89); ab(a, 0xF0);             /* mov eax, r14d */
    ab(a, 0x48); ab(a, 0x83); ab(a, 0xC4); ab(a, 0x20);
    ab(a, 0x41); ab(a, 0x5E);                          /* pop r14 */
    ab(a, 0x41); ab(a, 0x5D);                          /* pop r13 */
    ab(a, 0x41); ab(a, 0x5C);                          /* pop r12 */
    ab(a, 0x5B);                                      /* pop rbx */
    ab(a, 0x5D);                                      /* pop rbp */
    ab(a, 0xC3);                                      /* ret */
}

/* ------------------------------------------------------------------ */
/* Imports                                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *dll;
    const char *name;
    int is_data;
    int ordinal;
    int hint;
    int fixed;           /* IMP_* index, or -1 */
    int publish;         /* define the symbol / thunk for user code */
    uint32_t iat_boff;   /* byte offset of the IAT slot inside the blob */
    size_t thunk_off;    /* offset of the jmp thunk inside the stub */
} ImpSlot;

typedef struct {
    Buffer blob;
    uint32_t iat_off;     /* within blob: first byte of the contiguous IATs */
    uint32_t dir_off;
    uint32_t dir_size;
    uint32_t iat_size;
    uint32_t stub_boff[IMP_COUNT];
} Imports;

static int icmp_dll(const char *a, const char *b) {
    if (!a || !b) return a ? 1 : (b ? -1 : 0);
    while (*a && *b) {
        unsigned char ca = (unsigned char)*a, cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb - 'A' + 'a');
        if (ca != cb) return (int)ca - (int)cb;
        a++; b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static int build_imports(Imports *im, ImpSlot *slots, size_t nslots,
                         uint32_t rdata_rva, uint32_t blob_off) {
    buffer_init(&im->blob);
    memset(im->stub_boff, 0, sizeof im->stub_boff);
    im->iat_off = 0;
    im->dir_off = 0;
    im->dir_size = 0;
    im->iat_size = 0;
    if (!nslots) return 0;

    /* DLL order is first-seen, so KERNEL32 (added first) leads. */
    const char **dlls = NULL;
    size_t ndll = 0;
    for (size_t i = 0; i < nslots; i++) {
        size_t d;
        for (d = 0; d < ndll; d++)
            if (icmp_dll(dlls[d], slots[i].dll) == 0) break;
        if (d == ndll) {
            dlls = xrealloc(dlls, (ndll + 1) * sizeof(char *));
            dlls[ndll++] = slots[i].dll;
        }
    }

    size_t *hint_at = xmalloc(nslots * sizeof(size_t));
    for (size_t i = 0; i < nslots; i++) {
        if (slots[i].ordinal > 0) { hint_at[i] = (size_t)-1; continue; }
        hint_at[i] = im->blob.len;
        bu16(&im->blob, (uint16_t)(slots[i].hint > 0 ? slots[i].hint : 0));
        buffer_append(&im->blob, slots[i].name, strlen(slots[i].name) + 1);
        if (im->blob.len & 1) bu8(&im->blob, 0);
    }
    size_t *dll_at = xmalloc(ndll * sizeof(size_t));
    for (size_t d = 0; d < ndll; d++) {
        dll_at[d] = im->blob.len;
        buffer_append(&im->blob, dlls[d], strlen(dlls[d]) + 1);
    }
    while (im->blob.len & 7) bu8(&im->blob, 0);

    size_t *ilt_at = xmalloc(ndll * sizeof(size_t));
    for (size_t d = 0; d < ndll; d++) {
        ilt_at[d] = im->blob.len;
        for (size_t i = 0; i < nslots; i++) {
            if (icmp_dll(slots[i].dll, dlls[d]) != 0) continue;
            uint64_t ent;
            if (slots[i].ordinal > 0)
                ent = (1ULL << 63) | (uint64_t)(uint16_t)slots[i].ordinal;
            else
                ent = (uint64_t)rdata_rva + blob_off + hint_at[i];
            bu64(&im->blob, ent);
        }
        bu64(&im->blob, 0);
    }

    im->iat_off = (uint32_t)im->blob.len;
    for (size_t d = 0; d < ndll; d++) {
        for (size_t i = 0; i < nslots; i++) {
            if (icmp_dll(slots[i].dll, dlls[d]) != 0) continue;
            slots[i].iat_boff = (uint32_t)im->blob.len;
            if (slots[i].fixed >= 0 && slots[i].fixed < IMP_COUNT)
                im->stub_boff[slots[i].fixed] = slots[i].iat_boff;
            uint64_t ent;
            if (slots[i].ordinal > 0)
                ent = (1ULL << 63) | (uint64_t)(uint16_t)slots[i].ordinal;
            else
                ent = (uint64_t)rdata_rva + blob_off + hint_at[i];
            bu64(&im->blob, ent);
        }
        bu64(&im->blob, 0);
    }
    im->iat_size = (uint32_t)im->blob.len - im->iat_off;

    im->dir_off = (uint32_t)im->blob.len;
    im->dir_size = (uint32_t)((ndll + 1) * 20);
    for (size_t d = 0; d < ndll; d++) {
        /* FirstThunk is this DLL's first IAT slot.  Find it. */
        uint32_t ft = 0;
        int got = 0;
        for (size_t i = 0; i < nslots; i++) {
            if (icmp_dll(slots[i].dll, dlls[d]) != 0) continue;
            ft = slots[i].iat_boff;
            got = 1;
            break;
        }
        bu32(&im->blob, rdata_rva + blob_off + (uint32_t)ilt_at[d]);
        bu32(&im->blob, 0);
        bu32(&im->blob, 0);
        bu32(&im->blob, rdata_rva + blob_off + (uint32_t)dll_at[d]);
        bu32(&im->blob, got ? rdata_rva + blob_off + ft : 0);
    }
    bpad(&im->blob, 20);
    free(hint_at);
    free(dll_at);
    free(ilt_at);
    free(dlls);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Symbols and relocations                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    uint64_t addr;
    int from_user;
    int strong;
    int imp;             /* 2: data import, addr/iat_rva name the IAT slot */
    uint32_t iat_rva;
} GSym;

static int gsym_find(GSym *g, size_t n, const char *name) {
    for (size_t i = 0; i < n; i++)
        if (strcmp(g[i].name, name) == 0) return (int)i;
    return -1;
}

static int gsym_add(GSym **g, size_t *n, size_t *cap, const char *name,
                    uint64_t addr, int from_user, int strong, int allow_dup) {
    int ix = gsym_find(*g, *n, name);
    if (ix < 0) {
        if (*n == *cap) {
            *cap = *cap ? *cap * 2 : 64;
            *g = xrealloc(*g, *cap * sizeof(GSym));
        }
        (*g)[*n].name = name;
        (*g)[*n].addr = addr;
        (*g)[*n].from_user = from_user;
        (*g)[*n].strong = strong;
        (*g)[*n].imp = 0;
        (*g)[*n].iat_rva = 0;
        (*n)++;
        return 0;
    }
    GSym *e = &(*g)[ix];
    if (strong && !e->strong) {
        e->addr = addr; e->from_user = from_user; e->strong = 1;
        return 0;
    }
    if (!strong && e->strong) return 0;
    if (from_user && !e->from_user) {
        e->addr = addr; e->from_user = 1; e->strong = strong || e->strong;
        return 0;
    }
    if (from_user && e->from_user && strong && e->strong) {
        if (allow_dup) return 0; /* COMDAT: keep the first definition */
        fprintf(stderr, "fakecc: duplicate symbol '%s'\n", name);
        return -1;
    }
    return 0;
}

static int sym_defined_here(const EmitSymbol *s) {
    return s->shndx != SECT_UNDEF && s->shndx != SHN_COMMON && s->name;
}

typedef struct {
    uint64_t text_va, rdata_va, data_va;
    size_t synth_off;
    size_t *mod_text, *mod_ro, *mod_data, *mod_bss;
    size_t data_file;          /* file-backed .data bytes, including synth */
    Buffer *text, *rodata, *data, *pdata, *xdata;
    size_t init_at, fini_at;   /* offset of those arrays inside rdata */
    uint64_t xdata_va, pdata_va;
    size_t *mod_xdata, *mod_pdata;
    uint64_t tls_va;
    size_t *mod_tdata, *mod_tbss;
    size_t tbss_off;
    Buffer *tdata;
} Lay;

static int comdat_kept(const EmitModule *m, int idx) {
    if (!m || idx < 0) return 1;
    if ((size_t)idx >= m->num_comdats) return 1;
    return m->comdats[idx].keep != 0;
}

/* Map an offset in a module's original section buffer onto that module's
 * kept (post-COMDAT) contribution.  Returns 0 when the offset sits in a
 * group the linker dropped. */
static int remap_off(const EmitModule *m, uint16_t kind, size_t old, size_t *out) {
    if (!m || !m->num_spans) { *out = old; return 1; }
    int any = 0;
    size_t cursor = 0;
    for (size_t i = 0; i < m->num_spans; i++) {
        const EmitSpan *sp = &m->spans[i];
        if (sp->kind != kind) continue;
        any = 1;
        if (!comdat_kept(m, sp->comdat)) continue;
        if (old >= sp->off && old < sp->off + sp->len) {
            *out = cursor + (old - sp->off);
            return 1;
        }
        cursor += sp->len;
    }
    if (!any) { *out = old; return 1; }
    return 0;
}

static int local_addr(const Lay *L, const EmitModule *m, size_t mi,
                      const EmitSymbol *s, uint64_t *out) {
    size_t off = s->value;
    if (!remap_off(m, s->shndx, s->value, &off)) return -2;
    switch (s->shndx) {
    case SECT_TEXT:   *out = L->text_va + L->mod_text[mi] + off; return 0;
    case SECT_RODATA: *out = L->rdata_va + L->mod_ro[mi] + off; return 0;
    case SECT_DATA:   *out = L->data_va + L->mod_data[mi] + off; return 0;
    case SECT_BSS:    *out = L->data_va + L->data_file + L->mod_bss[mi] + off; return 0;
    case SECT_XDATA:  *out = L->xdata_va + L->mod_xdata[mi] + off; return 0;
    case SECT_PDATA:  *out = L->pdata_va + L->mod_pdata[mi] + off; return 0;
    case SECT_TDATA:  *out = L->tls_va + (L->mod_tdata ? L->mod_tdata[mi] : 0) + off; return 0;
    case SECT_TBSS:   *out = L->tls_va + L->tbss_off + (L->mod_tbss ? L->mod_tbss[mi] : 0) + off; return 0;
    case SECT_TLS_CALLBACKS:
        *out = L->rdata_va;
        return 0;
    case SECT_INIT_ARRAY:
        *out = L->rdata_va + L->init_at;
        return -1;
    default:
        fprintf(stderr, "fakecc: symbol '%s' is in a section the Windows "
                "target does not support\n", s->name ? s->name : "(anon)");
        return -1;
    }
}

static int resolve_sym(const Lay *L, EmitModule **mods, GSym *gs, size_t ngs,
                       size_t mi, uint32_t si, uint64_t *out,
                       int *imp, uint32_t *iat_rva) {
    if (imp) *imp = 0;
    if (iat_rva) *iat_rva = 0;
    if (si >= mods[mi]->num_syms) {
        fprintf(stderr, "fakecc: relocation has a bad symbol index\n");
        return -1;
    }
    const EmitSymbol *s = &mods[mi]->syms[si];
    if (!s->name) {
        fprintf(stderr, "fakecc: relocation against an unnamed symbol\n");
        return -1;
    }
    /* A local definition is private to this module.  Globals (and weak
     * symbols) go through the merged table so a user definition wins. */
    if (s->binding == 0 && sym_defined_here(s)) {
        int lr = local_addr(L, mods[mi], mi, s, out);
        if (lr == -2) {
            fprintf(stderr, "fakecc: relocation against discarded symbol '%s'\n",
                    s->name);
            return -1;
        }
        return lr;
    }
    int ix = gsym_find(gs, ngs, s->name);
    if (ix < 0) {
        if (s->name && s->name[0] == '?') {
            fprintf(stderr, "fakecc: undefined symbol '%s' (unsupported C++ mangled symbol)\n", s->name);
        } else {
            fprintf(stderr, "fakecc: undefined symbol '%s'\n", s->name);
        }
        return -1;
    }
    *out = gs[ix].addr;
    if (imp) *imp = gs[ix].imp;
    if (iat_rva) *iat_rva = gs[ix].iat_rva;
    return 0;
}

static int add_abs(uint32_t **rv, size_t *n, size_t *cap, uint32_t rva) {
    if (*n == *cap) {
        *cap = *cap ? *cap * 2 : 64;
        *rv = xrealloc(*rv, *cap * sizeof(uint32_t));
    }
    (*rv)[(*n)++] = rva;
    return 0;
}

static int section_va_of(const Lay *L, EmitModule **mods, size_t nmods,
                         size_t mi, uint32_t si, uint64_t *base) {
    const EmitSymbol *s = &mods[mi]->syms[si];
    uint16_t sh = s->shndx;
    if (sh == SECT_UNDEF || sh == SHN_COMMON || !s->name) {
        sh = SECT_UNDEF;
        if (s->name) {
            for (size_t i = 0; i < nmods && sh == SECT_UNDEF; i++) {
                for (size_t k = 0; k < mods[i]->num_syms; k++) {
                    EmitSymbol *sy = &mods[i]->syms[k];
                    if (!sy->name || sy->binding == 0) continue;
                    if (sy->shndx == SECT_UNDEF || sy->shndx == SHN_COMMON) continue;
                    if (strcmp(sy->name, s->name) != 0) continue;
                    sh = sy->shndx;
                    break;
                }
            }
        }
    }
    switch (sh) {
    case SECT_TEXT:   *base = L->text_va; return 0;
    case SECT_RODATA: *base = L->rdata_va; return 0;
    case SECT_DATA:
    case SECT_BSS:    *base = L->data_va; return 0;
    case SECT_XDATA:  *base = L->xdata_va; return 0;
    case SECT_PDATA:  *base = L->pdata_va; return 0;
    case SECT_TDATA:
    case SECT_TBSS:   *base = L->tls_va; return 0;
    case SECT_TLS_CALLBACKS: *base = L->rdata_va; return 0;
    default:
        *base = L->rdata_va;
        return 0;
    }
}

static int apply_sec_relocs(const Lay *L, EmitModule **mods, size_t nmods,
                            GSym *gs, size_t ngs, size_t mi,
                            EmitReloc *rels, size_t nrels,
                            Buffer *buf, size_t *mod_off, uint16_t kind) {
    if (!nrels || !buf || !buf->data || !mod_off) return 0;
    EmitModule *m = mods[mi];
    for (size_t r = 0; r < nrels; r++) {
        EmitReloc *rel = &rels[r];
        size_t roff;
        if (!remap_off(m, kind, rel->offset, &roff)) continue;
        size_t poff = mod_off[mi] + roff;
        if (poff + 4 > buf->len) {
            fprintf(stderr, "fakecc: section relocation past the end of the "
                    "output section\n");
            return -1;
        }
        uint64_t S;
        if (resolve_sym(L, mods, gs, ngs, mi, rel->sym, &S, NULL, NULL) != 0)
            return -1;
        if (rel->type == R_PE_ADDR32NB || rel->type == R_X86_64_32) {
            uint32_t v = (uint32_t)((S + (int64_t)rel->addend) - IMAGE_BASE);
            memcpy(buf->data + poff, &v, 4);
        } else if (rel->type == R_PE_SECREL || rel->type == R_X86_64_TPOFF32 || rel->type == R_X86_64_DTPOFF32) {
            uint64_t base = 0;
            if (section_va_of(L, mods, nmods, mi, rel->sym, &base) != 0)
                return -1;
            uint32_t v = (uint32_t)(S + (int64_t)rel->addend - base);
            memcpy(buf->data + poff, &v, 4);
        } else if (rel->type == R_PE_SECTION) {
            uint16_t snum = 1;
            if (poff + 2 > buf->len) return -1;
            memcpy(buf->data + poff, &snum, 2);
        } else if (rel->type == R_X86_64_64) {
            uint64_t val = S + (int64_t)rel->addend;
            if (poff + 8 > buf->len) return -1;
            memcpy(buf->data + poff, &val, 8);
        } else {
            fprintf(stderr, "fakecc: section relocation type %u is not "
                    "supported on the x86_64-windows target\n", rel->type);
            return -1;
        }
    }
    (void)kind;
    return 0;
}

/* A data import's address is not known until the loader fills the IAT.
 * GOTPCREL loads through that slot.  An absolute pointer in .data is
 * copied from the IAT at process/DLL attach (see the dyn table). */
static int apply_relocs(const Lay *L, EmitModule **mods, size_t nmods,
                        GSym *gs, size_t ngs,
                        uint32_t **abs, size_t *nabs, size_t *capabs,
                        uint32_t got_rva, size_t dyn_at, size_t dyn_cap,
                        size_t *ndyn) {
    uint32_t slot = 0;
    for (size_t mi = 0; mi < nmods; mi++) {
        EmitModule *m = mods[mi];
        for (size_t r = 0; r < m->num_relocs; r++) {
            EmitReloc *rel = &m->relocs[r];
            size_t roff;
            if (!remap_off(m, SECT_TEXT, rel->offset, &roff)) continue;
            uint64_t S;
            int imp = 0;
            uint32_t iat_rva = 0;
            if (resolve_sym(L, mods, gs, ngs, mi, rel->sym, &S,
                            &imp, &iat_rva) != 0)
                return -1;
            size_t poff = L->mod_text[mi] + roff;
            uint64_t P = L->text_va + poff;
            if (rel->type == R_X86_64_PC32 || rel->type == R_X86_64_PLT32) {
                int64_t disp = (int64_t)S + rel->addend - (int64_t)P;
                if (disp < (int64_t)INT32_MIN || disp > (int64_t)INT32_MAX) {
                    fprintf(stderr, "fakecc: PC relocation out of range\n");
                    return -1;
                }
                int32_t d = (int32_t)disp;
                memcpy(L->text->data + poff, &d, 4);
            } else if (rel->type == R_X86_64_64) {
                if (imp == 2) {
                    fprintf(stderr, "fakecc: pointer to imported data is not "
                            "in a writable section\n");
                    return -1;
                }
                uint64_t val = S + (int64_t)rel->addend;
                memcpy(L->text->data + poff, &val, 8);
                add_abs(abs, nabs, capabs, (uint32_t)(L->text_va - IMAGE_BASE + poff));
            } else if (rel->type == R_X86_64_GOTPCREL) {
                uint64_t slot_va;
                if (imp == 2) {
                    /* Load the remote address straight out of the IAT. */
                    slot_va = IMAGE_BASE + iat_rva;
                } else {
                    slot_va = IMAGE_BASE + got_rva + (uint64_t)slot * 8;
                    uint64_t symva = S;
                    memcpy(L->rodata->data + (got_rva - (uint32_t)(L->rdata_va - IMAGE_BASE)) + (size_t)slot * 8,
                           &symva, 8);
                    add_abs(abs, nabs, capabs, got_rva + slot * 8);
                    slot++;
                }
                int64_t disp = (int64_t)slot_va + rel->addend - (int64_t)P;
                if (disp < (int64_t)INT32_MIN || disp > (int64_t)INT32_MAX) {
                    fprintf(stderr, "fakecc: GOT relocation out of range\n");
                    return -1;
                }
                int32_t d = (int32_t)disp;
                memcpy(L->text->data + poff, &d, 4);
            } else if (rel->type == R_PE_ADDR32NB) {
                uint32_t v = (uint32_t)((S + (int64_t)rel->addend) - IMAGE_BASE);
                memcpy(L->text->data + poff, &v, 4);
            } else if (rel->type == R_PE_SECREL || rel->type == R_X86_64_TPOFF32 || rel->type == R_X86_64_DTPOFF32) {
                uint64_t base = L->text_va;
                if (section_va_of(L, mods, nmods, mi, rel->sym, &base) != 0)
                    return -1;
                uint32_t v = (uint32_t)(S + (int64_t)rel->addend - base);
                memcpy(L->text->data + poff, &v, 4);
            } else if (rel->type == R_PE_SECTION) {
                uint16_t snum = 1;
                memcpy(L->text->data + poff, &snum, 2);
            } else {
                fprintf(stderr, "fakecc: relocation type %u is not supported "
                        "on the x86_64-windows target\n", rel->type);
                return -1;
            }
        }
        for (size_t r = 0; r < m->num_data_relocs; r++) {
            EmitReloc *rel = &m->data_relocs[r];
            if (rel->shndx == SECT_INIT_ARRAY || rel->shndx == SECT_FINI_ARRAY
                || rel->shndx == SECT_TLS_CALLBACKS)
                continue;
            size_t roff = rel->offset;
            uint16_t kind = rel->shndx;
            if (kind == SECT_DATA || kind == SECT_RODATA || kind == SECT_TDATA) {
                if (!remap_off(m, kind, rel->offset, &roff)) continue;
            }
            uint64_t S;
            int imp = 0;
            uint32_t iat_rva = 0;
            if (resolve_sym(L, mods, gs, ngs, mi, rel->sym, &S,
                            &imp, &iat_rva) != 0)
                return -1;
            size_t poff = 0;
            uint32_t rva = 0;
            char *place = NULL;
            int writable = 0;
            if (rel->shndx == SECT_DATA) {
                poff = L->mod_data[mi] + roff;
                place = L->data->data + poff;
                rva = (uint32_t)(L->data_va - IMAGE_BASE) + (uint32_t)poff;
                writable = 1;
            } else if (rel->shndx == SECT_RODATA) {
                poff = L->mod_ro[mi] + roff;
                place = L->rodata->data + poff;
                rva = (uint32_t)(L->rdata_va - IMAGE_BASE) + (uint32_t)poff;
            } else if (rel->shndx == SECT_TDATA) {
                poff = (L->mod_tdata ? L->mod_tdata[mi] : 0) + roff;
                place = L->tdata->data + poff;
                rva = (uint32_t)(L->tls_va - IMAGE_BASE) + (uint32_t)poff;
                writable = 1;
            } else {
                fprintf(stderr, "fakecc: data relocation in an unsupported "
                        "section\n");
                return -1;
            }
            if (rel->type == R_X86_64_64 && imp == 2) {
                if (!writable) {
                    fprintf(stderr, "fakecc: pointer to imported data is not "
                            "in a writable section\n");
                    return -1;
                }
                if (*ndyn >= dyn_cap) {
                    fprintf(stderr, "fakecc: internal: data-import fixup overflow\n");
                    return -1;
                }
                uint64_t zero = 0;
                memcpy(place, &zero, 8);
                uint64_t slot_va = IMAGE_BASE + rva;
                uint64_t iat_va = IMAGE_BASE + iat_rva;
                size_t at = dyn_at + *ndyn * 16;
                memcpy(L->rodata->data + at, &slot_va, 8);
                memcpy(L->rodata->data + at + 8, &iat_va, 8);
                add_abs(abs, nabs, capabs, (uint32_t)(L->rdata_va - IMAGE_BASE) + (uint32_t)at);
                add_abs(abs, nabs, capabs, (uint32_t)(L->rdata_va - IMAGE_BASE) + (uint32_t)at + 8);
                (*ndyn)++;
            } else if (rel->type == R_X86_64_64) {
                uint64_t val = S + (int64_t)rel->addend;
                memcpy(place, &val, 8);
                add_abs(abs, nabs, capabs, rva);
            } else if (rel->type == R_PE_ADDR32NB) {
                uint32_t v = (uint32_t)((S + (int64_t)rel->addend) - IMAGE_BASE);
                memcpy(place, &v, 4);
            } else if (rel->type == R_PE_SECREL || rel->type == R_X86_64_TPOFF32 || rel->type == R_X86_64_DTPOFF32) {
                uint64_t base = 0;
                if (section_va_of(L, mods, nmods, mi, rel->sym, &base) != 0)
                    return -1;
                uint32_t v = (uint32_t)(S + (int64_t)rel->addend - base);
                memcpy(place, &v, 4);
            } else if (rel->type == R_PE_SECTION) {
                uint16_t snum = 1;
                memcpy(place, &snum, 2);
            } else {
                fprintf(stderr, "fakecc: data relocation type %u is not "
                        "supported on the x86_64-windows target\n", rel->type);
                return -1;
            }
            (void)poff;
        }
        if (apply_sec_relocs(L, mods, nmods, gs, ngs, mi, m->pdata_relocs,
                             m->num_pdata_relocs, L->pdata, L->mod_pdata,
                             SECT_PDATA) != 0)
            return -1;
        if (apply_sec_relocs(L, mods, nmods, gs, ngs, mi, m->xdata_relocs,
                             m->num_xdata_relocs, L->xdata, L->mod_xdata,
                             SECT_XDATA) != 0)
            return -1;
    }
    return 0;
}

static int cmp_u32(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

static void build_reloc_section(Buffer *out, uint32_t *rva, size_t n) {
    buffer_init(out);
    if (!n) return;
    qsort(rva, n, sizeof(uint32_t), cmp_u32);
    size_t w = 0;
    for (size_t i = 0; i < n; i++) {
        if (w && rva[w - 1] == rva[i]) continue;
        rva[w++] = rva[i];
    }
    n = w;
    size_t i = 0;
    while (i < n) {
        uint32_t page = rva[i] & ~0xFFFu;
        size_t j = i;
        while (j < n && (rva[j] & ~0xFFFu) == page) j++;
        size_t count = j - i;
        size_t padded = count + (count & 1);
        bu32(out, page);
        bu32(out, (uint32_t)(8 + padded * 2));
        for (size_t k = i; k < j; k++) {
            uint16_t ent = (uint16_t)((10u << 12) | (rva[k] & 0xFFFu));
            bu16(out, ent);
        }
        if (count & 1) bu16(out, 0);
        i = j;
    }
}

static void put_section(Buffer *b, const char *name, uint32_t vsize, uint32_t va,
                        uint32_t rawsize, uint32_t rawptr, uint32_t chars) {
    char nm[8];
    memset(nm, 0, 8);
    size_t n = strlen(name);
    if (n > 8) n = 8;
    memcpy(nm, name, n);
    buffer_append(b, nm, 8);
    bu32(b, vsize);
    bu32(b, va);
    bu32(b, rawsize);
    bu32(b, rawptr);
    bu32(b, 0);
    bu32(b, 0);
    bu16(b, 0);
    bu16(b, 0);
    bu32(b, chars);
}

/* Long PE section names (/offset into the COFF string table). */
static void put_section_long(Buffer *b, Buffer *strtab, const char *name,
                             uint32_t vsize, uint32_t va, uint32_t rawsize,
                             uint32_t rawptr, uint32_t chars) {
    char nm[8];
    memset(nm, 0, 8);
    size_t n = strlen(name);
    if (n <= 8) {
        memcpy(nm, name, n);
    } else {
        if (strtab->len < 4) {
            buffer_init(strtab);
            bu32(strtab, 4);
        }
        uint32_t off = (uint32_t)strtab->len;
        buffer_append(strtab, name, n + 1);
        snprintf(nm, sizeof nm, "/%u", off);
    }
    buffer_append(b, nm, 8);
    bu32(b, vsize);
    bu32(b, va);
    bu32(b, rawsize);
    bu32(b, rawptr);
    bu32(b, 0);
    bu32(b, 0);
    bu16(b, 0);
    bu16(b, 0);
    bu32(b, chars);
}

static int pe_module_has_debug(const EmitModule *em) {
    return em && (em->num_dbg_lines > 0 || em->num_dbg_funcs > 0
                  || em->num_dbg_globals > 0 || em->dbg_tu_name != NULL);
}

static int name_defined(EmitModule **mods, size_t n, const char *name) {
    for (size_t i = 0; i < n; i++) {
        for (size_t s = 0; s < mods[i]->num_syms; s++) {
            EmitSymbol *sy = &mods[i]->syms[s];
            if (sy->name && sy->binding != 0 && sym_defined_here(sy)
                && strcmp(sy->name, name) == 0)
                return 1;
        }
    }
    return 0;
}

/* Patch init/fini pointer relocs into the concatenated arrays and record
 * base relocations.  `mod_off[mi]` is the byte offset of that module's
 * array inside `arr` before sorting; sorting rewrites both the bytes and
 * a parallel reloc list we apply after the sort by matching the original
 * slot contents...  Simpler: patch BEFORE sorting, using the pre-sort
 * offset, then sort the already-filled pointer values together with an
 * explicit reloc-rva array that moves with the slots.
 *
 * A reloc's target address moves when the slot moves, so the RVA recorded
 * for the base relocation has to move too.  Do it in one structure. */
typedef struct { uint64_t val; int prio; uint32_t rva_slot; } Hook;

static int fill_hooks(EmitModule **mods, size_t nmods, int is_fini,
                      GSym *gs, size_t ngs, const Lay *L,
                      Hook **out, size_t *nout, uint32_t array_rva) {
    size_t cap = 0, n = 0;
    Hook *h = NULL;
    for (size_t mi = 0; mi < nmods; mi++) {
        EmitModule *m = mods[mi];
        for (size_t r = 0; r < m->num_data_relocs; r++) {
            EmitReloc *rel = &m->data_relocs[r];
            uint16_t want = is_fini ? SECT_FINI_ARRAY : SECT_INIT_ARRAY;
            if (rel->shndx != want) continue;
            size_t ignored;
            if (!remap_off(m, rel->shndx, rel->offset, &ignored)) continue;
            uint64_t S;
            if (resolve_sym(L, mods, gs, ngs, mi, rel->sym, &S, NULL, NULL) != 0) {
                free(h);
                return -1;
            }
            if (n == cap) {
                cap = cap ? cap * 2 : 8;
                h = xrealloc(h, cap * sizeof(Hook));
            }
            size_t slot = rel->offset / 8;
            int *prio = is_fini ? m->fini_prio : m->init_prio;
            h[n].val = S + (int64_t)rel->addend;
            h[n].prio = (prio && slot < (is_fini ? m->fini_array.len
                                                : m->init_array.len) / 8)
                        ? prio[slot] : INIT_PRIO_DEFAULT;
            h[n].rva_slot = 0;
            n++;
        }
    }
    for (size_t i = 1; i < n; i++) {
        Hook key = h[i];
        size_t j = i;
        while (j > 0 && h[j - 1].prio > key.prio) {
            h[j] = h[j - 1];
            j--;
        }
        h[j] = key;
    }
    for (size_t i = 0; i < n; i++)
        h[i].rva_slot = array_rva + (uint32_t)(i * 8);
    *out = h;
    *nout = n;
    return 0;
}

/* ------------------------------------------------------------------ */
/* COMDAT selection, imports, DLL entry, unwind, pe_link_ex           */
/* ------------------------------------------------------------------ */

static int cmp_pdata(const void *a, const void *b) {
    uint32_t xa, xb;
    memcpy(&xa, a, 4);
    memcpy(&xb, b, 4);
    if (xa < xb) return -1;
    if (xa > xb) return 1;
    return 0;
}

static const char *path_base(const char *p) {
    const char *b = p ? p : "";
    for (const char *s = b; *s; s++)
        if (*s == '/' || *s == '\\') b = s + 1;
    return b;
}

static int exportable_name(const char *name) {
    if (!name || !name[0] || name[0] == '.') return 0;
    if (strncmp(name, "__fakecc_", 9) == 0) return 0;
    if (strncmp(name, "__imp_", 6) == 0) return 0;
    return 1;
}

static int sym_in_comdat_span(const EmitModule *m, const EmitSymbol *s) {
    if (!m->num_spans || !s->name) return 0;
    if (s->shndx == SECT_UNDEF || s->shndx == SHN_COMMON) return 0;
    for (size_t i = 0; i < m->num_spans; i++) {
        const EmitSpan *sp = &m->spans[i];
        if (sp->comdat < 0 || sp->kind != s->shndx) continue;
        if (s->value >= sp->off && s->value < sp->off + sp->len) return 1;
    }
    return 0;
}

static int defined_outside_comdat(EmitModule **mods, size_t nmods, const char *key) {
    for (size_t mi = 0; mi < nmods; mi++) {
        EmitModule *m = mods[mi];
        for (size_t s = 0; s < m->num_syms; s++) {
            EmitSymbol *sy = &m->syms[s];
            if (!sy->name || sy->binding == 0) continue;
            if (sy->shndx == SECT_UNDEF || sy->shndx == SHN_COMMON) continue;
            if (strcmp(sy->name, key) != 0) continue;
            if (!sym_in_comdat_span(m, sy)) return 1;
        }
    }
    return 0;
}

static const EmitSpan *span_for_comdat(const EmitModule *m, int ci) {
    for (size_t i = 0; i < m->num_spans; i++)
        if (m->spans[i].comdat == ci) return &m->spans[i];
    return NULL;
}

static const char *span_src(const EmitModule *m, const EmitSpan *sp) {
    if (!sp) return NULL;
    const Buffer *b = NULL;
    switch (sp->kind) {
    case SECT_TEXT: b = &m->text; break;
    case SECT_RODATA: b = &m->rodata; break;
    case SECT_DATA: b = &m->data; break;
    case SECT_PDATA: b = &m->pdata; break;
    case SECT_XDATA: b = &m->xdata; break;
    default: return NULL;
    }
    if (!b->data || sp->off + sp->len > b->len) return NULL;
    return b->data + sp->off;
}

static int comdat_bytes_eq(const EmitModule *a, int ca, const EmitModule *b, int cb) {
    const EmitSpan *sa = span_for_comdat(a, ca);
    const EmitSpan *sb = span_for_comdat(b, cb);
    if (!sa || !sb) return sa == sb;
    if (sa->len != sb->len) return 0;
    const char *pa = span_src(a, sa);
    const char *pb = span_src(b, sb);
    if (!pa || !pb) return pa == pb;
    return memcmp(pa, pb, sa->len) == 0;
}

static size_t comdat_span_len(const EmitModule *m, int ci) {
    const EmitSpan *sp = span_for_comdat(m, ci);
    return sp ? sp->len : 0;
}

static int comdat_select(EmitModule **mods, size_t nmods) {
    typedef struct { size_t mi; int ci; } CdRef;
    size_t nref = 0;
    for (size_t mi = 0; mi < nmods; mi++) {
        EmitModule *m = mods[mi];
        for (size_t c = 0; c < m->num_comdats; c++) {
            m->comdats[c].keep = 1;
            if (m->comdats[c].selection != IMAGE_COMDAT_SELECT_ASSOCIATIVE)
                nref++;
        }
    }
    CdRef *refs = nref ? xmalloc(nref * sizeof(CdRef)) : NULL;
    uint8_t *seen = nref ? zalloc(nref) : NULL;
    size_t nr = 0;
    for (size_t mi = 0; mi < nmods; mi++) {
        EmitModule *m = mods[mi];
        for (size_t c = 0; c < m->num_comdats; c++) {
            if (m->comdats[c].selection == IMAGE_COMDAT_SELECT_ASSOCIATIVE)
                continue;
            refs[nr].mi = mi;
            refs[nr].ci = (int)c;
            nr++;
        }
    }
    for (size_t i = 0; i < nr; i++) {
        if (seen[i]) continue;
        EmitComdat *c0 = &mods[refs[i].mi]->comdats[refs[i].ci];
        const char *key = (c0->symbol && c0->symbol[0]) ? c0->symbol : c0->sect_name;
        if (!key) { seen[i] = 1; continue; }
        size_t *idx = NULL, ng = 0, cg = 0;
        for (size_t j = i; j < nr; j++) {
            if (seen[j]) continue;
            EmitComdat *cj = &mods[refs[j].mi]->comdats[refs[j].ci];
            const char *kj = (cj->symbol && cj->symbol[0]) ? cj->symbol : cj->sect_name;
            if (!kj || strcmp(kj, key) != 0) continue;
            seen[j] = 1;
            if (ng == cg) {
                cg = cg ? cg * 2 : 4;
                idx = xrealloc(idx, cg * sizeof(size_t));
            }
            idx[ng++] = j;
        }
        if (defined_outside_comdat(mods, nmods, key)) {
            for (size_t g = 0; g < ng; g++)
                mods[refs[idx[g]].mi]->comdats[refs[idx[g]].ci].keep = 0;
            free(idx);
            continue;
        }
        int nodup = 0, same = 0, exact = 0, large = 0;
        for (size_t g = 0; g < ng; g++) {
            uint8_t sel = mods[refs[idx[g]].mi]->comdats[refs[idx[g]].ci].selection;
            if (sel == IMAGE_COMDAT_SELECT_NODUPLICATES) nodup = 1;
            else if (sel == IMAGE_COMDAT_SELECT_SAME_SIZE) same = 1;
            else if (sel == IMAGE_COMDAT_SELECT_EXACT_MATCH) exact = 1;
            else if (sel == IMAGE_COMDAT_SELECT_LARGEST) large = 1;
        }
        if (ng > 1 && nodup) {
            fprintf(stderr, "fakecc: duplicate COMDAT '%s'\n", key);
            free(idx); free(seen); free(refs);
            return -1;
        }
        if ((same || exact) && ng > 1) {
            size_t sz0 = comdat_span_len(mods[refs[idx[0]].mi], refs[idx[0]].ci);
            for (size_t g = 1; g < ng; g++) {
                size_t sz = comdat_span_len(mods[refs[idx[g]].mi], refs[idx[g]].ci);
                if (sz != sz0) {
                    fprintf(stderr, "fakecc: COMDAT '%s' sizes differ\n", key);
                    free(idx); free(seen); free(refs);
                    return -1;
                }
                if (exact && !comdat_bytes_eq(mods[refs[idx[0]].mi], refs[idx[0]].ci,
                                              mods[refs[idx[g]].mi], refs[idx[g]].ci)) {
                    fprintf(stderr, "fakecc: COMDAT '%s' contents differ\n", key);
                    free(idx); free(seen); free(refs);
                    return -1;
                }
            }
        }
        size_t win = 0;
        if (large) {
            size_t best = 0;
            for (size_t g = 0; g < ng; g++) {
                size_t sz = comdat_span_len(mods[refs[idx[g]].mi], refs[idx[g]].ci);
                if (g == 0 || sz > best) { best = sz; win = g; }
            }
        }
        for (size_t g = 0; g < ng; g++)
            if (g != win)
                mods[refs[idx[g]].mi]->comdats[refs[idx[g]].ci].keep = 0;
        free(idx);
    }
    int guard = 0;
    int changed = 1;
    while (changed && guard++ < 64) {
        changed = 0;
        for (size_t mi = 0; mi < nmods; mi++) {
            EmitModule *m = mods[mi];
            for (size_t c = 0; c < m->num_comdats; c++) {
                EmitComdat *cd = &m->comdats[c];
                if (cd->selection != IMAGE_COMDAT_SELECT_ASSOCIATIVE || !cd->keep)
                    continue;
                if (!cd->assoc_name) continue;
                for (size_t p = 0; p < m->num_comdats; p++) {
                    if (p == c) continue;
                    EmitComdat *par = &m->comdats[p];
                    if (!par->sect_name || strcmp(par->sect_name, cd->assoc_name) != 0)
                        continue;
                    if (!par->keep) { cd->keep = 0; changed = 1; }
                    break;
                }
            }
        }
    }
    free(seen);
    free(refs);
    return 0;
}

static void append_kept(Buffer *dst, const EmitModule *m, uint16_t kind,
                        const char *src, size_t src_len) {
    if (!m->num_spans) {
        if (src && src_len) buffer_append(dst, src, src_len);
        return;
    }
    int any = 0;
    for (size_t i = 0; i < m->num_spans; i++) {
        const EmitSpan *sp = &m->spans[i];
        if (sp->kind != kind) continue;
        any = 1;
        if (!comdat_kept(m, sp->comdat)) continue;
        if (!src || sp->off + sp->len > src_len) {
            if (sp->len) bpad(dst, sp->len);
        } else if (sp->len) {
            buffer_append(dst, src + sp->off, sp->len);
        }
    }
    if (!any && src && src_len)
        buffer_append(dst, src, src_len);
}

static size_t kept_size(const EmitModule *m, uint16_t kind, size_t fallback) {
    if (!m->num_spans) return fallback;
    int any = 0;
    size_t n = 0;
    for (size_t i = 0; i < m->num_spans; i++) {
        const EmitSpan *sp = &m->spans[i];
        if (sp->kind != kind) continue;
        any = 1;
        if (comdat_kept(m, sp->comdat)) n += sp->len;
    }
    return any ? n : fallback;
}

static int sym_is_undef(EmitModule **mods, size_t n, const char *name) {
    for (size_t i = 0; i < n; i++) {
        for (size_t s = 0; s < mods[i]->num_syms; s++) {
            EmitSymbol *sy = &mods[i]->syms[s];
            if (sy->name && sy->shndx == SECT_UNDEF && strcmp(sy->name, name) == 0)
                return 1;
        }
    }
    return 0;
}

static const char *reloc_sym_name(const EmitModule *m, uint32_t si) {
    if (si >= m->num_syms) return NULL;
    return m->syms[si].name;
}

#define UW_PUSH_NONVOL 0
#define UW_ALLOC_LARGE 1
#define UW_ALLOC_SMALL 2
#define UW_SET_FPREG   3

/* Append one UNWIND_INFO and return its offset in `xdata`, or (uint32_t)-1. */
static uint32_t append_unwind_info(Buffer *xdata, const EmitWinUnwind *u) {
    if (!u || u->end <= u->begin) return (uint32_t)-1;
    uint32_t xoff = (uint32_t)xdata->len;
    uint16_t codes[32];
    int nc = 0;
    uint8_t prolog = (uint8_t)(u->after_prolog > u->begin
                               ? u->after_prolog - u->begin : 0);
    if (u->stack_alloc > 0 && nc < 30) {
        uint8_t off = prolog;
        if (u->stack_alloc <= 128 && (u->stack_alloc % 8) == 0) {
            uint8_t info = (uint8_t)(u->stack_alloc / 8 - 1);
            codes[nc++] = (uint16_t)(off | ((UW_ALLOC_SMALL | (info << 4)) << 8));
        } else if (u->stack_alloc <= 512 * 1024) {
            codes[nc++] = (uint16_t)((u->stack_alloc / 8) & 0xffff);
            codes[nc++] = (uint16_t)(off | (UW_ALLOC_LARGE << 8));
        } else {
            codes[nc++] = (uint16_t)((u->stack_alloc >> 16) & 0xffff);
            codes[nc++] = (uint16_t)(u->stack_alloc & 0xffff);
            codes[nc++] = (uint16_t)(off | ((UW_ALLOC_LARGE | (1 << 4)) << 8));
        }
    }
    for (int p = 7; p >= 0 && nc < 31; p--) {
        if (u->pushed[p] < 0) continue;
        uint8_t off = (uint8_t)(u->after_set_fp > u->begin
                                ? (u->after_set_fp - u->begin) + (size_t)(p + 1) * 8
                                : 0);
        if (u->after_prolog > u->begin && off > prolog) off = prolog;
        codes[nc++] = (uint16_t)(off
            | ((UW_PUSH_NONVOL | ((uint8_t)u->pushed[p] << 4)) << 8));
    }
    if (nc < 32) {
        uint8_t off = (uint8_t)(u->after_set_fp > u->begin
                                ? u->after_set_fp - u->begin : 0);
        codes[nc++] = (uint16_t)(off | (UW_SET_FPREG << 8));
    }
    if (nc < 32) {
        uint8_t off = (uint8_t)(u->after_push_rbp > u->begin
                                ? u->after_push_rbp - u->begin : 1);
        codes[nc++] = (uint16_t)(off | ((UW_PUSH_NONVOL | (5 << 4)) << 8));
    }
    int ncodes = nc;
    if (ncodes & 1) codes[ncodes++] = 0;
    bu8(xdata, 1);
    bu8(xdata, prolog);
    bu8(xdata, (uint8_t)nc);
    bu8(xdata, 5); /* FrameRegister = RBP */
    for (int c = 0; c < ncodes; c++) bu16(xdata, codes[c]);
    return xoff;
}

static void emit_import_thunk(Asm *a, uint32_t rdata_off) {
    ab(a, 0xFF); ab(a, 0x25);
    add_fix(a, FK_RDATA, (int)rdata_off, NULL);
    ab(a, 0x90); ab(a, 0x90);
}

typedef struct {
    const char *name;
    uint32_t rva;
    int is_data;
} ExpSym;

typedef struct {
    size_t mi;
    size_t begin;
    size_t len;
    uint32_t xoff;
} SynUw;

static int slot_is_data_imp(const ImpSlot *slots, size_t nslots, const char *name,
                            EmitModule **mods, size_t nmods) {
    if (!name) return 0;
    if (name_defined(mods, nmods, name)) return 0;
    for (size_t i = 0; i < nslots; i++) {
        if (!slots[i].publish || !slots[i].is_data || !slots[i].name) continue;
        if (strcmp(slots[i].name, name) == 0) return 1;
    }
    return 0;
}

static int add_slot(ImpSlot **slots, size_t *n, size_t *cap, const char *dll,
                    const char *name, int is_data, int ordinal, int hint,
                    int fixed, int publish) {
    for (size_t i = 0; i < *n; i++) {
        if (icmp_dll((*slots)[i].dll, dll) != 0) continue;
        if (strcmp((*slots)[i].name, name) != 0) continue;
        if (publish) (*slots)[i].publish = 1;
        if (is_data) (*slots)[i].is_data = 1;
        return 0;
    }
    if (*n == *cap) {
        *cap = *cap ? *cap * 2 : 8;
        *slots = xrealloc(*slots, *cap * sizeof(ImpSlot));
    }
    ImpSlot *s = &(*slots)[(*n)++];
    memset(s, 0, sizeof *s);
    s->dll = dll;
    s->name = name;
    s->is_data = is_data;
    s->ordinal = ordinal;
    s->hint = hint;
    s->fixed = fixed;
    s->publish = publish;
    s->thunk_off = (size_t)-1;
    return 0;
}

int pe_link_ex(EmitModule **mods, size_t nmods, const char *path,
               const PeLinkOpt *opt) {
    int shared = opt && opt->shared;
    const PeImport *uimp = opt ? opt->imports : NULL;
    size_t nuimp = opt ? opt->nimports : 0;
    const char *implib_path = opt ? opt->implib_path : NULL;
    if (!nmods || !path) {
        fprintf(stderr, "fakecc: no modules to link\n");
        return -1;
    }
    if (comdat_select(mods, nmods) != 0) return -1;

    Buffer text, rodata, data, pdata, xdata, reloc, tdata;
    buffer_init(&text); buffer_init(&rodata); buffer_init(&data);
    buffer_init(&pdata); buffer_init(&xdata); buffer_init(&reloc);
    buffer_init(&tdata);
    size_t *mod_text = zalloc(nmods * sizeof(size_t));
    size_t *mod_ro = zalloc(nmods * sizeof(size_t));
    size_t *mod_data = zalloc(nmods * sizeof(size_t));
    size_t *mod_bss = zalloc(nmods * sizeof(size_t));
    size_t *mod_pdata = zalloc(nmods * sizeof(size_t));
    size_t *mod_xdata = zalloc(nmods * sizeof(size_t));
    size_t *mod_tdata = zalloc(nmods * sizeof(size_t));
    size_t *mod_tbss = zalloc(nmods * sizeof(size_t));
    Imports im;
    memset(&im, 0, sizeof im);
    buffer_init(&im.blob);
    Asm stub;
    memset(&stub, 0, sizeof stub);
    buffer_init(&stub.buf);
    ImpSlot *slots = NULL;
    size_t nslots = 0, capslots = 0;
    char **imp_owned = NULL;
    size_t nimp_owned = 0;
    SynUw *syn = NULL;
    size_t nsyn = 0, capsyn = 0;
    ExpSym *exps = NULL;
    size_t nexp = 0, capexp = 0;
    GSym *gs = NULL;
    size_t ngs = 0, capgs = 0;
    uint32_t *abs = NULL;
    size_t nabs = 0, capabs = 0;
    Hook *ih = NULL, *fh = NULL;
    typedef struct { const char *name; size_t size; size_t align; size_t off; } Common;
    Common *commons = NULL;
    size_t ncommon = 0, capcommon = 0;
    int rc = 0;
    size_t bss = 0;
    size_t tbss = 0;
    size_t max_tls_align = 16;

    for (size_t i = 0; i < nmods; i++) {
        EmitModule *m = mods[i];
        size_t ta = m->text_align > 16 ? m->text_align : 16;
        while (text.len % ta) bu8(&text, 0x90);
        mod_text[i] = text.len;
        append_kept(&text, m, SECT_TEXT, m->text.data, m->text.len);

        size_t ra = m->rodata_align ? m->rodata_align : 8;
        while (rodata.len % ra) bu8(&rodata, 0);
        mod_ro[i] = rodata.len;
        append_kept(&rodata, m, SECT_RODATA, m->rodata.data, m->rodata.len);

        size_t da = m->data_align ? m->data_align : 8;
        while (data.len % da) bu8(&data, 0);
        mod_data[i] = data.len;
        append_kept(&data, m, SECT_DATA, m->data.data, m->data.len);

        size_t ba = m->bss_align ? m->bss_align : 8;
        while (bss % ba) bss++;
        mod_bss[i] = bss;
        bss += kept_size(m, SECT_BSS, m->bss_size);

        size_t tda = m->tdata_align ? m->tdata_align : 8;
        if (tda > max_tls_align) max_tls_align = tda;
        while (tdata.len % tda) bu8(&tdata, 0);
        mod_tdata[i] = tdata.len;
        append_kept(&tdata, m, SECT_TDATA, m->tdata.data, m->tdata.len);

        size_t tba = m->tbss_align ? m->tbss_align : 8;
        if (tba > max_tls_align) max_tls_align = tba;
        while (tbss % tba) tbss++;
        mod_tbss[i] = tbss;
        tbss += kept_size(m, SECT_TBSS, m->tbss_size);

        while (pdata.len % 4) bu8(&pdata, 0);
        mod_pdata[i] = pdata.len;
        append_kept(&pdata, m, SECT_PDATA, m->pdata.data, m->pdata.len);

        while (xdata.len % 4) bu8(&xdata, 0);
        mod_xdata[i] = xdata.len;
        append_kept(&xdata, m, SECT_XDATA, m->xdata.data, m->xdata.len);

        for (size_t u = 0; u < m->num_win_unwinds; u++) {
            const EmitWinUnwind *uw = &m->win_unwinds[u];
            if (uw->end <= uw->begin) continue;
            size_t begin_off = 0;
            if (!remap_off(m, SECT_TEXT, uw->begin, &begin_off)) continue;
            uint32_t xoff = append_unwind_info(&xdata, uw);
            if (xoff == (uint32_t)-1) continue;
            if (nsyn == capsyn) {
                capsyn = capsyn ? capsyn * 2 : 8;
                syn = xrealloc(syn, capsyn * sizeof(SynUw));
            }
            syn[nsyn].mi = i;
            syn[nsyn].begin = begin_off;
            syn[nsyn].len = uw->end - uw->begin;
            syn[nsyn].xoff = xoff;
            nsyn++;
        }
    }
    size_t syn_at = pdata.len;
    bpad(&pdata, nsyn * 12);

    for (size_t i = 0; i < nmods; i++) {
        EmitModule *m = mods[i];
        for (size_t s = 0; s < m->num_syms; s++) {
            EmitSymbol *sy = &m->syms[s];
            if (sy->shndx != SHN_COMMON || !sy->name) continue;
            size_t sz = sy->size ? sy->size : 1;
            size_t al = sy->value ? sy->value : 8;
            int found = -1;
            for (size_t c = 0; c < ncommon; c++)
                if (strcmp(commons[c].name, sy->name) == 0) { found = (int)c; break; }
            if (found >= 0) {
                if (sz > commons[found].size) commons[found].size = sz;
                if (al > commons[found].align) commons[found].align = al;
            } else {
                if (ncommon == capcommon) {
                    capcommon = capcommon ? capcommon * 2 : 8;
                    commons = xrealloc(commons, capcommon * sizeof(Common));
                }
                commons[ncommon].name = sy->name;
                commons[ncommon].size = sz;
                commons[ncommon].align = al;
                commons[ncommon].off = 0;
                ncommon++;
            }
        }
    }
    for (size_t c = 0; c < ncommon; c++) {
        while (bss % commons[c].align) bss++;
        commons[c].off = bss;
        bss += commons[c].size;
    }

    size_t max_tbss_align = 1;
    for (size_t i = 0; i < nmods; i++) {
        size_t tba = mods[i]->tbss_align ? mods[i]->tbss_align : 1;
        if (tba > max_tbss_align) max_tbss_align = tba;
    }
    while (tdata.len % max_tbss_align) bu8(&tdata, 0);
    size_t tbss_off = tdata.len;
    bpad(&tdata, tbss);
    size_t tls_memsz = tdata.len;
    int have_tls = (tls_memsz > 0);

    int want_tramp = 1;
    if (want_tramp) {
        while (data.len % 16) bu8(&data, 0);
    }
    size_t synth_off = data.len;
    if (want_tramp) bpad(&data, SYN_SIZE);
    size_t tls_index_off = 0;
    if (have_tls) {
        while (data.len % 8) bu8(&data, 0);
        tls_index_off = data.len;
        bu32(&data, 0);
    }
    /* .bss is the tail of .data.  The TLS index above is 4 bytes, so the
     * file-backed image can end 4-mod-8 and every zero-init global lands
     * off its alignment.  Pad to the strictest alignment used by .bss
     * (modules and commons) before the virtual addresses are assigned. */
    if (bss) {
        size_t max_bss_align = 8;
        for (size_t i = 0; i < nmods; i++) {
            size_t ba = mods[i]->bss_align ? mods[i]->bss_align : 8;
            if (ba > max_bss_align) max_bss_align = ba;
        }
        for (size_t c = 0; c < ncommon; c++) {
            size_t al = commons[c].align ? commons[c].align : 8;
            if (al > max_bss_align) max_bss_align = al;
        }
        while (data.len % max_bss_align) bu8(&data, 0);
    }
    if (!data.len && !bss) bu8(&data, 0);

    if (want_tramp) {
        for (int i = 0; i < IMP_COUNT; i++)
            add_slot(&slots, &nslots, &capslots, "KERNEL32.dll", IMP_NAMES[i],
                     0, 0, 0, i,
                     sym_is_undef(mods, nmods, IMP_NAMES[i]));
    }
    for (size_t i = 0; i < nuimp; i++) {
        if (!uimp[i].dll || !uimp[i].name) continue;
        add_slot(&slots, &nslots, &capslots, uimp[i].dll, uimp[i].name,
                 uimp[i].is_data, uimp[i].ordinal, uimp[i].hint, -1, 1);
    }

    size_t got_n = 0, dyn_n = 0;
    for (size_t i = 0; i < nmods; i++) {
        EmitModule *m = mods[i];
        for (size_t r = 0; r < m->num_relocs; r++) {
            EmitReloc *rel = &m->relocs[r];
            size_t dummy;
            if (!remap_off(m, SECT_TEXT, rel->offset, &dummy)) continue;
            if (rel->type != R_X86_64_GOTPCREL) continue;
            if (!slot_is_data_imp(slots, nslots, reloc_sym_name(m, rel->sym),
                                  mods, nmods))
                got_n++;
        }
        for (size_t r = 0; r < m->num_data_relocs; r++) {
            EmitReloc *rel = &m->data_relocs[r];
            if (rel->shndx != SECT_DATA || rel->type != R_X86_64_64) continue;
            size_t dummy;
            if (!remap_off(m, SECT_DATA, rel->offset, &dummy)) continue;
            if (slot_is_data_imp(slots, nslots, reloc_sym_name(m, rel->sym),
                                 mods, nmods))
                dyn_n++;
        }
    }

    int have_exit = name_defined(mods, nmods, "exit");
    int have_env = name_defined(mods, nmods, "__fakecc_win_env");
    int have_main = name_defined(mods, nmods, "main");
    int have_dllmain = name_defined(mods, nmods, "DllMain");
    if (!shared && !have_main) {
        fprintf(stderr, "fakecc: no 'main' function found\n");
        rc = -1;
        goto done;
    }

    const char *dll_name = path_base(path);
    if (!dll_name[0]) dll_name = "fakecc.dll";
    size_t exp_reserve = 0;
    if (shared) {
        size_t nexp_hi = 0;
        size_t bytes = 40 + strlen(dll_name) + 1;
        for (size_t mi = 0; mi < nmods; mi++) {
            EmitModule *m = mods[mi];
            for (size_t s = 0; s < m->num_syms; s++) {
                EmitSymbol *sy = &m->syms[s];
                if (!sy->name || sy->binding == 0 || sy->binding == 2) continue;
                if (!sym_defined_here(sy) || !exportable_name(sy->name)) continue;
                size_t dummy;
                if (!remap_off(m, sy->shndx, sy->value, &dummy)) continue;
                nexp_hi++;
                bytes += 4 + 4 + 2 + strlen(sy->name) + 1;
            }
        }
        if (nexp_hi) {
            while (bytes & 7) bytes++;
            exp_reserve = bytes;
        }
    }

    while (rodata.len % 8) bu8(&rodata, 0);
    size_t got_at = rodata.len;
    bpad(&rodata, got_n * 8);
    size_t imp_at = rodata.len;
    Imports probe;
    memset(&probe, 0, sizeof probe);
    if (build_imports(&probe, slots, nslots, 0, 0) != 0) {
        rc = -1;
        goto done;
    }
    size_t imp_size = probe.blob.len;
    buffer_free(&probe.blob);
    bpad(&rodata, imp_size);
    while (rodata.len % 8) bu8(&rodata, 0);
    size_t dyn_at = rodata.len;
    bpad(&rodata, dyn_n * 16);
    while (rodata.len % 8) bu8(&rodata, 0);
    size_t init_n = 0, fini_n = 0, tls_cb_n = 0;
    for (size_t i = 0; i < nmods; i++) {
        for (size_t r = 0; r < mods[i]->num_data_relocs; r++) {
            EmitReloc *rel = &mods[i]->data_relocs[r];
            size_t dummy;
            if (rel->shndx == SECT_INIT_ARRAY
                && remap_off(mods[i], rel->shndx, rel->offset, &dummy))
                init_n++;
            if (rel->shndx == SECT_FINI_ARRAY
                && remap_off(mods[i], rel->shndx, rel->offset, &dummy))
                fini_n++;
            if (rel->shndx == SECT_TLS_CALLBACKS
                && remap_off(mods[i], rel->shndx, rel->offset, &dummy))
                tls_cb_n++;
        }
    }
    size_t init_at = rodata.len;
    bpad(&rodata, init_n * 8);
    size_t fini_at = rodata.len;
    bpad(&rodata, fini_n * 8);
    while (rodata.len % 4) bu8(&rodata, 0);
    size_t exp_at = rodata.len;
    bpad(&rodata, exp_reserve);
    size_t tls_cb_at = 0, tls_dir_at = 0;
    if (have_tls || tls_cb_n > 0) {
        have_tls = 1;
        while (rodata.len % 8) bu8(&rodata, 0);
        tls_cb_at = rodata.len;
        bpad(&rodata, (tls_cb_n + 1) * 8); /* null-terminated callbacks array */
        while (rodata.len % 8) bu8(&rodata, 0);
        tls_dir_at = rodata.len;
        bpad(&rodata, 40); /* IMAGE_TLS_DIRECTORY64 */
    }
    if (!rodata.len) bu8(&rodata, 0);

    int need_dll = shared && (have_dllmain || init_n || fini_n || dyn_n || have_tls);
    while (text.len % 16) bu8(&text, 0x90);
    size_t stub_at = text.len;
    size_t entry_at = (size_t)-1;
    if (want_tramp) {
        size_t *fail_slots = xmalloc(256 * sizeof(size_t));
        int nfail = 0;
        emit_trampoline(&stub, fail_slots, &nfail);
        free(fail_slots);
    }
    for (size_t i = 0; i < nslots; i++) {
        if (!slots[i].publish || slots[i].is_data) continue;
        if (name_defined(mods, nmods, slots[i].name)) continue;
        slots[i].thunk_off = stub.buf.len;
        emit_import_thunk(&stub, (uint32_t)(imp_at + slots[i].iat_boff));
    }
    uint32_t fini_lea = (uint32_t)(fini_n ? fini_at + (fini_n - 1) * 8 : fini_at);
    if (!shared) {
        entry_at = stub.buf.len;
        emit_winmain(&stub, have_exit, have_env,
                     (uint32_t)init_at, (uint32_t)init_n,
                     fini_lea, (uint32_t)fini_n,
                     (uint32_t)dyn_at, (uint32_t)dyn_n);
    } else if (need_dll) {
        entry_at = stub.buf.len;
        emit_dllentry(&stub, have_dllmain,
                      (uint32_t)init_at, (uint32_t)init_n,
                      fini_lea, (uint32_t)fini_n,
                      (uint32_t)dyn_at, (uint32_t)dyn_n);
    }
    if (stub.buf.len)
        buffer_append(&text, stub.buf.data, stub.buf.len);

    uint32_t text_rva = SECT_ALIGN;
    uint32_t rdata_rva = align_up_u32(text_rva + (uint32_t)text.len, SECT_ALIGN);
    uint32_t data_rva = align_up_u32(rdata_rva + (uint32_t)rodata.len, SECT_ALIGN);
    uint32_t data_vsize = (uint32_t)data.len + (uint32_t)bss;
    uint32_t cursor = align_up_u32(data_rva + data_vsize, SECT_ALIGN);
    uint32_t pdata_rva = 0, xdata_rva = 0;
    if (pdata.len) {
        pdata_rva = cursor;
        cursor = align_up_u32(pdata_rva + (uint32_t)pdata.len, SECT_ALIGN);
    }
    if (xdata.len) {
        xdata_rva = cursor;
        cursor = align_up_u32(xdata_rva + (uint32_t)xdata.len, SECT_ALIGN);
    }
    uint32_t tls_rva = 0, tls_vsize = 0;
    if (have_tls) {
        tls_rva = cursor;
        tls_vsize = (uint32_t)tls_memsz;
        cursor = align_up_u32(tls_rva + tls_vsize, SECT_ALIGN);
    }

    if (nslots) {
        if (build_imports(&im, slots, nslots, rdata_rva, (uint32_t)imp_at) != 0) {
            rc = -1;
            goto done;
        }
        if (im.blob.len != imp_size) {
            fprintf(stderr, "fakecc: internal: import blob size changed\n");
            rc = -1;
            goto done;
        }
        if (imp_size)
            memcpy(rodata.data + imp_at, im.blob.data, im.blob.len);
    }

    Lay L;
    memset(&L, 0, sizeof L);
    L.text_va = IMAGE_BASE + text_rva;
    L.rdata_va = IMAGE_BASE + rdata_rva;
    L.data_va = IMAGE_BASE + data_rva;
    L.pdata_va = IMAGE_BASE + pdata_rva;
    L.xdata_va = IMAGE_BASE + xdata_rva;
    L.tls_va = have_tls ? (IMAGE_BASE + tls_rva) : 0;
    L.synth_off = synth_off;
    L.mod_text = mod_text;
    L.mod_ro = mod_ro;
    L.mod_data = mod_data;
    L.mod_bss = mod_bss;
    L.mod_pdata = mod_pdata;
    L.mod_xdata = mod_xdata;
    L.mod_tdata = mod_tdata;
    L.mod_tbss = mod_tbss;
    L.tbss_off = tbss_off;
    L.data_file = data.len;
    L.text = &text;
    L.rodata = &rodata;
    L.data = &data;
    L.pdata = &pdata;
    L.xdata = &xdata;
    L.tdata = &tdata;
    L.init_at = init_at;
    L.fini_at = fini_at;

    for (int pass = 0; pass < 2 && rc == 0; pass++) {
        int want_strong = pass == 0;
        for (size_t mi = 0; mi < nmods && rc == 0; mi++) {
            EmitModule *m = mods[mi];
            for (size_t s = 0; s < m->num_syms && rc == 0; s++) {
                EmitSymbol *sy = &m->syms[s];
                if (!sy->name || sy->binding == 0) continue;
                int strong = sy->binding != 2;
                if (want_strong != strong) continue;
                if (!sym_defined_here(sy)) continue;
                uint64_t addr;
                int lr = local_addr(&L, m, mi, sy, &addr);
                if (lr == -2) continue;
                if (lr != 0) { rc = -1; break; }
                int prev = gsym_find(gs, ngs, sy->name);
                int allow = comdat_allows_dup(mods, nmods, sy->name);
                if (gsym_add(&gs, &ngs, &capgs, sy->name, addr,
                             m->is_user_tu, strong, allow) != 0) {
                    rc = -1;
                    break;
                }
                if (prev < 0 && shared && strong && exportable_name(sy->name)) {
                    if (nexp == capexp) {
                        capexp = capexp ? capexp * 2 : 8;
                        exps = xrealloc(exps, capexp * sizeof(ExpSym));
                    }
                    exps[nexp].name = sy->name;
                    exps[nexp].rva = (uint32_t)(addr - IMAGE_BASE);
                    exps[nexp].is_data = sy->type != 2;
                    nexp++;
                }
            }
        }
    }
    if (rc == 0) {
        for (size_t c = 0; c < ncommon; c++) {
            uint64_t addr = L.data_va + L.data_file + commons[c].off;
            int prev = gsym_find(gs, ngs, commons[c].name);
            if (gsym_add(&gs, &ngs, &capgs, commons[c].name, addr, 0, 1, 0) != 0)
                rc = -1;
            else if (prev < 0 && shared && exportable_name(commons[c].name)) {
                if (nexp == capexp) {
                    capexp = capexp ? capexp * 2 : 8;
                    exps = xrealloc(exps, capexp * sizeof(ExpSym));
                }
                exps[nexp].name = commons[c].name;
                exps[nexp].rva = (uint32_t)(addr - IMAGE_BASE);
                exps[nexp].is_data = 1;
                nexp++;
            }
        }
    }
    if (rc == 0 && want_tramp) {
        uint64_t sa = L.text_va + stub_at;
        if (gsym_add(&gs, &ngs, &capgs, "__fakecc_win_syscall", sa, 0, 1, 0) != 0)
            rc = -1;
    }
    if (rc == 0) {
        for (size_t i = 0; i < nslots; i++) {
            if (!slots[i].publish) continue;
            uint32_t iat_rva = rdata_rva + (uint32_t)imp_at + slots[i].iat_boff;
            uint64_t iat_va = IMAGE_BASE + iat_rva;
            int exists = gsym_find(gs, ngs, slots[i].name) >= 0;
            if (!exists) {
                uint64_t addr = iat_va;
                if (!slots[i].is_data) {
                    if (slots[i].thunk_off == (size_t)-1) {
                        fprintf(stderr, "fakecc: internal: missing thunk for '%s'\n",
                                slots[i].name);
                        rc = -1;
                        break;
                    }
                    addr = L.text_va + stub_at + slots[i].thunk_off;
                }
                if (gsym_add(&gs, &ngs, &capgs, slots[i].name, addr, 0, 1, 0) != 0) {
                    rc = -1;
                    break;
                }
                int ix = gsym_find(gs, ngs, slots[i].name);
                if (ix >= 0 && slots[i].is_data) {
                    gs[ix].imp = 2;
                    gs[ix].iat_rva = iat_rva;
                }
            }
            size_t ln = strlen(slots[i].name);
            char *owned = xmalloc(ln + 7);
            memcpy(owned, "__imp_", 6);
            memcpy(owned + 6, slots[i].name, ln + 1);
            imp_owned = xrealloc(imp_owned, (nimp_owned + 1) * sizeof(char *));
            imp_owned[nimp_owned++] = owned;
            if (gsym_find(gs, ngs, owned) < 0) {
                if (gsym_add(&gs, &ngs, &capgs, owned, iat_va, 0, 1, 0) != 0) {
                    rc = -1;
                    break;
                }
            }
        }
    }

    uint64_t tls_index_va = 0;
    if (rc == 0 && have_tls) {
        int tix = gsym_find(gs, ngs, "_tls_index");
        if (tix >= 0) {
            tls_index_va = gs[tix].addr;
        } else {
            tls_index_va = L.data_va + tls_index_off;
            if (gsym_add(&gs, &ngs, &capgs, "_tls_index", tls_index_va, 0, 1, 0) != 0)
                rc = -1;
        }
    }

    size_t ndyn = 0;
    if (rc == 0)
        rc = apply_relocs(&L, mods, nmods, gs, ngs, &abs, &nabs, &capabs,
                          rdata_rva + (uint32_t)got_at, dyn_at, dyn_n, &ndyn);
    if (rc == 0 && ndyn != dyn_n) {
        fprintf(stderr, "fakecc: internal: data-import fixup count mismatch\n");
        rc = -1;
    }

    size_t nih = 0, nfh = 0;
    if (rc == 0)
        rc = fill_hooks(mods, nmods, 0, gs, ngs, &L, &ih, &nih,
                        rdata_rva + (uint32_t)init_at);
    if (rc == 0)
        rc = fill_hooks(mods, nmods, 1, gs, ngs, &L, &fh, &nfh,
                        rdata_rva + (uint32_t)fini_at);
    if (rc == 0) {
        for (size_t i = 0; i < nih; i++) {
            memcpy(rodata.data + init_at + i * 8, &ih[i].val, 8);
            add_abs(&abs, &nabs, &capabs, ih[i].rva_slot);
        }
        for (size_t i = 0; i < nfh; i++) {
            memcpy(rodata.data + fini_at + i * 8, &fh[i].val, 8);
            add_abs(&abs, &nabs, &capabs, fh[i].rva_slot);
        }
    }

    if (rc == 0 && tls_cb_n > 0) {
        size_t cb_idx = 0;
        for (size_t mi = 0; mi < nmods; mi++) {
            EmitModule *m = mods[mi];
            for (size_t r = 0; r < m->num_data_relocs; r++) {
                EmitReloc *rel = &m->data_relocs[r];
                if (rel->shndx != SECT_TLS_CALLBACKS) continue;
                size_t dummy;
                if (!remap_off(m, rel->shndx, rel->offset, &dummy)) continue;
                uint64_t S = 0;
                if (resolve_sym(&L, mods, gs, ngs, mi, rel->sym, &S, NULL, NULL) != 0) {
                    rc = -1;
                    break;
                }
                uint64_t val = S + (int64_t)rel->addend;
                memcpy(rodata.data + tls_cb_at + cb_idx * 8, &val, 8);
                add_abs(&abs, &nabs, &capabs, rdata_rva + (uint32_t)tls_cb_at + (uint32_t)cb_idx * 8);
                cb_idx++;
            }
            if (rc != 0) break;
        }
    }

    if (rc == 0 && have_tls) {
        uint64_t start_raw = L.tls_va;
        uint64_t end_raw = L.tls_va + tdata.len;
        uint64_t addr_index = tls_index_va;
        uint64_t addr_cb = L.rdata_va + tls_cb_at;
        uint32_t zero_fill = (uint32_t)tbss;
        uint32_t chars = align_chars(max_tls_align);

        uint8_t *dir = (uint8_t *)(rodata.data + tls_dir_at);
        memcpy(dir + 0, &start_raw, 8);
        memcpy(dir + 8, &end_raw, 8);
        memcpy(dir + 16, &addr_index, 8);
        memcpy(dir + 24, &addr_cb, 8);
        memcpy(dir + 32, &zero_fill, 4);
        memcpy(dir + 36, &chars, 4);

        add_abs(&abs, &nabs, &capabs, rdata_rva + (uint32_t)tls_dir_at + 0);
        add_abs(&abs, &nabs, &capabs, rdata_rva + (uint32_t)tls_dir_at + 8);
        add_abs(&abs, &nabs, &capabs, rdata_rva + (uint32_t)tls_dir_at + 16);
        add_abs(&abs, &nabs, &capabs, rdata_rva + (uint32_t)tls_dir_at + 24);
    }

    if (rc == 0 && nsyn && pdata.data) {
        for (size_t i = 0; i < nsyn; i++) {
            uint32_t begin = text_rva + (uint32_t)mod_text[syn[i].mi]
                           + (uint32_t)syn[i].begin;
            uint32_t end = begin + (uint32_t)syn[i].len;
            uint32_t unw = xdata_rva + syn[i].xoff;
            memcpy(pdata.data + syn_at + i * 12, &begin, 4);
            memcpy(pdata.data + syn_at + i * 12 + 4, &end, 4);
            memcpy(pdata.data + syn_at + i * 12 + 8, &unw, 4);
        }
    }
    if (rc == 0 && pdata.data && pdata.len >= 12 && (pdata.len % 12) == 0)
        qsort(pdata.data, pdata.len / 12, 12, cmp_pdata);

    uint32_t exp_size = 0;
    if (rc == 0 && nexp && exp_reserve && rodata.data) {
        for (size_t i = 1; i < nexp; i++) {
            ExpSym key = exps[i];
            size_t j = i;
            while (j > 0 && strcmp(exps[j - 1].name, key.name) > 0) {
                exps[j] = exps[j - 1];
                j--;
            }
            exps[j] = key;
        }
        Buffer eb;
        buffer_init(&eb);
        bpad(&eb, 40 + nexp * 4 + nexp * 4 + nexp * 2);
        size_t name_at = eb.len;
        buffer_append(&eb, dll_name, strlen(dll_name) + 1);
        size_t *name_off = xmalloc(nexp * sizeof(size_t));
        for (size_t i = 0; i < nexp; i++) {
            name_off[i] = eb.len;
            buffer_append(&eb, exps[i].name, strlen(exps[i].name) + 1);
        }
        if (eb.len > exp_reserve) {
            fprintf(stderr, "fakecc: internal: export directory overflow\n");
            rc = -1;
        } else {
            uint32_t base = rdata_rva + (uint32_t)exp_at;
            uint32_t eat = base + 40;
            uint32_t ntab = eat + (uint32_t)nexp * 4;
            uint32_t otab = ntab + (uint32_t)nexp * 4;
            uint32_t nrva = base + (uint32_t)name_at;
            char *d = eb.data;
            memset(d, 0, 40);
            memcpy(d + 12, &nrva, 4);
            uint32_t one = 1, nn = (uint32_t)nexp;
            memcpy(d + 16, &one, 4);
            memcpy(d + 20, &nn, 4);
            memcpy(d + 24, &nn, 4);
            memcpy(d + 28, &eat, 4);
            memcpy(d + 32, &ntab, 4);
            memcpy(d + 36, &otab, 4);
            for (size_t i = 0; i < nexp; i++) {
                memcpy(d + 40 + i * 4, &exps[i].rva, 4);
                uint32_t np = base + (uint32_t)name_off[i];
                memcpy(d + 40 + nexp * 4 + i * 4, &np, 4);
                uint16_t ord = (uint16_t)i;
                memcpy(d + 40 + nexp * 8 + i * 2, &ord, 2);
            }
            memcpy(rodata.data + exp_at, eb.data, eb.len);
            exp_size = (uint32_t)eb.len;
        }
        free(name_off);
        buffer_free(&eb);
    }

    if (rc == 0) {
        for (size_t i = 0; i < stub.nfix; i++) {
            Fixup *f = &stub.fix[i];
            size_t disp_at = stub_at + f->off;
            uint64_t next = L.text_va + disp_at + 4;
            uint64_t target = 0;
            if (f->kind == FK_IAT) {
                if (f->extra < 0 || f->extra >= IMP_COUNT) {
                    fprintf(stderr, "fakecc: internal: bad import fixup\n");
                    rc = -1;
                    break;
                }
                target = IMAGE_BASE + rdata_rva + imp_at + im.stub_boff[f->extra];
            } else if (f->kind == FK_SYNTH) {
                target = L.data_va + synth_off + (uint64_t)f->extra;
            } else if (f->kind == FK_RDATA) {
                target = L.rdata_va + (uint64_t)f->extra;
            } else if (f->kind == FK_CALL || f->kind == FK_STORE) {
                int ix = gsym_find(gs, ngs, f->name);
                if (ix < 0) {
                    if (f->name && f->name[0] == '?') {
                        fprintf(stderr, "fakecc: undefined symbol '%s' (unsupported C++ mangled symbol)\n", f->name);
                    } else {
                        fprintf(stderr, "fakecc: undefined symbol '%s'\n", f->name);
                    }
                    rc = -1;
                    break;
                }
                target = gs[ix].addr;
            } else {
                fprintf(stderr, "fakecc: internal: bad stub fixup\n");
                rc = -1;
                break;
            }
            int64_t disp = (int64_t)target - (int64_t)next;
            if (disp < (int64_t)INT32_MIN || disp > (int64_t)INT32_MAX) {
                fprintf(stderr, "fakecc: stub displacement out of range\n");
                rc = -1;
                break;
            }
            int32_t d = (int32_t)disp;
            memcpy(text.data + disp_at, &d, 4);
        }
    }

    if (rc == 0) build_reloc_section(&reloc, abs, nabs);

    Buffer debug_abbrev, debug_info, debug_str, debug_line, debug_frame, debug_loc;
    buffer_init(&debug_abbrev); buffer_init(&debug_info);
    buffer_init(&debug_str); buffer_init(&debug_line);
    buffer_init(&debug_frame); buffer_init(&debug_loc);
    int have_dbg = 0;
    typedef struct { const char *name; Buffer *buf; uint32_t rva, raw, rs; } PeDbgSec;
    PeDbgSec dsec[6];
    size_t ndsec = 0;
    uint32_t dbg_init_sum = 0;
    Buffer pe_strtab;
    buffer_init(&pe_strtab);
    bu32(&pe_strtab, 4);

    if (rc == 0) {
        for (size_t i = 0; i < nmods && !have_dbg; i++)
            have_dbg = pe_module_has_debug(mods[i]);
    }
    if (rc == 0 && have_dbg) {
        EmitModule dbg;
        emit_module_init(&dbg);
        for (size_t i = 0; i < nmods; i++) {
            EmitModule *m = mods[i];
            if (!dbg.dbg_tu_name && m->dbg_tu_name)
                dbg.dbg_tu_name = xstrdup(m->dbg_tu_name);
            for (size_t j = 0; j < m->num_dbg_lines; j++) {
                const DebugLineEntry *line = &m->dbg_lines[j];
                emit_module_add_dbg_line(&dbg, line->file, line->line,
                                         line->col,
                                         mod_text[i] + line->pc_off);
            }
            for (size_t j = 0; j < m->num_dbg_funcs; j++) {
                const DebugFunc *f = &m->dbg_funcs[j];
                int fi = emit_module_add_dbg_func(
                    &dbg, f->name, f->file, f->line,
                    mod_text[i] + f->start_pc);
                emit_module_dbg_func_end(
                    &dbg, fi, mod_text[i] + f->end_pc,
                    mod_text[i] + f->prologue_end_pc);
                for (size_t k = 0; k < f->num_vars; k++) {
                    emit_module_add_dbg_var(&dbg, fi, &f->vars[k]);
                    DebugVar *dv = &dbg.dbg_funcs[fi].vars[
                        dbg.dbg_funcs[fi].num_vars - 1];
                    for (size_t r = 0; r < dv->num_ranges; r++) {
                        dv->ranges[r].pc_start += mod_text[i];
                        dv->ranges[r].pc_end += mod_text[i];
                    }
                }
                for (size_t k = 0; k < f->num_call_sites; k++) {
                    DebugCallSite cs = f->call_sites[k];
                    cs.call_pc += mod_text[i];
                    cs.return_pc += mod_text[i];
                    emit_module_add_dbg_call_site(&dbg, fi, &cs);
                }
            }
            for (size_t j = 0; j < m->num_dbg_globals; j++) {
                DebugVar gv = m->dbg_globals[j];
                switch (gv.addr_sect) {
                case SECT_RODATA: gv.addr_off += mod_ro[i]; break;
                case SECT_DATA:   gv.addr_off += mod_data[i]; break;
                case SECT_BSS:    gv.addr_off += mod_bss[i]; break;
                case SECT_TDATA:  gv.addr_off += mod_tdata[i]; break;
                case SECT_TBSS:   gv.addr_off += mod_tbss[i]; break;
                default: break;
                }
                emit_module_add_dbg_global(&dbg, &gv);
            }
        }
        uint64_t sec_vaddr[16];
        memset(sec_vaddr, 0, sizeof sec_vaddr);
        sec_vaddr[SECT_TEXT]   = L.text_va;
        sec_vaddr[SECT_RODATA] = L.rdata_va;
        sec_vaddr[SECT_DATA]   = L.data_va;
        sec_vaddr[SECT_BSS]    = L.data_va + (uint64_t)data.len;
        if (have_tls) {
            sec_vaddr[SECT_TDATA] = L.tls_va;
            sec_vaddr[SECT_TBSS]  = L.tls_va + (uint64_t)tdata.len;
        }
        debug_emit_dwarf(&dbg, L.text_va, &debug_abbrev, &debug_info,
                         &debug_str, &debug_line, &debug_frame, &debug_loc,
                         sec_vaddr);
        emit_module_free(&dbg);
    }

    if (rc == 0) {
        int has_pdata = pdata.len > 0;
        int has_xdata = xdata.len > 0;
        int has_reloc = reloc.len > 0;
        int nsec = 3 + has_pdata + has_xdata + has_reloc + (have_tls ? 1 : 0);
        uint32_t headers = align_up_u32(0x80u + 4u + 20u + 240u + (uint32_t)nsec * 40u,
                                        FILE_ALIGN);
        uint32_t text_raw = headers;
        uint32_t text_rs = align_up_u32((uint32_t)text.len, FILE_ALIGN);
        uint32_t rdata_raw = text_raw + text_rs;
        uint32_t rdata_rs = align_up_u32((uint32_t)rodata.len, FILE_ALIGN);
        uint32_t data_raw = rdata_raw + rdata_rs;
        uint32_t data_rs = align_up_u32((uint32_t)data.len, FILE_ALIGN);
        uint32_t pdata_raw = data_raw + data_rs;
        uint32_t pdata_rs = has_pdata ? align_up_u32((uint32_t)pdata.len, FILE_ALIGN) : 0;
        uint32_t xdata_raw = pdata_raw + pdata_rs;
        uint32_t xdata_rs = has_xdata ? align_up_u32((uint32_t)xdata.len, FILE_ALIGN) : 0;
        uint32_t tls_raw = (have_tls && tdata.len > 0) ? (xdata_raw + xdata_rs) : 0;
        uint32_t tls_rs = (have_tls && tdata.len > 0) ? align_up_u32((uint32_t)tdata.len, FILE_ALIGN) : 0;
        uint32_t reloc_raw = (xdata_raw + xdata_rs) + tls_rs;
        uint32_t reloc_rs = has_reloc ? align_up_u32((uint32_t)reloc.len, FILE_ALIGN) : 0;
        uint32_t reloc_rva = 0;
        uint32_t image_end = cursor;
        if (has_reloc) {
            reloc_rva = cursor;
            image_end = align_up_u32(reloc_rva + (uint32_t)reloc.len, SECT_ALIGN);
        }

        /* DWARF sections: discardable, named via the COFF string table. */
        ndsec = 0;
        dbg_init_sum = 0;
        if (have_dbg) {
            Buffer *bufs[6] = {
                &debug_abbrev, &debug_info, &debug_str,
                &debug_line, &debug_frame, &debug_loc
            };
            const char *names[6] = {
                ".debug_abbrev", ".debug_info", ".debug_str",
                ".debug_line", ".debug_frame", ".debug_loc"
            };
            uint32_t dbg_file = reloc_raw + reloc_rs;
            uint32_t dbg_rva = image_end;
            for (size_t di = 0; di < 6; di++) {
                if (bufs[di]->len == 0) continue;
                dsec[ndsec].name = names[di];
                dsec[ndsec].buf = bufs[di];
                dsec[ndsec].rva = dbg_rva;
                dsec[ndsec].raw = dbg_file;
                dsec[ndsec].rs = align_up_u32((uint32_t)bufs[di]->len, FILE_ALIGN);
                dbg_rva = align_up_u32(dbg_rva + (uint32_t)bufs[di]->len, SECT_ALIGN);
                dbg_file += dsec[ndsec].rs;
                dbg_init_sum += dsec[ndsec].rs;
                ndsec++;
            }
            if (ndsec) {
                nsec += (int)ndsec;
                image_end = dbg_rva;
                headers = align_up_u32(0x80u + 4u + 20u + 240u + (uint32_t)nsec * 40u,
                                       FILE_ALIGN);
                if (headers != text_raw) {
                    int32_t delta = (int32_t)headers - (int32_t)text_raw;
                    text_raw = headers;
                    rdata_raw = (uint32_t)((int32_t)rdata_raw + delta);
                    data_raw = (uint32_t)((int32_t)data_raw + delta);
                    pdata_raw = (uint32_t)((int32_t)pdata_raw + delta);
                    xdata_raw = (uint32_t)((int32_t)xdata_raw + delta);
                    if (tls_raw) tls_raw = (uint32_t)((int32_t)tls_raw + delta);
                    reloc_raw = (uint32_t)((int32_t)reloc_raw + delta);
                    for (size_t di = 0; di < ndsec; di++)
                        dsec[di].raw = (uint32_t)((int32_t)dsec[di].raw + delta);
                }
            }
        }

        uint16_t coff_chars = 0x0022;
        if (shared) coff_chars = (uint16_t)(coff_chars | 0x2000);
        uint16_t dll_chars = 0x0100;
        if (!has_reloc && !shared) coff_chars = (uint16_t)(coff_chars | 0x0001);
        if (has_reloc) dll_chars = (uint16_t)(dll_chars | 0x0040 | 0x0020);
        uint32_t entry_rva = 0;
        if (entry_at != (size_t)-1)
            entry_rva = text_rva + (uint32_t)(stub_at + entry_at);

        Buffer file;
        buffer_init(&file);
        {
            uint8_t dos[0x80];
            memset(dos, 0, sizeof dos);
            dos[0] = 'M'; dos[1] = 'Z';
            uint32_t lfanew = 0x80;
            memcpy(dos + 0x3C, &lfanew, 4);
            buffer_append(&file, (const char *)dos, sizeof dos);
        }
        buffer_append(&file, "PE\0\0", 4);
        bu16(&file, 0x8664);
        bu16(&file, (uint16_t)nsec);
        bu32(&file, 0);
        size_t pe_symptr_at = file.len;
        bu32(&file, 0); /* PointerToSymbolTable */
        bu32(&file, 0); /* NumberOfSymbols */
        bu16(&file, 240);
        bu16(&file, coff_chars);
        bu16(&file, 0x20B);
        bu8(&file, 1); bu8(&file, 0);
        bu32(&file, text_rs);
        bu32(&file, rdata_rs + data_rs + pdata_rs + xdata_rs + reloc_rs + tls_rs
                    + dbg_init_sum);
        bu32(&file, (uint32_t)bss + (uint32_t)tbss);
        bu32(&file, entry_rva);
        bu32(&file, text_rva);
        bu64(&file, IMAGE_BASE);
        bu32(&file, SECT_ALIGN);
        bu32(&file, FILE_ALIGN);
        bu16(&file, 6); bu16(&file, 0);
        bu16(&file, 0); bu16(&file, 0);
        bu16(&file, 6); bu16(&file, 0);
        bu32(&file, 0);
        bu32(&file, image_end);
        bu32(&file, headers);
        bu32(&file, 0);
        bu16(&file, 3);
        bu16(&file, dll_chars);
        /* 8MB, the usual host default.  1MB overflows while the self-hosted
         * compiler parses a large module such as cg64.c. */
        bu64(&file, 0x800000);
        bu64(&file, 0x1000);
        bu64(&file, 0x100000);
        bu64(&file, 0x1000);
        bu32(&file, 0);
        bu32(&file, 16);
        uint32_t exc_sz = has_pdata ? (uint32_t)((pdata.len / 12) * 12) : 0;
        for (int di = 0; di < 16; di++) {
            uint32_t r = 0, sz = 0;
            if (di == 0 && exp_size) {
                r = rdata_rva + (uint32_t)exp_at;
                sz = exp_size;
            } else if (di == 1 && im.dir_size) {
                r = rdata_rva + (uint32_t)imp_at + im.dir_off;
                sz = im.dir_size;
            } else if (di == 3 && exc_sz) {
                r = pdata_rva;
                sz = exc_sz;
            } else if (di == 5 && has_reloc) {
                r = reloc_rva;
                sz = (uint32_t)reloc.len;
            } else if (di == 9 && have_tls) {
                r = rdata_rva + (uint32_t)tls_dir_at;
                sz = 40;
            } else if (di == 12 && im.iat_size) {
                r = rdata_rva + (uint32_t)imp_at + im.iat_off;
                sz = im.iat_size;
            }
            bu32(&file, r);
            bu32(&file, sz);
        }
        put_section(&file, ".text", (uint32_t)text.len, text_rva,
                    text_rs, text_raw, SCN_CODE | SCN_EXEC | SCN_READ);
        put_section(&file, ".rdata", (uint32_t)rodata.len, rdata_rva,
                    rdata_rs, rdata_raw, SCN_IDATA | SCN_READ);
        put_section(&file, ".data", data_vsize, data_rva,
                    data_rs, data_raw,
                    SCN_IDATA | SCN_UDATA | SCN_READ | SCN_WRITE);
        if (has_pdata)
            put_section(&file, ".pdata", (uint32_t)pdata.len, pdata_rva,
                        pdata_rs, pdata_raw, SCN_IDATA | SCN_READ);
        if (has_xdata)
            put_section(&file, ".xdata", (uint32_t)xdata.len, xdata_rva,
                        xdata_rs, xdata_raw, SCN_IDATA | SCN_READ);
        if (have_tls) {
            uint32_t tls_chars = SCN_READ | SCN_WRITE | align_chars(max_tls_align);
            if (tdata.len > 0) tls_chars |= SCN_IDATA;
            if (tbss > 0) tls_chars |= SCN_UDATA;
            put_section(&file, ".tls", tls_vsize, tls_rva,
                        tls_rs, tls_raw, tls_chars);
        }
        if (has_reloc)
            put_section(&file, ".reloc", (uint32_t)reloc.len, reloc_rva,
                        reloc_rs, reloc_raw,
                        SCN_IDATA | SCN_READ | SCN_DISCARD);
        for (size_t di = 0; di < ndsec; di++) {
            put_section_long(&file, &pe_strtab, dsec[di].name,
                             (uint32_t)dsec[di].buf->len, dsec[di].rva,
                             dsec[di].rs, dsec[di].raw,
                             SCN_IDATA | SCN_READ | SCN_DISCARD);
        }

        if (file.len > headers) {
            fprintf(stderr, "fakecc: internal: PE headers overflowed\n");
            rc = -1;
        } else {
            bpad(&file, headers - file.len);
            buffer_append(&file, text.data, text.len);
            bpad(&file, text_rs - text.len);
            buffer_append(&file, rodata.data, rodata.len);
            bpad(&file, rdata_rs - rodata.len);
            buffer_append(&file, data.data, data.len);
            bpad(&file, data_rs - data.len);
            if (has_pdata) {
                buffer_append(&file, pdata.data, pdata.len);
                bpad(&file, pdata_rs - pdata.len);
            }
            if (has_xdata) {
                buffer_append(&file, xdata.data, xdata.len);
                bpad(&file, xdata_rs - xdata.len);
            }
            if (have_tls && tdata.len > 0) {
                buffer_append(&file, tdata.data, tdata.len);
                bpad(&file, tls_rs - tdata.len);
            }
            if (has_reloc) {
                buffer_append(&file, reloc.data, reloc.len);
                bpad(&file, reloc_rs - reloc.len);
            }
            for (size_t di = 0; di < ndsec; di++) {
                buffer_append(&file, dsec[di].buf->data, dsec[di].buf->len);
                bpad(&file, dsec[di].rs - dsec[di].buf->len);
            }
            /* COFF symbol table + string table.  gdb's `break main` without
             * -g needs EXTERNAL symbols; DWARF alone is not enough.  Long
             * section names (/offset) and long symbol names share pe_strtab. */
            {
                Buffer pe_symtab;
                buffer_init(&pe_symtab);
                if (pe_strtab.len < 4) {
                    buffer_init(&pe_strtab);
                    bu32(&pe_strtab, 4);
                }
                /* 1-based section numbers matching put_section order above. */
                uint16_t sec_text = 1, sec_rdata = 2, sec_data = 3;
                uint16_t sec_next = 4;
                uint16_t sec_pdata = 0, sec_xdata = 0, sec_tls = 0, sec_reloc = 0;
                if (has_pdata) sec_pdata = sec_next++;
                if (has_xdata) sec_xdata = sec_next++;
                if (have_tls) sec_tls = sec_next++;
                if (has_reloc) sec_reloc = sec_next++;
                (void)sec_pdata; (void)sec_xdata; (void)sec_tls; (void)sec_reloc;

                for (size_t si = 0; si < ngs; si++) {
                    GSym *e = &gs[si];
                    if (!e->name || e->imp || !e->strong) continue;
                    uint64_t a = e->addr;
                    uint16_t sn = 0;
                    uint32_t off = 0;
                    uint16_t ty = 0;
                    if (a >= L.text_va && a < L.text_va + text.len) {
                        sn = sec_text;
                        off = (uint32_t)(a - L.text_va);
                        ty = 0x20; /* IMAGE_SYM_DTYPE_FUNCTION */
                    } else if (a >= L.rdata_va && a < L.rdata_va + rodata.len) {
                        sn = sec_rdata;
                        off = (uint32_t)(a - L.rdata_va);
                    } else if (a >= L.data_va &&
                               a < L.data_va + data_vsize) {
                        sn = sec_data;
                        off = (uint32_t)(a - L.data_va);
                    } else {
                        continue;
                    }
                    /* Symbol name: 8-byte field or string-table offset. */
                    {
                        char nm[8];
                        memset(nm, 0, 8);
                        size_t nl = strlen(e->name);
                        if (nl <= 8) {
                            memcpy(nm, e->name, nl);
                        } else {
                            uint32_t soff = (uint32_t)pe_strtab.len;
                            buffer_append(&pe_strtab, e->name, nl + 1);
                            memset(nm, 0, 8);
                            memcpy(nm + 4, &soff, 4);
                        }
                        buffer_append(&pe_symtab, nm, 8);
                    }
                    bu32(&pe_symtab, off);
                    bu16(&pe_symtab, sn);
                    bu16(&pe_symtab, ty);
                    bu8(&pe_symtab, IMAGE_SYM_CLASS_EXTERNAL);
                    bu8(&pe_symtab, 0);
                }

                uint32_t nsyms = (uint32_t)(pe_symtab.len / 18);
                if (nsyms > 0 || pe_strtab.len > 4) {
                    uint32_t slen = (uint32_t)pe_strtab.len;
                    memcpy(pe_strtab.data, &slen, 4);
                    uint32_t symptr = (uint32_t)file.len;
                    memcpy(file.data + pe_symptr_at, &symptr, 4);
                    memcpy(file.data + pe_symptr_at + 4, &nsyms, 4);
                    if (nsyms > 0)
                        buffer_append(&file, pe_symtab.data, pe_symtab.len);
                    buffer_append(&file, pe_strtab.data, pe_strtab.len);
                }
                buffer_free(&pe_symtab);
            }
            FILE *fp = fopen(path, "wb");
            if (!fp) {
                fprintf(stderr, "fakecc: cannot write '%s'\n", path);
                rc = -1;
            } else {
                size_t wr = fwrite(file.data, 1, file.len, fp);
                if (wr != file.len) {
                    fprintf(stderr, "fakecc: short write of '%s'\n", path);
                    rc = -1;
                }
                fclose(fp);
            }
        }
        buffer_free(&file);
    }

    if (rc == 0 && shared && nexp) {
        PeImplibSym *isyms = xmalloc(nexp * sizeof(PeImplibSym));
        for (size_t i = 0; i < nexp; i++) {
            isyms[i].name = exps[i].name;
            isyms[i].is_data = exps[i].is_data;
        }
        if (pe_write_import_libs(path, implib_path, dll_name, isyms, nexp) != 0)
            rc = -1;
        free(isyms);
    }

    buffer_free(&debug_abbrev); buffer_free(&debug_info);
    buffer_free(&debug_str); buffer_free(&debug_line);
    buffer_free(&debug_frame); buffer_free(&debug_loc);
    buffer_free(&pe_strtab);

done:
    buffer_free(&text); buffer_free(&rodata); buffer_free(&data);
    buffer_free(&pdata); buffer_free(&xdata); buffer_free(&reloc);
    buffer_free(&tdata);
    buffer_free(&stub.buf); buffer_free(&im.blob);
    free(stub.fix);
    free(mod_text); free(mod_ro); free(mod_data); free(mod_bss);
    free(mod_pdata); free(mod_xdata);
    free(mod_tdata); free(mod_tbss);
    free(slots); free(syn); free(exps); free(gs); free(abs);
    for (size_t i = 0; i < nimp_owned; i++) free(imp_owned[i]);
    free(imp_owned);
    free(ih); free(fh); free(commons);
    return rc;
}

int pe_link(EmitModule **mods, size_t nmods, const char *path) {
    return pe_link_ex(mods, nmods, path, NULL);
}
