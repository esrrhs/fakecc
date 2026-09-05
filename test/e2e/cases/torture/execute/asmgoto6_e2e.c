// expect: 0
// Runtime smoke test mirroring test/compile/asmgoto-6.c (PR middle-end/110420).
// Validates that fakecc can compile a function whose only interesting
// statement is an asm goto with an output operand, ending in a bare label
// (the goto target).  As of 2026-09-05 fakecc treats asm goto as a black box
// (the template / output binding / labels are dropped), so this case does
// NOT exercise the real asm-goto semantics — it only verifies that the
// parser accepts the construct and emits a runnable program that returns 0
// from main.  The real semantics check happens at compile time in the
// test/compile suite via test/compile/asmgoto-6.c.
package main;

extern void abort(void);

static int t;
static int g_called;

void g(void) { g_called = 1; }

void f(void)
{
    int __gu_val;
    asm goto ("#my asm "
        : "=&r" (__gu_val)
        :
        :
        : Efault);
    t = __gu_val;
    g();
Efault:
}

static void test_multi_end_labels(void)
{
l1:
l2:
}

static void test_consecutive_labels_in_if(void)
{
    int val = 0;
    if (0)
    lbl1:
    lbl2:
        val = 1;
    if (val != 0)
        abort();
}

int main(void)
{
    f();
    test_multi_end_labels();
    test_consecutive_labels_in_if();
    /* main returns 0 regardless of asm-goto semantics — see the comment
     * above.  The compile-time check in test/compile/asmgoto-6.c is the
     * strict assertion; this e2e case only guards against the parser
     * regressing in a way that also prevents the program from running. */
    return 0;
}

