/* PE archives, import libraries, DLL export, and unwind directory.
 * Shells out to fakecc plus the installed MinGW toolchain. CTest runs this
 * with cwd build-win/test, so fakecc is ../fakecc.exe. */
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p, 0755)
#endif

#define WORK "pe-lib-work"

/* Filled in main from the well-known MSYS prefix or from PATH.  GitHub's
 * setup-msys2 action does not install under C:\msys64. */
static char g_gcc[512];
static char g_ar[512];
static char g_mingw_lib[512];
#define GCC g_gcc
#define AR g_ar
#define MINGW_LIB g_mingw_lib

static char g_cc[512];

static int run(const char *cmd) {
    return system(cmd);
}

static int run_ok(const char *cmd) {
    int rc = run(cmd);
    if (rc != 0)
        fprintf(stderr, "  command failed (%d): %s\n", rc, cmd);
    return rc == 0;
}

static int exit_is(int rc, int want) {
    return rc == want || rc == (want << 8);
}

static int write_file(const char *path, const char *text) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return -1;
    size_t n = strlen(text);
    int ok = fwrite(text, 1, n, fp) == n;
    fclose(fp);
    return ok ? 0 : -1;
}

static unsigned char *read_all(const char *path, size_t *n) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return NULL; }
    long sz = ftell(fp);
    if (sz < 0) { fclose(fp); return NULL; }
    rewind(fp);
    unsigned char *b = malloc((size_t)sz + 1);
    if (!b) { fclose(fp); return NULL; }
    if (fread(b, 1, (size_t)sz, fp) != (size_t)sz) {
        free(b); fclose(fp); return NULL;
    }
    fclose(fp);
    b[sz] = 0;
    *n = (size_t)sz;
    return b;
}

static uint16_t ru16(const unsigned char *p) {
    uint16_t v; memcpy(&v, p, 2); return v;
}
static uint32_t ru32(const unsigned char *p) {
    uint32_t v; memcpy(&v, p, 4); return v;
}

static uint64_t ru64(const unsigned char *p) {
    uint64_t v; memcpy(&v, p, 8); return v;
}

typedef struct {
    char name[9];
    uint32_t vsize, va, rawsz, rawptr;
} Sec;

static int load_pe(const unsigned char *b, size_t n, uint32_t *opt,
                   Sec *sec, int *nsec, uint32_t dir[16][2]) {
    if (n < 0x40 || b[0] != 'M' || b[1] != 'Z') return -1;
    uint32_t lfa = ru32(b + 0x3C);
    if ((size_t)lfa + 24 + 240 > n) return -1;
    if (memcmp(b + lfa, "PE\0\0", 4) != 0) return -1;
    if (ru16(b + lfa + 4) != 0x8664) return -1;
    *nsec = ru16(b + lfa + 6);
    if (*nsec < 1 || *nsec > 32) return -1;
    uint16_t optsz = ru16(b + lfa + 20);
    *opt = lfa + 24;
    if ((size_t)*opt + optsz > n) return -1;
    if (ru16(b + *opt) != 0x20B) return -1;
    const unsigned char *dd = b + *opt + 112;
    for (int i = 0; i < 16; i++) {
        dir[i][0] = ru32(dd + (size_t)i * 8);
        dir[i][1] = ru32(dd + (size_t)i * 8 + 4);
    }
    const unsigned char *st = b + *opt + optsz;
    for (int i = 0; i < *nsec; i++) {
        const unsigned char *s = st + (size_t)i * 40;
        if (s + 40 > b + n) return -1;
        memset(sec[i].name, 0, 9);
        memcpy(sec[i].name, s, 8);
        sec[i].vsize = ru32(s + 8);
        sec[i].va = ru32(s + 12);
        sec[i].rawsz = ru32(s + 16);
        sec[i].rawptr = ru32(s + 20);
    }
    return 0;
}

static int rva_off(const Sec *sec, int nsec, uint32_t rva, uint32_t *off) {
    for (int i = 0; i < nsec; i++) {
        uint32_t span = sec[i].vsize > sec[i].rawsz ? sec[i].vsize : sec[i].rawsz;
        if (rva >= sec[i].va && rva < sec[i].va + span) {
            *off = sec[i].rawptr + (rva - sec[i].va);
            return 0;
        }
    }
    return -1;
}

