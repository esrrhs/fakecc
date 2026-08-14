// expect: 0
package main;


import runtime;
// This test exercises the φ-merge-across-call pattern:
// Variables initialized before a loop, conditionally modified
// inside a loop containing a function call, read after the loop.
// Multiple variables are used to create register pressure.
int test(int n, int a0, int a1, int a2, int a3, int a4, int a5) {
    int v0, v1, v2, v3, v4, v5, v6, v7, v8, v9;
    int flag;
    int i;

    // Initialize all variables before the loop
    v0 = a0; v1 = a1; v2 = a2; v3 = a3; v4 = a4;
    v5 = a5; v6 = 6; v7 = 7; v8 = 8; v9 = 9;
    flag = 0;
    i = 0;

L:
    if (i >= n) goto done;

    // Function call inside loop — values live across call
    runtime.puts("loop");

    // Carry variables across the call (forces register pressure)
    v0 = v0 + 1;
    v1 = v1 + 1;
    v2 = v2 + 1;
    v3 = v3 + 1;
    v4 = v4 + 1;
    v5 = v5 + 1;
    v6 = v6 + 1;
    v7 = v7 + 1;
    v8 = v8 + 1;
    v9 = v9 + 1;

    // Conditionally modify flag (creates phi merge)
    if (i > 5)
        flag = 1;

    i = i + 1;
    goto L;

done:
    // Read after the loop: v0..v9 should have been incremented n times
    // flag should be 0 when n <= 5, 1 when n > 5
    if (n <= 5) {
        if (flag != 0) return 1;
    } else {
        if (flag != 1) return 2;
    }
    // Check that loop-carried values were correctly incremented
    if (v0 != a0 + n) return 10;
    if (v1 != a1 + n) return 11;
    if (v2 != a2 + n) return 12;
    if (v3 != a3 + n) return 13;
    if (v4 != a4 + n) return 14;
    if (v5 != a5 + n) return 15;
    if (v6 != 6 + n) return 16;
    if (v7 != 7 + n) return 17;
    if (v8 != 8 + n) return 18;
    if (v9 != 9 + n) return 19;
    return 0;
}

// Also test goto-based continue pattern
int test_goto_continue(int n) {
    int val, t0, t1, t2, t3;
    val = 0;
    t0 = 0; t1 = 1; t2 = 2; t3 = 3;
    int i;
    i = 0;

loop_head:
    if (i >= n) goto done2;
    runtime.puts("loop2");
    t0 = t0 + 1;
    t1 = t1 + 1;
    t2 = t2 + 1;
    t3 = t3 + 1;
    if (i > 3)
        val = 1;
    i = i + 1;
    goto loop_head;

done2:
    if (n <= 3 && val != 0) return 1;
    if (n > 3 && val != 1) return 2;
    if (t0 != n) return 3;
    if (t1 != 1 + n) return 4;
    return 0;
}

int main(void) {
    int r;

    // Test with n=3 (flag should stay 0)
    r = test(3, 100, 200, 300, 400, 500, 600);
    if (r != 0) return r;

    // Test with n=10 (flag should become 1)
    r = test(10, 100, 200, 300, 400, 500, 600);
    if (r != 0) return r;

    // Test goto-continue pattern
    r = test_goto_continue(2);
    if (r != 0) return r + 100;

    r = test_goto_continue(5);
    if (r != 0) return r + 100;

    return 0;
}
