#include "fakecc/emit.h"
#include "fakecc/target.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#else
#include <io.h>
#include <process.h>
#ifndef unlink
#define unlink _unlink
#endif
#endif

#ifdef _WIN32
/* Host-writable temp path (MSYS /tmp is not a native Win32 path). */
static const char *test_tmp(const char *name) {
    static char buf[8][260];
    static int slot;
    char *p = buf[slot++ & 7];
    const char *t = getenv("TEMP");
    if (!t || !t[0]) t = ".";
    snprintf(p, 260, "%s\\%s", t, name);
    return p;
}
#define TMP_PATH(name) test_tmp(name)
#else
#define TMP_PATH(name) ("/tmp/" name)
#endif

/* Craft two ELF64 ET_REL objects via nasm (boot + high), link with
 * link_kernel_multiboot, and check Multiboot ELF32 shape (fakeos S1+S2). */

static int run_cmd(char *const argv[]) {
#ifdef _WIN32
    if (!argv || !argv[0]) return 0;
    intptr_t rc = _spawnvp(_P_WAIT, argv[0], (const char *const *)argv);
    return rc == 0;
#else
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    int st = 0;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) && WEXITSTATUS(st) == 0;
#endif
}

static int write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(text, f);
    fclose(f);
    return 1;
}

static uint16_t rd16(const unsigned char *p) {
    uint16_t v; memcpy(&v, p, 2); return v;
}
static uint32_t rd32(const unsigned char *p) {
    uint32_t v; memcpy(&v, p, 4); return v;
}

static unsigned char *slurp(const char *path, size_t *n_out) {
    FILE *f = fopen(path, "rb");
    T_ASSERT(f != NULL);
    fseek(f, 0, SEEK_SET);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *b = malloc((size_t)n);
    T_ASSERT(b != NULL);
    T_ASSERT_EQ_INT((int)fread(b, 1, (size_t)n, f), (int)n);
    fclose(f);
    *n_out = (size_t)n;
    return b;
}

static void test_kernel_multiboot_shape(void) {
    const char *boot_asm = TMP_PATH("fakecc_kmb_boot.asm");
    const char *kern_asm = TMP_PATH("fakecc_kmb_kern.asm");
    const char *boot_o = TMP_PATH("fakecc_kmb_boot.o");
    const char *kern_o = TMP_PATH("fakecc_kmb_kern.o");
    const char *out = TMP_PATH("fakecc_kmb_out.elf");

    /* Multiboot header in .multiboot; _start in .boot.text (low LMA). */
    T_ASSERT(write_file(boot_asm,
        "section .multiboot\n"
        "dd 0x1BADB002\n"
        "dd 0\n"
        "dd -(0x1BADB002)\n"
        "section .boot.text\n"
        "global _start\n"
        "_start:\n"
        "    ret\n"));
    /* Higher-half .text + .data reloc against synthetic _kernel_text_start. */
    T_ASSERT(write_file(kern_asm,
        "section .text\n"
        "global kmain\n"
        "kmain:\n"
        "    ret\n"
        "section .data\n"
        "global marker\n"
        "extern _kernel_text_start\n"
        "marker:\n"
        "    dq _kernel_text_start\n"
        "section .bss\n"
        "resb 8\n"));

    {
        char *av1[] = { "nasm", "-f", "elf64",  (char *)boot_asm, "-o", (char *)boot_o, NULL };
        char *av2[] = { "nasm", "-f", "elf64",  (char *)kern_asm, "-o", (char *)kern_o, NULL };
        if (!run_cmd(av1) || !run_cmd(av2)) {
            printf("  SKIP test_kernel_multiboot_shape (nasm failed)\n");
            return;
        }
    }

    const char *paths[2] = { boot_o, kern_o };
    T_ASSERT_EQ_INT(link_kernel_multiboot(paths, 2, out), 0);

    size_t n = 0;
    unsigned char *b = slurp(out, &n);
    T_ASSERT(n > 0x1000 + 16);
    T_ASSERT(b[0] == 0x7f && b[1] == 'E' && b[2] == 'L' && b[3] == 'F');
    T_ASSERT_EQ_INT((int)b[4], 1); /* ELFCLASS32 */
    T_ASSERT_EQ_INT((int)rd16(b + 16), 2); /* ET_EXEC */
    T_ASSERT_EQ_INT((int)rd16(b + 18), 3); /* EM_386 */
    T_ASSERT_EQ_INT((int)rd16(b + 44), 2); /* e_phnum */
    T_ASSERT_EQ_INT((int)rd32(b + 32), 0); /* no section headers */

    /* Multiboot magic at file offset HDR_PAD (0x1000). */
    T_ASSERT_EQ_INT((int)rd32(b + 0x1000), (int)0x1BADB002);

    uint32_t entry = rd32(b + 24);
    T_ASSERT(entry >= 0x100000 && entry < 0x101000);

    uint32_t phoff = rd32(b + 28);
    T_ASSERT_EQ_INT((int)phoff, 52);
    /* Low PT_LOAD */
    T_ASSERT_EQ_INT((int)rd32(b + phoff + 0), 1);
    T_ASSERT_EQ_INT((int)rd32(b + phoff + 4), 0x1000);
    T_ASSERT_EQ_INT((int)rd32(b + phoff + 8), 0x100000);
    T_ASSERT_EQ_INT((int)rd32(b + phoff + 12), 0x100000);
    uint32_t lo_filesz = rd32(b + phoff + 16);
    T_ASSERT(lo_filesz > 0 && lo_filesz <= 0x5000);

    /* High PT_LOAD: truncated VMA (low 32 of KERNEL_VMA+LMA), paddr = LMA */
    unsigned char *hi = b + phoff + 32;
    T_ASSERT_EQ_INT((int)rd32(hi + 0), 1);
    uint32_t hi_off = rd32(hi + 4);
    uint32_t hi_vaddr = rd32(hi + 8);
    uint32_t hi_paddr = rd32(hi + 12);
    T_ASSERT(hi_off >= 0x1000);
    T_ASSERT((hi_vaddr & 0x80000000u) != 0); /* top bit set from KERNEL_VMA */
    T_ASSERT(hi_paddr >= 0x100000);
    T_ASSERT(rd32(hi + 16) > 0);
    T_ASSERT(rd32(hi + 20) >= rd32(hi + 16));

    /* .data marker should hold full KERNEL_VMA + text LMA (R_X86_64_64).
     * Find marker at high segment: kmain is 1 byte ret; .data follows after
     * 4K align typically. Read 8 bytes at file offset of high content that
     * looks like 0xFFFFFFFF8....... — scan high segment file bytes. */
    int found_kva = 0;
    for (size_t off = hi_off; off + 8 <= n; off++) {
        uint64_t q;
        memcpy(&q, b + off, 8);
        if ((q >> 32) == 0xFFFFFFFFu && (uint32_t)q >= 0x80000000u) {
            found_kva = 1;
            break;
        }
    }
    T_ASSERT(found_kva);

    free(b);
    unlink(out);
    unlink(boot_o);
    unlink(kern_o);
    unlink(boot_asm);
    unlink(kern_asm);
    (void)entry;
    (void)lo_filesz;
}