static int reloc_has_rva(const unsigned char *b, size_t n, const Sec *sec,
                         int nsec, uint32_t dir_rva, uint32_t dir_sz,
                         uint32_t target_rva) {
    uint32_t off = 0;
    if (rva_off(sec, nsec, dir_rva, &off) != 0) return 0;
    uint32_t cur = 0;
    while (cur + 8 <= dir_sz) {
        if (off + cur + 8 > n) break;
        uint32_t block_va = ru32(b + off + cur);
        uint32_t block_sz = ru32(b + off + cur + 4);
        if (block_sz < 8 || cur + block_sz > dir_sz) break;
        uint32_t num_entries = (block_sz - 8) / 2;
        for (uint32_t i = 0; i < num_entries; i++) {
            if (off + cur + 8 + (i + 1) * 2 > n) break;
            uint16_t entry = ru16(b + off + cur + 8 + i * 2);
            int type = entry >> 12;
            int r_offset = entry & 0xFFF;
            if (type == 10 /* IMAGE_REL_BASED_DIR64 */) {
                if (block_va + (uint32_t)r_offset == target_rva) return 1;
            }
        }
        cur += block_sz;
    }
    return 0;
}

static int reloc_has_rva_in_range(const unsigned char *b, size_t n, const Sec *sec,
                                  int nsec, uint32_t dir_rva, uint32_t dir_sz,
                                  uint32_t range_start, uint32_t range_len) {
    uint32_t off = 0;
    if (rva_off(sec, nsec, dir_rva, &off) != 0) return 0;
    uint32_t cur = 0;
    while (cur + 8 <= dir_sz) {
        if (off + cur + 8 > n) break;
        uint32_t block_va = ru32(b + off + cur);
        uint32_t block_sz = ru32(b + off + cur + 4);
        if (block_sz < 8 || cur + block_sz > dir_sz) break;
        uint32_t num_entries = (block_sz - 8) / 2;
        for (uint32_t i = 0; i < num_entries; i++) {
            if (off + cur + 8 + (i + 1) * 2 > n) break;
            uint16_t entry = ru16(b + off + cur + 8 + i * 2);
            int type = entry >> 12;
            int r_offset = entry & 0xFFF;
            if (type == 10 /* IMAGE_REL_BASED_DIR64 */) {
                uint32_t rva = block_va + (uint32_t)r_offset;
                if (rva >= range_start && rva < range_start + range_len) return 1;
            }
        }
        cur += block_sz;
    }
    return 0;
}

static int ieq(const char *a, const char *b) {
    while (*a && *b) {
        unsigned char ca = (unsigned char)*a, cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int pe_import_has(const unsigned char *b, size_t n, const Sec *sec,
                         int nsec, uint32_t dir_rva, const char *dll) {
    uint32_t off = 0;
    if (rva_off(sec, nsec, dir_rva, &off) != 0) return 0;
    for (int i = 0; i < 64; i++) {
        if ((size_t)off + 20 > n) return 0;
        uint32_t name_rva = ru32(b + off + 12);
        uint32_t ft = ru32(b + off + 16);
        if (!name_rva && !ft) return 0;
        uint32_t noff = 0;
        if (name_rva && rva_off(sec, nsec, name_rva, &noff) == 0 && noff < n) {
            if (ieq((const char *)b + noff, dll)) return 1;
        }
        off += 20;
    }
    return 0;
}

static int pe_exports_have(const unsigned char *b, size_t n, const Sec *sec,
                           int nsec, uint32_t dir_rva, const char *sym) {
    uint32_t off = 0;
    if (rva_off(sec, nsec, dir_rva, &off) != 0 || off + 40 > n) return 0;
    uint32_t nnames = ru32(b + off + 24);
    uint32_t names_rva = ru32(b + off + 32);
    uint32_t noff = 0;
    if (rva_off(sec, nsec, names_rva, &noff) != 0) return 0;
    for (uint32_t i = 0; i < nnames && i < 4096; i++) {
        if (noff + i * 4 + 4 > n) return 0;
        uint32_t sr = ru32(b + noff + i * 4);
        uint32_t so = 0;
        if (rva_off(sec, nsec, sr, &so) != 0 || so >= n) continue;
        if (strcmp((const char *)b + so, sym) == 0) return 1;
    }
    return 0;
}

static int pdata_ok(const unsigned char *b, size_t n, const Sec *sec, int nsec,
                    uint32_t rva, uint32_t sz) {
    if (!sz || (sz % 12) != 0) return 0;
    uint32_t text_va = 0, text_vs = 0;
    int found = 0;
    for (int i = 0; i < nsec; i++)
        if (strcmp(sec[i].name, ".text") == 0) {
            text_va = sec[i].va;
            text_vs = sec[i].vsize;
            found = 1;
        }
    if (!found || !text_vs) return 0;
    uint32_t off = 0;
    if (rva_off(sec, nsec, rva, &off) != 0) return 0;
    int recs = 0;
    for (uint32_t i = 0; i + 12 <= sz; i += 12) {
        if (off + i + 12 > n) return 0;
        uint32_t begin = ru32(b + off + i);
        uint32_t end = ru32(b + off + i + 4);
        if (begin >= end) return 0;
        if (begin < text_va || end > text_va + text_vs) return 0;
        recs++;
    }
    return recs > 0;
}

static void test_static_archive(void) {
    T_ASSERT(write_file(WORK "/used.c",
        "package main;\nint used(void) { return 42; }\n") == 0);
    T_ASSERT(write_file(WORK "/unused.c",
        "package main;\nint missing(void);\n"
        "int unused(void) { return missing(); }\n") == 0);
    T_ASSERT(write_file(WORK "/main.c",
        "package main;\nint used(void);\n"
        "int main(void) { return used(); }\n") == 0);
    char cmd[1024];
    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -c %s\\used.c -o %s\\used.obj",
             g_cc, WORK, WORK);
    T_ASSERT(run_ok(cmd));
    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -c %s\\unused.c -o %s\\unused.obj",
             g_cc, WORK, WORK);
    T_ASSERT(run_ok(cmd));
    snprintf(cmd, sizeof cmd,
             "%s rcs %s\\libx.a %s\\used.obj %s\\unused.obj",
             AR, WORK, WORK, WORK);
    T_ASSERT(run_ok(cmd));
    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -nostdlib %s\\main.c -L%s -lx "
             "-o %s\\arch.exe",
             g_cc, WORK, WORK, WORK);
    T_ASSERT(run_ok(cmd));
    snprintf(cmd, sizeof cmd, "cd %s && arch.exe", WORK);
    T_ASSERT(exit_is(run(cmd), 42));
}

