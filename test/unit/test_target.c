#include "fakecc/target.h"
#include "test_framework.h"

#include <string.h>

static void test_canonical_targets(void) {
    const TargetDesc *t = target_x86_64_linux();
    T_ASSERT_EQ_INT(t->arch, TARGET_ARCH_X86_64);
    T_ASSERT_EQ_INT(t->os, TOS_LINUX);
    T_ASSERT_EQ_INT(t->objfmt, TARGET_OBJFMT_ELF);
    T_ASSERT_EQ_INT(t->ptr_bits, 64);
    T_ASSERT_EQ_INT(t->native_vector_bytes, 32);
    T_ASSERT_EQ_INT(t->gp_arg_regs, 6);
    T_ASSERT_EQ_INT(t->sret_uses_gp, 1);
    T_ASSERT_STR_EQ(t->triple, "x86_64-linux");
    T_ASSERT(target_backend_ready(t));

    t = target_arm64_macos();
    T_ASSERT_EQ_INT(t->arch, TARGET_ARCH_ARM64);
    T_ASSERT_EQ_INT(t->os, TOS_MACOS);
    T_ASSERT_EQ_INT(t->objfmt, TARGET_OBJFMT_MACHO);
    T_ASSERT_EQ_INT(t->ptr_bits, 64);
    T_ASSERT_EQ_INT(t->native_vector_bytes, 16);
    T_ASSERT_EQ_INT(t->gp_arg_regs, 8);
    T_ASSERT_EQ_INT(t->sret_uses_gp, 0);
    T_ASSERT_STR_EQ(t->triple, "arm64-macos");
    /* Freestanding arm64 Mach-O executables work since T5/T6; relocatable
     * object emission and dylib links land in T14/T19. */
    T_ASSERT(target_backend_ready(t));
}

static void test_triple_aliases(void) {
    /* x86_64-linux spellings. */
    T_ASSERT(target_parse("x86_64-linux") == target_x86_64_linux());
    T_ASSERT(target_parse("x86_64-unknown-linux") == target_x86_64_linux());
    T_ASSERT(target_parse("x86_64-unknown-linux-gnu") == target_x86_64_linux());
    T_ASSERT(target_parse("amd64-linux") == target_x86_64_linux());
    T_ASSERT(target_parse("amd64-unknown-linux-gnu") == target_x86_64_linux());

    /* arm64-macos spellings. */
    T_ASSERT(target_parse("arm64-macos") == target_arm64_macos());
    T_ASSERT(target_parse("arm64-apple-macos") == target_arm64_macos());
    T_ASSERT(target_parse("arm64-apple-macos26") == target_arm64_macos());
    T_ASSERT(target_parse("arm64-darwin") == target_arm64_macos());
    T_ASSERT(target_parse("aarch64-apple-darwin") == target_arm64_macos());
}

static void test_bad_triples(void) {
    T_ASSERT(target_parse("") == NULL);
    T_ASSERT(target_parse("garbage") == NULL);
    T_ASSERT(target_parse("mips-linux") == NULL);
    /* Delivered combinations only. */
    T_ASSERT(target_parse("x86_64-macos") == NULL);   /* Rosetta backend: non-goal */
    T_ASSERT(target_parse("x86_64-apple-darwin") == NULL);
    T_ASSERT(target_parse("arm64-linux") == NULL);     /* aarch64-linux: not delivered */
    T_ASSERT(target_parse("aarch64-unknown-linux-gnu") == NULL);
    T_ASSERT(target_parse("arm64") == NULL);
    T_ASSERT(target_parse("linux") == NULL);
    T_ASSERT(target_parse(NULL) == NULL);
}

static void test_current_selection(void) {
    const TargetDesc *saved = target_current();
    T_ASSERT(saved != NULL);

    target_set_current(target_arm64_macos());
    T_ASSERT(target_current() == target_arm64_macos());
    T_ASSERT_EQ_INT(target_current()->arch, TARGET_ARCH_ARM64);

    target_set_current(target_x86_64_linux());
    T_ASSERT(target_current() == target_x86_64_linux());

    /* Restore so test ordering never leaks a selection into other suites. */
    target_set_current(saved);
}

static void test_host_default(void) {
    const TargetDesc *d = target_default();
#if defined(__APPLE__) && defined(__aarch64__)
    T_ASSERT(d == target_arm64_macos());
#elif defined(__linux__) && defined(__x86_64__)
    T_ASSERT(d == target_x86_64_linux());
#else
    (void)d; /* other hosts keep the historical x86-64 ELF default */
#endif
}

int main(void) {
    test_canonical_targets();
    test_triple_aliases();
    test_bad_triples();
    test_current_selection();
    test_host_default();
    return t_finalize();
}
