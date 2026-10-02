#include "fakecc/a64.h"
#include "fakecc/macho.h"
#include "fakecc/reg_arm64.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <sys/wait.h>
#include <unistd.h>

static const char MSG[] = "fakecc macho t4 ok\n";  /* 19 bytes */

/* Build the freestanding program:
 *   write(2, msg, 19); exit(42);
 * with the string embedded right after the code in __text. */
static int build_program(Buffer *out) {
    A64Asm a;
    a64_init(&a);

    a64_stp64(&a, A64_FP, A64_LR, A64_SP, -16, A64_PAIR_PRE);
    a64_movz(&a, A64_X16, 4, 0, 1);          /* write */
    a64_movz(&a, A64_X0, 2, 0, 1);           /* stderr */
    int lmsg = a64_new_label(&a);
    a64_adrp_label(&a, A64_X1, lmsg);
    uint32_t add_at = (uint32_t)a.code.len;
    a64_add_imm12(&a, A64_X1, A64_X1, 0, 0, 1, 0);
    a64_movz(&a, A64_X2, (uint16_t)strlen(MSG), 0, 1);
    a64_svc(&a, 0x80);
    a64_movz(&a, A64_X16, 1, 0, 1);          /* exit */
    a64_movz(&a, A64_X0, 42, 0, 1);
    a64_svc(&a, 0x80);

    a64_bind(&a, lmsg);
    buffer_append(&a.code, MSG, strlen(MSG));

    if (a64_resolve(&a) != 0) { a64_free(&a); return -1; }

    /* ADRP page delta is zero (same page); ADD needs the message's file
     * page offset: text section base + buffer offset of the string. */
    uint32_t msg_buf_off = 10 * 4;
    uint32_t pageoff = (macho_text_offset() + msg_buf_off) & 0xFFFu;  /* 0x228 */
    uint32_t addw = 0x91000000u | (pageoff << 10)
                  | (A64_X1 << 5) | A64_X1;
    memcpy(a.code.data + add_at, &addw, 4);

    *out = a.code;
    /* Keep the backing storage alive: detach without freeing. */
    memset(&a, 0, sizeof(a));
    return 0;
}

/* Run path, capture stderr into buf; returns exit status. */
static int run_capture(const char *path, char *buf, size_t buflen) {
    int p[2];
    if (pipe(p) < 0) return -1;
    pid_t pid = fork();
    if (pid == 0) {
        dup2(p[1], 2);
        close(p[0]); close(p[1]);
        execl(path, path, (char *)NULL);
        _exit(127);
    }
    close(p[1]);
    size_t n = read(p[0], buf, buflen - 1);
    buf[n > 0 ? n : 0] = 0;
    close(p[0]);
    int st;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

static void test_roundtrip(void) {
    Buffer code;
    T_ASSERT_EQ_INT(build_program(&code), 0);

    const char *path = "/tmp/fakecc_test_macho";
    /* keep */
    T_ASSERT_EQ_INT(macho_write_exec_text(&code, macho_text_offset(), path), 0);
    T_ASSERT_EQ_INT(macho_codesign(path), 0);

    /* Structural checks. */
    FILE *f = fopen(path, "rb");
    T_ASSERT(f != NULL);
    uint32_t mh[8];
    T_ASSERT(fread(mh, 4, 8, f) == 8);
    fclose(f);
    /* Compare as uint32 — MH_MAGIC_64 exceeds INT_MAX and this clang
     * mis-folds the raw 0xFEEDFACFu literal in comparisons. */
    uint32_t expect_magic = 0xCFu | (0xFAu << 8) | (0xEDu << 16)
                          | (0xFEu << 24);
    T_ASSERT(mh[0] == expect_magic);                /* MH_MAGIC_64 */
    T_ASSERT_EQ_INT((int)mh[1], 0x0100000C);       /* CPU_TYPE_ARM64 */
    T_ASSERT_EQ_INT((int)mh[2], 0);                /* CPU_SUBTYPE_ALL */
    T_ASSERT_EQ_INT((int)mh[3], 2);                /* MH_EXECUTE */
    T_ASSERT((mh[6] & 0x00200000u) != 0);          /* MH_PIE */

    char out[64];
    int rc = run_capture(path, out, sizeof out);
    T_ASSERT_EQ_INT(rc, 42);
    T_ASSERT_STR_EQ(out, MSG);

    free(code.data);
    /* keep */
}
#endif

int main(void) {
#ifdef __APPLE__
    test_roundtrip();
#else
    /* Mach-O writing is exercised on Apple Silicon hosts only. */
#endif
    return t_finalize();
}