static void test_consume_mingw_dll(void) {
    T_ASSERT(write_file(WORK "/add.c",
        "__declspec(dllexport) int add(int a, int b) { return a + b; }\n"
        "__declspec(dllexport) int g_value = 41;\n") == 0);
    T_ASSERT(write_file(WORK "/use_add.c",
        "package main;\n"
        "int add(int a, int b);\n"
        "extern int g_value;\n"
        "int main(void) { return add(g_value, 1); }\n") == 0);
    char cmd[1200];
    snprintf(cmd, sizeof cmd,
             "%s -shared -o %s\\add.dll %s\\add.c -Wl,--out-implib,%s\\libadd.dll.a",
             GCC, WORK, WORK, WORK);
    T_ASSERT(run_ok(cmd));
    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -nostdlib %s\\use_add.c "
             "-L%s -ladd -o %s\\use_add.exe",
             g_cc, WORK, WORK, WORK);
    T_ASSERT(run_ok(cmd));
    snprintf(cmd, sizeof cmd, "cd %s && use_add.exe", WORK);
    T_ASSERT(exit_is(run(cmd), 42));
}

static void test_fakecc_dll(void) {
    T_ASSERT(write_file(WORK "/foo.c",
        "package main;\n"
        "int g_value = 41;\n"
        "int g_flag = 0;\n"
        "int *g_ptr = &g_value;\n"
        "int DllMain(int h, int reason, int reserved) {\n"
        "    g_flag = (reason == 1) ? 7 : g_flag;\n"
        "    return (h == reserved) ? 1 : 1;\n"
        "}\n"
        "int add(int a, int b) { return a + b; }\n") == 0);
    T_ASSERT(write_file(WORK "/use_foo.c",
        "package main;\n"
        "int add(int a, int b);\n"
        "extern int g_value;\n"
        "extern int g_flag;\n"
        "int main(void) {\n"
        "    if (g_flag != 7) return 10;\n"
        "    return add(g_value, 1);\n"
        "}\n") == 0);
    T_ASSERT(write_file(WORK "/gcc_use.c",
        "__declspec(dllimport) int add(int, int);\n"
        "__declspec(dllimport) int g_value;\n"
        "__declspec(dllimport) int g_flag;\n"
        "int main(void) {\n"
        "    if (g_flag != 7) return 10;\n"
        "    if (add(g_value, 1) != 42) return 11;\n"
        "    return 42;\n"
        "}\n") == 0);
    char cmd[1200];
    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -shared -nostdlib %s\\foo.c "
             "-o %s\\foo.dll",
             g_cc, WORK, WORK);
    T_ASSERT(run_ok(cmd));
    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -nostdlib %s\\use_foo.c "
             "-L%s -lfoo -o %s\\use_foo.exe",
             g_cc, WORK, WORK, WORK);
    T_ASSERT(run_ok(cmd));
    snprintf(cmd, sizeof cmd, "cd %s && use_foo.exe", WORK);
    T_ASSERT(exit_is(run(cmd), 42));
    snprintf(cmd, sizeof cmd,
             "%s -o %s\\gcc_use.exe %s\\gcc_use.c -L%s -lfoo",
             GCC, WORK, WORK, WORK);
    T_ASSERT(run_ok(cmd));
    snprintf(cmd, sizeof cmd, "cd %s && gcc_use.exe", WORK);
    T_ASSERT(exit_is(run(cmd), 42));
}

