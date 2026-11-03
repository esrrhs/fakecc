#include "fakecc/emit.h"
#include "fakecc/ldscript.h"
#include "fakecc/target.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static const char *KERNEL_LD =
    "OUTPUT_FORMAT(elf64-x86-64)\n"
    "ENTRY(_start)\n"
    "\n"
    "KERNEL_VMA = 0xFFFFFFFF80000000;\n"
    "KERNEL_LMA = 0x00100000;\n"
    "\n"
    "SECTIONS\n"
    "{\n"
    "    . = KERNEL_LMA;\n"
    "\n"
    "    /* bootstrap */\n"
    "    .boot : {\n"
    "        KEEP(*(.multiboot))\n"
    "        *(.boot.text)\n"
    "        *(.boot.data)\n"
    "        *(.boot.bss)\n"
    "    }\n"
    "\n"
    "    . += KERNEL_VMA;\n"
    "\n"
    "    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA) {\n"
    "        _kernel_text_start = .;\n"
    "        *(.text .text.*)\n"
    "        _kernel_text_end = .;\n"
    "    }\n"
    "\n"
    "    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA) {\n"
    "        _kernel_rodata_start = .;\n"
    "        *(.rodata .rodata.*)\n"
    "        _kernel_rodata_end = .;\n"
    "    }\n"
    "\n"
    "    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA) {\n"
    "        _kernel_data_start = .;\n"
    "        *(.data .data.*)\n"
    "        _kernel_data_end = .;\n"
    "    }\n"
    "\n"
    "    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA) {\n"
    "        _kernel_bss_start = .;\n"
    "        *(.bss .bss.*)\n"
    "        *(COMMON)\n"
    "        _kernel_bss_end = .;\n"
    "    }\n"
    "\n"
    "    _kernel_end = .;\n"
    "}\n";

static const char *USER_LD =
    "OUTPUT_FORMAT(elf64-x86-64)\n"
    "ENTRY(_start)\n"
    "\n"
    "USER_VMA = 0x400000;\n"
    "\n"
    "PHDRS\n"
    "{\n"
    "    text PT_LOAD FLAGS(5);   /* R + X */\n"
    "    data PT_LOAD FLAGS(6);   /* R + W */\n"
    "}\n"
    "\n"
    "SECTIONS\n"
    "{\n"
    "    . = USER_VMA;\n"
    "\n"
    "    .text ALIGN(4K) : {\n"
    "        *(.text .text.*)\n"
    "    } :text\n"
    "\n"
    "    .rodata ALIGN(4K) : {\n"
    "        *(.rodata .rodata.*)\n"
    "    } :text\n"
    "\n"
    "    .data ALIGN(4K) : {\n"
    "        *(.data .data.*)\n"
    "    } :data\n"
    "\n"
    "    .bss ALIGN(4K) : {\n"
    "        *(.bss .bss.*)\n"
    "        *(COMMON)\n"
    "    } :data\n"
    "}\n";

static int write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(text, f);
    fclose(f);
    return 1;
}

static int run_cmd(char *const argv[]) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    int st = 0;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) && WEXITSTATUS(st) == 0;
}

static uint16_t rd16(const unsigned char *p) {
    uint16_t v; memcpy(&v, p, 2); return v;
}
static uint32_t rd32(const unsigned char *p) {
    uint32_t v; memcpy(&v, p, 4); return v;
}
static uint64_t rd64(const unsigned char *p) {
    uint64_t v; memcpy(&v, p, 8); return v;
}

