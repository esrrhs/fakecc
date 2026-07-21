#ifndef FAKECC_TEST_FRAMEWORK_H
#define FAKECC_TEST_FRAMEWORK_H

extern int t_total;
extern int t_failed;

void t_report(const char *file, int line, const char *expr);

#define T_ASSERT(cond) do { \
    t_total++; \
    if (!(cond)) { t_failed++; t_report(__FILE__, __LINE__, #cond); } \
} while (0)

#define T_ASSERT_EQ_INT(a, b) T_ASSERT((a) == (b))
#define T_ASSERT_STR_EQ(a, b) T_ASSERT(strcmp((a), (b)) == 0)

int t_finalize(void);

#endif /* FAKECC_TEST_FRAMEWORK_H */