static void test_fakecc_dll_tls(void) {
    T_ASSERT(write_file(WORK "/tls_dll.c",
        "package main;\n"
        "int dummy_target = 777;\n"
        "__thread int dll_tls_val = 1000;\n"
        "__thread int dll_bss;\n"
        "__thread int *dll_ptr = &dummy_target;\n"
        "int add_dll(int x) {\n"
        "    dll_tls_val += x;\n"
        "    dll_bss = dll_tls_val * 2;\n"
        "    return dll_tls_val;\n"
        "}\n"
        "int get_dll_bss(void) { return dll_bss; }\n"
        "int get_dll_ptr_val(void) { return *dll_ptr; }\n") == 0);
    T_ASSERT(write_file(WORK "/tls_main.c",
        "package main;\n"
        "int add_dll(int x);\n"
        "int get_dll_bss(void);\n"
        "int get_dll_ptr_val(void);\n"
        "int exe_target = 888;\n"
        "__thread int exe_tls_val = 2000;\n"
        "__thread int exe_bss;\n"
        "__thread int *exe_ptr = &exe_target;\n"
        "int add_exe(int x) {\n"
        "    exe_tls_val += x;\n"
        "    exe_bss = exe_tls_val * 3;\n"
        "    return exe_tls_val;\n"
        "}\n"
        "int main(void) {\n"
        "    int d1 = add_dll(5);    /* 1005 */\n"
        "    int e1 = add_exe(10);   /* 2010 */\n"
        "    int d2 = add_dll(15);   /* 1020 */\n"
        "    int e2 = add_exe(20);   /* 2030 */\n"
        "    if (d1 != 1005) return 1;\n"
        "    if (e1 != 2010) return 2;\n"
        "    if (d2 != 1020) return 3;\n"
        "    if (e2 != 2030) return 4;\n"
        "    if (get_dll_bss() != 2040) return 5;\n"
        "    if (exe_bss != 6090) return 6;\n"
        "    if (get_dll_ptr_val() != 777) return 7;\n"
        "    if (*exe_ptr != 888) return 8;\n"
        "    return 0;\n"
        "}\n") == 0);

    char cmd[1200];
    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -shared -nostdlib %s\\tls_dll.c "
             "-o %s\\tls_dll.dll",
             g_cc, WORK, WORK);
    T_ASSERT(run_ok(cmd));

    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -nostdlib %s\\tls_main.c "
             "-L%s -ltls_dll -o %s\\tls_main.exe",
             g_cc, WORK, WORK, WORK);
    T_ASSERT(run_ok(cmd));

    snprintf(cmd, sizeof cmd, "cd %s && tls_main.exe", WORK);
    T_ASSERT(exit_is(run(cmd), 0));

    size_t n = 0;
    unsigned char *b = read_all(WORK "/tls_dll.dll", &n);
    T_ASSERT(b != NULL);
    if (b) {
        uint32_t opt = 0, dir[16][2];
        Sec sec[32];
        int nsec = 0;
        T_ASSERT(load_pe(b, n, &opt, sec, &nsec, dir) == 0);
        T_ASSERT(dir[9][0] != 0 && dir[9][1] == 40);
        free(b);
    }

    b = read_all(WORK "/tls_main.exe", &n);
    T_ASSERT(b != NULL);
    if (b) {
        uint32_t opt = 0, dir[16][2];
        Sec sec[32];
        int nsec = 0;
        T_ASSERT(load_pe(b, n, &opt, sec, &nsec, dir) == 0);
        T_ASSERT(dir[9][0] != 0 && dir[9][1] == 40);
        free(b);
    }
}