static unsigned char *slurp(const char *path, size_t *n_out) {
    FILE *f = fopen(path, "rb");
    T_ASSERT(f != NULL);
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

static void test_parse_kernel_script(void) {
    LdScriptInfo info;
    T_ASSERT_EQ_INT(ldscript_parse_string(KERNEL_LD, "boot/linker.ld", &info), 0);
    T_ASSERT_EQ_INT((int)info.kind, (int)LDSCRIPT_KIND_KERNEL_HH);
    T_ASSERT(strcmp(info.entry, "_start") == 0);
    T_ASSERT(info.has_kernel_vma);
    T_ASSERT(info.has_kernel_lma);
    T_ASSERT_EQ_INT((int)(info.kernel_vma == 0xFFFFFFFF80000000ULL), 1);
    T_ASSERT_EQ_INT((int)(info.kernel_lma == 0x100000ULL), 1);
    T_ASSERT(info.saw_boot_section);
    T_ASSERT(info.saw_keep_multiboot);
    T_ASSERT(info.saw_dot_plus_kernel_vma);
    T_ASSERT(info.saw_at_addr_minus_kernel_vma);

    int kmb = 0;
    T_ASSERT_EQ_INT(ldscript_configure_link(&info, &kmb), 0);
    T_ASSERT_EQ_INT(kmb, 1);
}

static void test_parse_user_script(void) {
    LdScriptInfo info;
    T_ASSERT_EQ_INT(ldscript_parse_string(USER_LD, "user/user.ld", &info), 0);
    T_ASSERT_EQ_INT((int)info.kind, (int)LDSCRIPT_KIND_USER);
    T_ASSERT(strcmp(info.entry, "_start") == 0);
    T_ASSERT(info.has_user_vma);
    T_ASSERT_EQ_INT((int)(info.user_vma == 0x400000ULL), 1);
    T_ASSERT(info.has_phdrs);
    T_ASSERT_EQ_INT(info.text_phdr_flags, 5);
    T_ASSERT_EQ_INT(info.data_phdr_flags, 6);

    link_set_image_base(0x111111ULL);
    int kmb = 1;
    T_ASSERT_EQ_INT(ldscript_configure_link(&info, &kmb), 0);
    T_ASSERT_EQ_INT(kmb, 0);
    T_ASSERT_EQ_INT((int)(link_image_base() == 0x400000ULL), 1);
    link_set_image_base(0);
}

static void test_parse_fakeos_files_if_present(void) {
    const char *kpath = "/workspace/github/fakeos/boot/linker.ld";
    const char *upath = "/workspace/github/fakeos/user/user.ld";
    FILE *fk = fopen(kpath, "r");
    FILE *fu = fopen(upath, "r");
    if (!fk || !fu) {
        if (fk) fclose(fk);
        if (fu) fclose(fu);
        printf("  SKIP test_parse_fakeos_files_if_present (fakeos scripts absent)\n");
        return;
    }
    fclose(fk);
    fclose(fu);

    LdScriptInfo ki, ui;
    T_ASSERT_EQ_INT(ldscript_parse_file(kpath, &ki), 0);
    T_ASSERT_EQ_INT((int)ki.kind, (int)LDSCRIPT_KIND_KERNEL_HH);
    T_ASSERT_EQ_INT(ldscript_parse_file(upath, &ui), 0);
    T_ASSERT_EQ_INT((int)ui.kind, (int)LDSCRIPT_KIND_USER);
}

/* -T kernel script → Multiboot ELF32 via fakecc CLI (or API configure + link). */
static void test_T_kernel_multiboot_e2e(void) {
    const char *boot_asm = "/tmp/fakecc_T_k_boot.asm";
    const char *kern_asm = "/tmp/fakecc_T_k_kern.asm";
    const char *boot_o = "/tmp/fakecc_T_k_boot.o";
    const char *kern_o = "/tmp/fakecc_T_k_kern.o";
    const char *script = "/tmp/fakecc_T_k_linker.ld";
    const char *out = "/tmp/fakecc_T_k_out.elf";

    T_ASSERT(write_file(script, KERNEL_LD));
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
        "extern _kernel_text_start\n"
        "marker:\n"
        "    dq _kernel_text_start\n"
        "section .bss\n"
        "resb 8\n"));

    {
        char *av1[] = { "nasm", "-f", "elf64", (char *)boot_asm, "-o", (char *)boot_o, NULL };
        char *av2[] = { "nasm", "-f", "elf64", (char *)kern_asm, "-o", (char *)kern_o, NULL };
        if (!run_cmd(av1) || !run_cmd(av2)) {
            printf("  SKIP test_T_kernel_multiboot_e2e (nasm failed)\n");
            return;
        }
    }

    /* Prefer CLI -T when fakecc binary is available next to tests / in build. */
    const char *fcc_candidates[] = {
        "./fakecc",
        "../fakecc",
        "../../fakecc",
        "/workspace/github/fakecc/build/fakecc",
        NULL
    };
    const char *fcc = NULL;
    for (int i = 0; fcc_candidates[i]; i++) {
        if (access(fcc_candidates[i], X_OK) == 0) {
            fcc = fcc_candidates[i];
            break;
        }
    }

    if (fcc) {
        char *av[] = {
            (char *)fcc, "-nostdlib", "-T", (char *)script,
            (char *)boot_o, (char *)kern_o, "-o", (char *)out, NULL
        };
        T_ASSERT(run_cmd(av));
    } else {
        /* API path: configure from script then link_kernel_multiboot. */
        LdScriptInfo info;
        T_ASSERT_EQ_INT(ldscript_parse_file(script, &info), 0);
        int kmb = 0;
        T_ASSERT_EQ_INT(ldscript_configure_link(&info, &kmb), 0);
        T_ASSERT_EQ_INT(kmb, 1);
        const char *paths[2] = { boot_o, kern_o };
        T_ASSERT_EQ_INT(link_kernel_multiboot(paths, 2, out), 0);
    }

    size_t n = 0;
    unsigned char *b = slurp(out, &n);
    T_ASSERT(n > 0x1000 + 16);
    T_ASSERT(b[0] == 0x7f && b[1] == 'E' && b[2] == 'L' && b[3] == 'F');
    T_ASSERT_EQ_INT((int)b[4], 1); /* ELFCLASS32 */
    T_ASSERT_EQ_INT((int)rd16(b + 16), 2); /* ET_EXEC */
    T_ASSERT_EQ_INT((int)rd16(b + 18), 3); /* EM_386 */
    T_ASSERT_EQ_INT((int)rd16(b + 44), 2); /* e_phnum */
    T_ASSERT_EQ_INT((int)rd32(b + 0x1000), (int)0x1BADB002);
    free(b);

    unlink(out);
    unlink(boot_o);
    unlink(kern_o);
    unlink(boot_asm);
    unlink(kern_asm);
    unlink(script);
}

