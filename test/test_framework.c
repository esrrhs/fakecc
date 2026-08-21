#include "test_framework.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(__SANITIZE_ADDRESS__) || defined(__has_feature)
const char *__asan_default_options(void) {
    return "detect_leaks=0";
}
#endif

int t_total = 0;
int t_failed = 0;

void t_report(const char *file, int line, const char *expr) {
    fprintf(stderr, "  FAIL %s:%d: %s\n", file, line, expr);
}

int t_finalize(void) {
    printf("%d tests, %d failures\n", t_total, t_failed);
    return t_failed > 0 ? 1 : 0;
}