static void test_tls_features_e2e(void) {
    /* Multi-TU test covering:
     * - initialized TLS (__thread int, long long)
     * - uninitialized TLS (__thread int, char buf[64])
     * - aligned TLS (__thread __attribute__((aligned(64))))
     * - pointer TLS (__thread int *p_val, __thread const char *p_msg)
     * - cross-TU access
     */
    T_ASSERT(write_file(WORK "/feat_def.c",
        "package main;\n"
        "int target_val = 88;\n"
        "__thread int tls_a = 123;\n"
        "__thread long long tls_b = 0x1122334455667788LL;\n"
        "__thread int tls_bss;\n"
        "__thread char tls_buf[64];\n"
        "__thread int *tls_ptr = &target_val;\n"
        "__thread const char *tls_msg = \"tls_ok\";\n"
        "__thread int tls_aligned __attribute__((aligned(64))) = 77;\n"
        "\n"
        "int feat_def_check(void) {\n"
        "    if (tls_a != 123) return 1;\n"
        "    if (tls_b != 0x1122334455667788LL) return 2;\n"
        "    if (*tls_ptr != 88) return 3;\n"
        "    if (tls_msg[0] != 't' || tls_msg[1] != 'l') return 4;\n"
        "    if (tls_aligned != 77) return 5;\n"
        "    if (((unsigned long long)&tls_aligned) & 63ULL) return 6;\n"
        "    tls_a += 1;\n"
        "    tls_bss = 500;\n"
        "    tls_buf[0] = 'A';\n"
        "    return 0;\n"
        "}\n") == 0);

    T_ASSERT(write_file(WORK "/feat_main.c",
        "package main;\n"
        "extern int target_val;\n"
        "extern __thread int tls_a;\n"
        "extern __thread long long tls_b;\n"
        "extern __thread int tls_bss;\n"
        "extern __thread char tls_buf[64];\n"
        "extern __thread int *tls_ptr;\n"
        "extern __thread const char *tls_msg;\n"
        "extern __thread int tls_aligned __attribute__((aligned(64)));\n"
        "int feat_def_check(void);\n"
        "\n"
        "int main(void) {\n"
        "    int rc = feat_def_check();\n"
        "    if (rc != 0) return rc;\n"
        "    if (tls_a != 124) return 10;\n"
        "    if (tls_bss != 500) return 11;\n"
        "    if (tls_buf[0] != 'A') return 12;\n"
        "    if (*tls_ptr != 88) return 13;\n"
        "    if (((unsigned long long)&tls_aligned) & 63ULL) return 14;\n"
        "    return 0;\n"
        "}\n") == 0);

    char cmd[1200];
    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -c %s\\feat_def.c -o %s\\feat_def.obj",
             g_cc, WORK, WORK);
    T_ASSERT(run_ok(cmd));

    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -c %s\\feat_main.c -o %s\\feat_main.obj",
             g_cc, WORK, WORK);
    T_ASSERT(run_ok(cmd));

    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -nostdlib %s\\feat_def.obj %s\\feat_main.obj -o %s\\feat_test.exe",
             g_cc, WORK, WORK, WORK);
    T_ASSERT(run_ok(cmd));

    snprintf(cmd, sizeof cmd, "cd %s && feat_test.exe", WORK);
    T_ASSERT(exit_is(run(cmd), 0));

    /* PE directory inspection */
    size_t n = 0;
    unsigned char *b = read_all(WORK "/feat_test.exe", &n);
    T_ASSERT(b != NULL);
    if (b) {
        uint32_t opt = 0, dir[16][2];
        Sec sec[32];
        int nsec = 0;
        T_ASSERT(load_pe(b, n, &opt, sec, &nsec, dir) == 0);
        /* Entry 9 is IMAGE_DIRECTORY_ENTRY_TLS */
        T_ASSERT(dir[9][0] != 0 && dir[9][1] == 40);

        uint32_t tls_dir_off = 0;
        T_ASSERT(rva_off(sec, nsec, dir[9][0], &tls_dir_off) == 0);
        const unsigned char *td = b + tls_dir_off;
        uint64_t start_raw = ru64(td + 0);
        uint64_t end_raw   = ru64(td + 8);
        uint64_t addr_idx  = ru64(td + 16);
        uint64_t addr_cb   = ru64(td + 24);
        uint32_t zero_fill = ru32(td + 32);
        uint32_t chars     = ru32(td + 36);

        uint64_t image_base = ru64(b + opt + 24);
        T_ASSERT(start_raw >= image_base);
        T_ASSERT(end_raw > start_raw);
        T_ASSERT(addr_idx >= image_base);
        T_ASSERT(zero_fill >= 68);
        /* 64-byte alignment in Characteristics: bits 20..23 = 7 */
        T_ASSERT((chars & 0x00F00000) >= 0x00700000);

        /* Check base relocations in .reloc */
        T_ASSERT(dir[5][0] != 0 && dir[5][1] != 0);
        /* Base relocations for TLS directory pointers */
        T_ASSERT(reloc_has_rva(b, n, sec, nsec, dir[5][0], dir[5][1], dir[9][0] + 0));
        T_ASSERT(reloc_has_rva(b, n, sec, nsec, dir[5][0], dir[5][1], dir[9][0] + 8));
        T_ASSERT(reloc_has_rva(b, n, sec, nsec, dir[5][0], dir[5][1], dir[9][0] + 16));
        if (addr_cb)
            T_ASSERT(reloc_has_rva(b, n, sec, nsec, dir[5][0], dir[5][1], dir[9][0] + 24));

        /* Pointer in .tdata must also have a base relocation */
        uint32_t tdata_rva = (uint32_t)(start_raw - image_base);
        T_ASSERT(reloc_has_rva_in_range(b, n, sec, nsec, dir[5][0], dir[5][1],
                                        tdata_rva, (uint32_t)(end_raw - start_raw)));

        free(b);
    }
}