/* -T user script → image base 0x400000 + dual RX/RW PT_LOAD. */
static void test_T_user_image_base_e2e(void) {
#if !(defined(__linux__) && (defined(__x86_64__) || defined(__amd64__)))
    printf("  SKIP test_T_user_image_base_e2e (linux x86_64 only)\n");
    return;
#else
    const char *script = "/tmp/fakecc_T_u_user.ld";
    const char *src = "/tmp/fakecc_T_u_main.c";
    const char *out = "/tmp/fakecc_T_u_out.elf";
    const char *script6 =
        "OUTPUT_FORMAT(elf64-x86-64)\n"
        "ENTRY(_start)\n"
        "USER_VMA = 0x600000;\n"
        "PHDRS { text PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }\n"
        "SECTIONS {\n"
        "  . = USER_VMA;\n"
        "  .text ALIGN(4K) : { *(.text .text.*) } :text\n"
        "  .rodata ALIGN(4K) : { *(.rodata .rodata.*) } :text\n"
        "  .data ALIGN(4K) : { *(.data .data.*) } :data\n"
        "  .bss ALIGN(4K) : { *(.bss .bss.*) *(COMMON) } :data\n"
        "}\n";
    T_ASSERT(write_file(script, script6));
    T_ASSERT(write_file(src, "package main; int main(void) { return 0; }\n"));

    const char *fcc_candidates[] = {
        "./fakecc",
        "../fakecc",
        "../../fakecc",
        "/workspace/github/fakecc/build/fakecc",
        NULL
    };
    const char *fcc = NULL;
    for (int i = 0; fcc_candidates[i]; i++) {
        if (access(fcc_candidates[i], X_OK) == 0) {
            fcc = fcc_candidates[i];
            break;
        }
    }

    int linked = 0;
    if (fcc) {
        char *av[] = {
            (char *)fcc, "-T", (char *)script,
            (char *)src, "-o", (char *)out, NULL
        };
        linked = run_cmd(av);
        T_ASSERT(linked);
    }

    if (!linked) {
        /* API path: parse/configure then link a tiny module at USER_VMA. */
        LdScriptInfo info;
        T_ASSERT_EQ_INT(ldscript_parse_file(script, &info), 0);
        int kmb = 0;
        T_ASSERT_EQ_INT(ldscript_configure_link(&info, &kmb), 0);
        T_ASSERT_EQ_INT(kmb, 0);
        T_ASSERT_EQ_INT((int)(link_image_base() == 0x600000ULL), 1);

        /* Minimal ET_EXEC via emit path is heavier; rely on CLI when present.
         * Without CLI, still prove configure sets base + PHDR flags accepted. */
        T_ASSERT_EQ_INT(info.text_phdr_flags, 5);
        T_ASSERT_EQ_INT(info.data_phdr_flags, 6);
        link_set_image_base(0);
        unlink(script);
        unlink(src);
        if (!fcc)
            printf("  NOTE test_T_user_image_base_e2e: no fakecc binary; configure only\n");
        return;
    }

    size_t n = 0;
    unsigned char *b = slurp(out, &n);
    T_ASSERT(b != NULL);
    if (!b) { unlink(script); unlink(src); return; }
    T_ASSERT(b[0] == 0x7f && b[1] == 'E' && b[2] == 'L' && b[3] == 'F');
    T_ASSERT_EQ_INT((int)b[4], 2); /* ELFCLASS64 */
    uint64_t phoff = rd64(b + 32);
    uint16_t phentsize = rd16(b + 54);
    uint16_t phnum = rd16(b + 56);
    T_ASSERT(phnum >= 2);
    int found_rx = 0, found_rw = 0;
    uint64_t first_load_vaddr = 0;
    int saw_load = 0;
    for (uint16_t i = 0; i < phnum; i++) {
        const unsigned char *ph = b + phoff + (size_t)i * phentsize;
        uint32_t type = rd32(ph + 0);
        if (type != 1) continue; /* PT_LOAD */
        uint32_t flags = rd32(ph + 4);
        uint64_t vaddr = rd64(ph + 16);
        if (!saw_load) {
            first_load_vaddr = vaddr;
            saw_load = 1;
        }
        if ((flags & 5) == 5 && !(flags & 2)) found_rx = 1;
        if ((flags & 6) == 6) found_rw = 1;
        (void)vaddr;
    }
    T_ASSERT(saw_load);
    T_ASSERT_EQ_INT((int)(first_load_vaddr == 0x600000ULL), 1);
    T_ASSERT(found_rx);
    T_ASSERT(found_rw);
    free(b);
    unlink(out);
    unlink(script);
    unlink(src);
#endif
}

int main(void) {
    target_set_current(target_x86_64_linux());
    test_parse_kernel_script();
    test_parse_user_script();
    test_parse_fakeos_files_if_present();
    test_T_kernel_multiboot_e2e();
    test_T_user_image_base_e2e();
    return t_finalize();
}