/* Stress path that would fail under the old objs[40] / 768 KiB arena. */
static void test_kernel_capacity_stress(void) {
    const char *boot_asm = TMP_PATH("fakecc_kcap_boot.asm");
    const char *kern_asm = TMP_PATH("fakecc_kcap_kern.asm");
    const char *boot_o = TMP_PATH("fakecc_kcap_boot.o");
    const char *kern_o = TMP_PATH("fakecc_kcap_kern.o");
    const char *blob_asm = TMP_PATH("fakecc_kcap_blob.asm");
    const char *blob_o = TMP_PATH("fakecc_kcap_blob.o");
    const char *out = TMP_PATH("fakecc_kcap_out.elf");
    const char *paths[96];
    int npaths = 0;
    int i;

    T_ASSERT(write_file(boot_asm,
        "section .multiboot\n"
        "dd 0x1BADB002\n"
        "dd 0\n"
        "dd -(0x1BADB002)\n"
        "section .boot.text\n"
        "global _start\n"
        "_start:\n"
        "    ret\n"));
    T_ASSERT(write_file(kern_asm,
        "section .text\n"
        "global kmain\n"
        "kmain:\n"
        "    ret\n"
        "section .data\n"
        "global marker\n"
        "marker:\n"
        "    dq 0\n"));
    /* ~900 KiB PROGBITS — exceeds legacy OBJ_ARENA (786432). */
    T_ASSERT(write_file(blob_asm,
        "section .rodata\n"
        "global bigblob\n"
        "bigblob:\n"
        "times 900*1024 db 0xA5\n"));

    {
        char *av1[] = { "nasm", "-f", "elf64", (char *)boot_asm, "-o", (char *)boot_o, NULL };
        char *av2[] = { "nasm", "-f", "elf64", (char *)kern_asm, "-o", (char *)kern_o, NULL };
        char *av3[] = { "nasm", "-f", "elf64", (char *)blob_asm, "-o", (char *)blob_o, NULL };
        if (!run_cmd(av1) || !run_cmd(av2) || !run_cmd(av3)) {
            printf("  SKIP test_kernel_capacity_stress (nasm failed)\n");
            return;
        }
    }

    paths[npaths++] = boot_o;
    /* 50 copies of kern.o: exceeds legacy objs[40] slot limit. */
    for (i = 0; i < 50; i++)
        paths[npaths++] = kern_o;
    paths[npaths++] = blob_o;

    T_ASSERT_EQ_INT(link_kernel_multiboot(paths, (size_t)npaths, out), 0);

    size_t n = 0;
    unsigned char *b = slurp(out, &n);
    T_ASSERT(n > 0x1000);
    T_ASSERT(b[0] == 0x7f && b[1] == 'E' && b[2] == 'L' && b[3] == 'F');
    T_ASSERT_EQ_INT((int)b[4], 1);
    free(b);

    unlink(out);
    unlink(boot_o);
    unlink(kern_o);
    unlink(blob_o);
    unlink(boot_asm);
    unlink(kern_asm);
    unlink(blob_asm);
}

int main(void) {
    target_set_current(target_x86_64_linux());
    test_kernel_multiboot_shape();
    test_kernel_capacity_stress();
    return t_finalize();
}