static void test_tls_multithread_isolation(void) {
    /* Multi-thread isolation test using CreateThread:
     * 1. Direct fakecc EXE: spawns 4 worker threads, verifies per-thread TLS
     *    isolation, ensures worker mutations do not affect main thread.
     * 2. Host MinGW executable calling fakecc-produced DLL: verifies foreign
     *    threads each get their own DLL TLS instance.
     */
    T_ASSERT(write_file(WORK "/thread_main.c",
        "package main;\n"
        "void* CreateThread(void* sa, unsigned long stack, void* start, void* param, unsigned long flags, unsigned long* id);\n"
        "unsigned long WaitForSingleObject(void* handle, unsigned long ms);\n"
        "int CloseHandle(void* handle);\n"
        "\n"
        "__thread int t_var = 100;\n"
        "__thread long long t_cnt = 500;\n"
        "\n"
        "int worker(void* arg) {\n"
        "    long long id = (long long)arg;\n"
        "    if (t_var != 100) return 1;\n"
        "    if (t_cnt != 500) return 2;\n"
        "    t_var = (int)(id * 10);\n"
        "    t_cnt = 500 + id;\n"
        "    for (int i = 0; i < 10000; i++) { /* busy work */ }\n"
        "    if (t_var != (int)(id * 10)) return 3;\n"
        "    if (t_cnt != 500 + id) return 4;\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "int main(void) {\n"
        "    if (t_var != 100 || t_cnt != 500) return 10;\n"
        "    void* th[4];\n"
        "    for (int i = 0; i < 4; i++) {\n"
        "        th[i] = CreateThread(0, 0, (void*)worker, (void*)(long long)(i + 1), 0, 0);\n"
        "        if (!th[i]) return 11;\n"
        "    }\n"
        "    for (int i = 0; i < 4; i++) {\n"
        "        WaitForSingleObject(th[i], 0xFFFFFFFF);\n"
        "        CloseHandle(th[i]);\n"
        "    }\n"
        "    if (t_var != 100) return 12;\n"
        "    if (t_cnt != 500) return 13;\n"
        "    return 0;\n"
        "}\n") == 0);

    char cmd[1200];
    snprintf(cmd, sizeof cmd,
             "%s --target=x86_64-windows -nostdlib -L%s -lkernel32 %s\\thread_main.c -o %s\\thread_main.exe",
             g_cc, MINGW_LIB, WORK, WORK);
    T_ASSERT(run_ok(cmd));

    snprintf(cmd, sizeof cmd, "cd %s && thread_main.exe", WORK);
    T_ASSERT(exit_is(run(cmd), 0));

    /* Host MinGW executable calling fakecc DLL across threads */
    T_ASSERT(write_file(WORK "/host_thread.c",
        "#include <windows.h>\n"
        "#include <stdio.h>\n"
        "__declspec(dllimport) int add_dll(int x);\n"
        "DWORD WINAPI worker(LPVOID arg) {\n"
        "    int id = (int)(intptr_t)arg;\n"
        "    int v1 = add_dll(id * 10);\n"
        "    if (v1 != 1000 + id * 10) return 1;\n"
        "    Sleep(10);\n"
        "    int v2 = add_dll(5);\n"
        "    if (v2 != 1000 + id * 10 + 5) return 2;\n"
        "    return 0;\n"
        "}\n"
        "int main(void) {\n"
        "    HANDLE th[4];\n"
        "    for (int i = 0; i < 4; i++) {\n"
        "        th[i] = CreateThread(NULL, 0, worker, (LPVOID)(intptr_t)(i + 1), 0, NULL);\n"
        "        if (!th[i]) return 10;\n"
        "    }\n"
        "    int ok = 1;\n"
        "    for (int i = 0; i < 4; i++) {\n"
        "        WaitForSingleObject(th[i], INFINITE);\n"
        "        DWORD code = 0;\n"
        "        GetExitCodeThread(th[i], &code);\n"
        "        if (code != 0) ok = 0;\n"
        "        CloseHandle(th[i]);\n"
        "    }\n"
        "    return ok ? 0 : 20;\n"
        "}\n") == 0);

    snprintf(cmd, sizeof cmd,
             "%s -o %s\\host_thread.exe %s\\host_thread.c -L%s -ltls_dll",
             GCC, WORK, WORK, WORK);
    T_ASSERT(run_ok(cmd));

    snprintf(cmd, sizeof cmd, "cd %s && host_thread.exe", WORK);
    T_ASSERT(exit_is(run(cmd), 0));
}

static void test_pe_headers(void) {
    size_t n = 0;
    unsigned char *b = read_all(WORK "/foo.dll", &n);
    T_ASSERT(b != NULL);
    if (!b) return;
    uint32_t opt = 0, dir[16][2];
    Sec sec[32];
    int nsec = 0;
    T_ASSERT(load_pe(b, n, &opt, sec, &nsec, dir) == 0);
    uint16_t chars = 0;
    /* characteristics sit 18 bytes into the COFF header, which is 24
     * bytes before the optional header. */
    /* COFF characteristics are 22 bytes after the PE signature, two
     * bytes before the optional header. */
    if (opt >= 2) chars = ru16(b + opt - 2);
    T_ASSERT((chars & 0x2000) != 0);
    T_ASSERT(dir[0][0] != 0 && dir[0][1] != 0);
    T_ASSERT(pe_exports_have(b, n, sec, nsec, dir[0][0], "add"));
    T_ASSERT(pe_exports_have(b, n, sec, nsec, dir[0][0], "g_value"));
    T_ASSERT(pdata_ok(b, n, sec, nsec, dir[3][0], dir[3][1]));
    T_ASSERT(dir[5][0] != 0 && dir[5][1] != 0);
    free(b);

    b = read_all(WORK "/use_foo.exe", &n);
    T_ASSERT(b != NULL);
    if (!b) return;
    T_ASSERT(load_pe(b, n, &opt, sec, &nsec, dir) == 0);
    T_ASSERT(dir[1][0] != 0 && dir[1][1] != 0);
    T_ASSERT(pe_import_has(b, n, sec, nsec, dir[1][0], "KERNEL32.dll"));
    T_ASSERT(pe_import_has(b, n, sec, nsec, dir[1][0], "foo.dll"));
    free(b);
}

static void test_native_windows_threads(void) {
    char cmd[1024];
    T_ASSERT(write_file(WORK "/win_threads_e2e.c",
        "package main;\n"
        "import runtime;\n"
        "__thread int my_tls = 100;\n"
        "void *worker_ret(void *arg) {\n"
        "    long val = (long)arg;\n"
        "    my_tls += (int)val;\n"
        "    runtime.pthread_t s = runtime.pthread_self();\n"
        "    if (s == 0) runtime.pthread_exit((void *)999);\n"
        "    return (void *)(long)my_tls;\n"
        "}\n"
        "void *worker_exit(void *arg) {\n"
        "    runtime.pthread_exit((void *)77);\n"
        "    return (void *)88;\n"
        "}\n"
        "int main(void) {\n"
        "    runtime.pthread_t main_t = runtime.pthread_self();\n"
        "    if (main_t == 0) return 1;\n"
        "    if (runtime.pthread_join(main_t, 0) == 0) return 2;\n"
        "    if (runtime.pthread_join(0, 0) == 0) return 3;\n"
        "    if (runtime.pthread_join((runtime.pthread_t)0xdeadbeef, 0) == 0) return 4;\n"
        "    runtime.pthread_t t1, t2;\n"
        "    void *r1 = 0, *r2 = 0;\n"
        "    runtime.pthread_attr_t attr;\n"
        "    attr.__detachstate = 0;\n"
        "    attr.__stacksize = 65536;\n"
        "    if (runtime.pthread_create(&t1, &attr, worker_ret, (void *)50) != 0) return 5;\n"
        "    if (runtime.pthread_create(&t2, 0, worker_exit, 0) != 0) return 6;\n"
        "    if (runtime.pthread_join(t1, &r1) != 0) return 7;\n"
        "    if ((long)r1 != 150) return 8;\n"
        "    if (runtime.pthread_join(t1, &r1) == 0) return 9;\n"
        "    if (runtime.pthread_join(t2, &r2) != 0) return 10;\n"
        "    if ((long)r2 != 77) return 11;\n"
        "    if (my_tls != 100) return 12;\n"
        "    runtime.thread_t t3;\n"
        "    void *r3 = 0;\n"
        "    if (runtime.thread_create(&t3, worker_ret, (void *)20) != 0) return 13;\n"
        "    if (runtime.thread_join(t3, &r3) != 0) return 14;\n"
        "    if ((long)r3 != 120) return 15;\n"
        "    return 0;\n"
        "}\n") == 0);

    snprintf(cmd, sizeof cmd,
             "%s -target x86_64-windows -o %s\\win_threads_e2e.exe %s\\win_threads_e2e.c",
             g_cc, WORK, WORK);
    T_ASSERT(run_ok(cmd));

    snprintf(cmd, sizeof cmd, "cd %s && win_threads_e2e.exe", WORK);
    T_ASSERT(exit_is(run(cmd), 0));
}

static int file_ok(const char *p) {
    FILE *fp = fopen(p, "rb");
    if (!fp) return 0;
    fclose(fp);
    return 1;
}

static int search_path(char *out, size_t n, const char *exe) {
    const char *path = getenv("PATH");
    const char *p;
    if (!path) return 0;
    p = path;
    while (*p) {
        const char *semi = strchr(p, ';');
        size_t len = semi ? (size_t)(semi - p) : strlen(p);
        if (len > 0 && len + 1 + strlen(exe) < n) {
            memcpy(out, p, len);
            if (out[len - 1] != '\\' && out[len - 1] != '/') out[len++] = '\\';
            memcpy(out + len, exe, strlen(exe) + 1);
            if (file_ok(out)) return 1;
        }
        if (!semi) break;
        p = semi + 1;
    }
    out[0] = 0;
    return 0;
}

static void parent_lib(char *out, size_t n, const char *exe) {
    char buf[512];
    char *s;
    snprintf(buf, sizeof buf, "%s", exe);
    s = strrchr(buf, '\\');
    if (!s) s = strrchr(buf, '/');
    if (s) *s = 0;
    s = strrchr(buf, '\\');
    if (!s) s = strrchr(buf, '/');
    if (s) *s = 0;
    {
        size_t len = strlen(buf);
        if (len + 5 > n) {
            out[0] = 0;
            return;
        }
        memcpy(out, buf, len);
        memcpy(out + len, "\\lib", 5);
    }
}

int main(void) {
    const char *cands[] = {
        "..\\fakecc.exe", "..\\fakecc", ".\\fakecc.exe",
        "build-win\\fakecc.exe", ".\\build-win\\fakecc.exe", NULL
    };
    g_cc[0] = 0;
    for (int i = 0; cands[i]; i++) {
        FILE *fp = fopen(cands[i], "rb");
        if (fp) { fclose(fp); snprintf(g_cc, sizeof g_cc, "%s", cands[i]); break; }
    }
    T_ASSERT(g_cc[0] != 0);
    if (!g_cc[0]) return t_finalize();
    if (file_ok("C:\\msys64\\mingw64\\bin\\gcc.exe"))
        snprintf(g_gcc, sizeof g_gcc, "C:\\msys64\\mingw64\\bin\\gcc.exe");
    else if (!search_path(g_gcc, sizeof g_gcc, "gcc.exe"))
        g_gcc[0] = 0;
    T_ASSERT(g_gcc[0] != 0);
    if (!g_gcc[0]) return t_finalize();
    if (file_ok("C:\\msys64\\mingw64\\bin\\ar.exe"))
        snprintf(g_ar, sizeof g_ar, "C:\\msys64\\mingw64\\bin\\ar.exe");
    else if (!search_path(g_ar, sizeof g_ar, "ar.exe"))
        g_ar[0] = 0;
    T_ASSERT(g_ar[0] != 0);
    if (!g_ar[0]) return t_finalize();
    parent_lib(g_mingw_lib, sizeof g_mingw_lib, g_gcc);
    MKDIR(WORK);
    test_static_archive();
    test_consume_mingw_dll();
    test_fakecc_dll();
    test_fakecc_dll_tls();
    test_tls_features_e2e();
    test_tls_multithread_isolation();
    test_native_windows_threads();
    test_pe_headers();
    return t_finalize();
}
