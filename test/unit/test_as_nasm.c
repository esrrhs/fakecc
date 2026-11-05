#include "fakecc/as_nasm.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#ifndef unlink
#define unlink _unlink
#endif
#endif

static int read_all(const char *path, unsigned char **out, size_t *n) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return -1; }
    *out = (unsigned char *)malloc((size_t)sz);
    *n = (size_t)sz;
    if (fread(*out, 1, *n, f) != *n) { free(*out); fclose(f); return -1; }
    fclose(f);
    return 0;
}

static void test_ap_tramp_snippet(void) {
    static const char src[] =
        "ORG 0x8000\n"
        "BITS 16\n"
        "cli\n"
        "cld\n"
        "xor ax, ax\n"
        "mov ds, ax\n"
        "mov sp, 0x7C00\n"
        "BITS 32\n"
        "mov ax, 0x10\n"
        "mov ds, ax\n"
        "BITS 64\n"
        "mov rax, 0\n"
        "jmp rax\n";
    const char *path = "test_as_nasm_snip.bin";
    T_ASSERT_EQ_INT(assemble_nasm(src, "snip.asm", NASM_FMT_BIN, path), 0);
    unsigned char *buf = NULL; size_t n = 0;
    T_ASSERT_EQ_INT(read_all(path, &buf, &n), 0);
    T_ASSERT(n >= 2);
    T_ASSERT_EQ_INT(buf[0], 0xFA); /* cli */
    T_ASSERT_EQ_INT(buf[1], 0xFC); /* cld */
    free(buf);
    unlink(path);
}

static void test_times_dollar(void) {
    /* Mirrors ap_tramp pad: times (N - ($ - $$)) db 0 */
    static const char src[] =
        "ORG 0\n"
        "BITS 16\n"
        "cli\n"
        "cld\n"
        "times (16 - ($ - $$)) db 0x90\n";
    const char *path = "test_as_nasm_times.bin";
    T_ASSERT_EQ_INT(assemble_nasm(src, "times.asm", NASM_FMT_BIN, path), 0);
    unsigned char *buf = NULL; size_t n = 0;
    T_ASSERT_EQ_INT(read_all(path, &buf, &n), 0);
    T_ASSERT_EQ_INT((int)n, 16);
    T_ASSERT_EQ_INT(buf[0], 0xFA);
    T_ASSERT_EQ_INT(buf[1], 0xFC);
    T_ASSERT_EQ_INT(buf[15], 0x90);
    free(buf);
    unlink(path);
}

static void test_times_cap(void) {
    /* Must reject absurd times rather than OOM / hang the runner. */
    static const char src[] = "ORG 0\nBITS 16\ntimes 0x7fffffff db 0\n";
    T_ASSERT(assemble_nasm(src, "huge.asm", NASM_FMT_BIN, "test_as_nasm_huge.bin") != 0);
    unlink("test_as_nasm_huge.bin");
}

static void test_is_nasm_input(void) {
    T_ASSERT(is_nasm_input("boot.asm"));
    T_ASSERT(is_nasm_input("x.nasm"));
    T_ASSERT(!is_nasm_input("boot.S"));
    T_ASSERT(!is_nasm_input("boot.c"));
}

int main(void) {
    test_is_nasm_input();
    test_ap_tramp_snippet();
    test_times_dollar();
    test_times_cap();
    printf("test_as_nasm: PASS\n");
    return 0;
}
