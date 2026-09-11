/* tiny-bignum-c FakeCC port tests.
 * Combines upstream golden, load_cmp, hand_picked, and factorial(100). */
package main;

import tinybn;
import runtime;

typedef struct {
    char op;
    unsigned a;
    unsigned b;
    unsigned c;
} gold_test;

static gold_test oracle[] = {
    { '+', 80, 20, 100 },
    { '+', 18, 22, 40 },
    { '+', 12, 8, 20 },
    { '+', 100080, 20, 100100 },
    { '+', 18, 559022, 559040 },
    { '+', 2000000000u, 2000000000u, 4000000000u },
    { '+', 0x00FFFF, 1, 0x010000 },
    { '+', 0x00FFFF00, 0x00000100, 0x01000000 },
    { '-', 1000001, 1000000, 1 },
    { '-', 42, 0, 42 },
    { '-', 101, 100, 1 },
    { '-', 242, 42, 200 },
    { '-', 1042, 0, 1042 },
    { '-', 101010101, 101010100, 1 },
    { '-', 0x010000, 1, 0x00FFFF },
    { '-', 0xf505c2, 0x0fffe0, 0xe505e2 },
    { '-', 0x9f735a, 0x65ffb5, 0x3973a5 },
    { '-', 0xcf7810, 0x04ff34, 0xca78dc },
    { '-', 0xbbc55f, 0x4eff76, 0x6cc5e9 },
    { '-', 0x100000, 1, 0x0fffff },
    { '-', 0x010000, 1, 0x00ffff },
    { '-', 0xb5beb4, 0x01ffc4, 0xb3bef0 },
    { '-', 0x707655, 0x50ffa8, 0x1f76ad },
    { '-', 0xf0a990, 0x1cffd1, 0xd3a9bf },
    { '*', 0x010203, 0x1020, 0x10407060 },
    { '*', 42, 0, 0 },
    { '*', 42, 1, 42 },
    { '*', 42, 2, 84 },
    { '*', 42, 10, 420 },
    { '*', 42, 100, 4200 },
    { '*', 420, 1000, 420000 },
    { '*', 200, 8, 1600 },
    { '*', 2, 256, 512 },
    { '*', 500, 2, 1000 },
    { '*', 500000, 2, 1000000 },
    { '*', 500, 500, 250000 },
    { '*', 1000000000, 2, 2000000000 },
    { '*', 2, 1000000000, 2000000000 },
    { '*', 1000000000, 4, 4000000000u },
    { '/', 0xFFFFFFFFu, 0xFFFFFFFFu, 1 },
    { '/', 0xFFFFFFFFu, 0x10000, 0xFFFF },
    { '/', 0xFFFFFFFFu, 0x1000, 0xFFFFF },
    { '/', 0xFFFFFFFFu, 0x100, 0xFFFFFF },
    { '/', 1000000, 1000, 1000 },
    { '/', 1000000, 10000, 100 },
    { '/', 1000000, 100000, 10 },
    { '/', 1000000, 1000000, 1 },
    { '/', 1000000, 10000000, 0 },
    { '/', 28, 7, 4 },
    { '/', 27, 7, 3 },
    { '/', 26, 7, 3 },
    { '/', 25, 7, 3 },
    { '/', 24, 7, 3 },
    { '/', 23, 7, 3 },
    { '/', 22, 7, 3 },
    { '/', 21, 7, 3 },
    { '/', 20, 7, 2 },
    { '/', 0, 12, 0 },
    { '/', 10, 1, 10 },
    { '/', 0xFFFFFFFFu, 1, 0xFFFFFFFFu },
    { '/', 0xFFFFFFFFu, 0x10000, 0xFFFF },
    { '/', 0xb36627, 0x0dff95, 0x0c },
    { '/', 0xe5a18e, 0x09ff82, 0x16 },
    { '/', 0x45edd0, 0x04ff1a, 0x0d },
    { '%', 8, 3, 2 },
    { '%', 1024, 1000, 24 },
    { '%', 0xFFFFFF, 1234, 985 },
    { '%', 0xFFFFFFFFu, 0xEF, 0x6D },
    { '%', 12345678, 16384, 8526 },
    { '%', 0xe7a344, 0x71ffe8, 0x03a374 },
    { '%', 0xa3a9a1, 0x2ff44, 0x1d149 },
    { '%', 0xc128b2, 0x60ff61, 0x602951 },
    { '%', 0xDC2254, 0x517FEA, 0x392280 },
    { '%', 0x769c99, 0x2cffda, 0x1c9ce5 },
    { '%', 0xc19076, 0x31ffd4, 0x2b90fa },
    { '&', 0xFFFFFFFFu, 0x005500AA, 0x005500AA },
    { '&', 7, 3, 3 },
    { '&', 0xFFFFFFFFu, 0, 0 },
    { '&', 0, 0xFFFFFFFFu, 0 },
    { '&', 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu },
    { '|', 0xFFFFFFFFu, 0, 0xFFFFFFFFu },
    { '|', 0, 0xFFFFFFFFu, 0xFFFFFFFFu },
    { '|', 0x00000000, 0xFFFFFFFFu, 0xFFFFFFFFu },
    { '|', 0x55555555, 0xAAAAAAAA, 0xFFFFFFFFu },
    { '|', 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu },
    { '|', 4, 3, 7 },
    { '^', 7, 4, 3 },
    { '^', 0xFFFF, 0x5555, 0xAAAA },
    { '^', 0x5555, 0xAAAA, 0xFFFF },
    { '^', 0xAAAA, 0x5555, 0xFFFF },
    { '^', 0x0000, 0xFFFF, 0xFFFF },
    { '^', 0x5555, 0xFFFF, 0xAAAA },
    { '^', 0xAAAA, 0xFFFF, 0x5555 },
    { 'p', 2, 0, 1 },
    { 'p', 2, 1, 2 },
    { 'p', 2, 2, 4 },
    { 'p', 2, 3, 8 },
    { 'p', 2, 10, 1024 },
    { 'p', 2, 20, 1048576 },
    { 'p', 2, 30, 1073741824 },
    { '<', 1, 0, 1 },
    { '<', 1, 1, 2 },
    { '<', 1, 2, 4 },
    { '<', 1, 3, 8 },
    { '<', 1, 4, 16 },
    { '<', 1, 5, 32 },
    { '<', 1, 6, 64 },
    { '<', 1, 7, 128 },
    { '<', 1, 8, 256 },
    { '<', 1, 9, 512 },
    { '<', 1, 10, 1024 },
    { '<', 1, 11, 2048 },
    { '<', 1, 12, 4096 },
    { '<', 1, 13, 8192 },
    { '<', 1, 14, 16384 },
    { '<', 1, 15, 32768 },
    { '<', 1, 16, 65536 },
    { '<', 1, 17, 131072 },
    { '<', 1, 18, 262144 },
    { '<', 1, 19, 524288 },
    { '<', 1, 20, 1048576 },
    { '<', 0xdd, 0x18, 0xdd000000 },
    { '<', 0x68, 0x02, 0x01a0 },
    { '>', 0xf6, 1, 0x7b },
    { '>', 0x1a, 1, 0x0d },
    { '>', 0xb0, 1, 0x58 },
    { '>', 0xba, 1, 0x5d },
    { '>', 0x10, 3, 0x02 },
    { '>', 0xe8, 4, 0x0e },
    { '>', 0x37, 4, 0x03 },
    { '>', 0xa0, 7, 0x01 },
    { '>', 1, 0, 1 },
    { '>', 2, 1, 1 },
    { '>', 4, 2, 1 },
    { '>', 8, 3, 1 },
    { '>', 16, 4, 1 },
    { '>', 32, 5, 1 },
    { '>', 64, 6, 1 },
    { '>', 128, 7, 1 },
    { '>', 256, 8, 1 },
    { '>', 512, 9, 1 },
    { '>', 1024, 10, 1 },
    { '>', 2048, 11, 1 },
    { '>', 4096, 12, 1 },
    { '>', 8192, 13, 1 },
    { '>', 16384, 14, 1 },
    { '>', 32768, 15, 1 },
    { '>', 65536, 16, 1 },
    { '>', 131072, 17, 1 },
    { '>', 262144, 18, 1 },
    { '>', 524288, 19, 1 },
    { '>', 1048576, 20, 1 },
    { 0, 0, 0, 0 }
};

static int nfailed;

static void fail(const char *msg) {
    nfailed += 1;
    runtime.printf("FAIL: %s\n", msg);
}

static void factorial(tinybn.bn *n, tinybn.bn *res) {
    tinybn.bn tmp;

    tinybn.bignum_assign(&tmp, n);
    tinybn.bignum_dec(n);
    while (!tinybn.bignum_is_zero(n)) {
        tinybn.bignum_mul(&tmp, n, res);
        tinybn.bignum_dec(n);
        tinybn.bignum_assign(&tmp, res);
    }
    tinybn.bignum_assign(res, &tmp);
}

static int test_golden(void) {
    tinybn.bn sa, sb, sc, sd;
    unsigned ia, ib, ic;
    char op;
    int i;
    int ntests;
    int npassed;
    int test_passed;

    ntests = 0;
    npassed = 0;
    while (oracle[ntests].op) {
        ntests += 1;
    }

    for (i = 0; i < ntests; ++i) {
        op = oracle[i].op;
        ia = oracle[i].a;
        ib = oracle[i].b;
        ic = oracle[i].c;
        tinybn.bignum_init(&sd);
        tinybn.bignum_from_int(&sa, ia);
        tinybn.bignum_from_int(&sb, ib);
        tinybn.bignum_from_int(&sc, ic);
        if (op == '+') tinybn.bignum_add(&sa, &sb, &sd);
        else if (op == '-') tinybn.bignum_sub(&sa, &sb, &sd);
        else if (op == '*') tinybn.bignum_mul(&sa, &sb, &sd);
        else if (op == '/') tinybn.bignum_div(&sa, &sb, &sd);
        else if (op == '%') tinybn.bignum_mod(&sa, &sb, &sd);
        else if (op == '&') tinybn.bignum_and(&sa, &sb, &sd);
        else if (op == '|') tinybn.bignum_or(&sa, &sb, &sd);
        else if (op == '^') tinybn.bignum_xor(&sa, &sb, &sd);
        else if (op == 'p') tinybn.bignum_pow(&sa, &sb, &sd);
        else if (op == '<') tinybn.bignum_lshift(&sa, &sd, (int)ib);
        else if (op == '>') tinybn.bignum_rshift(&sa, &sd, (int)ib);
        else {
            fail("golden: unknown op");
            continue;
        }
        test_passed = (tinybn.bignum_cmp(&sc, &sd) == tinybn.EQUAL);
        if (test_passed) {
            npassed += 1;
        } else {
            runtime.printf("FAIL golden: %u %c %u expected %u\n", ia, op, ib, ic);
            nfailed += 1;
        }
    }
    runtime.printf("golden: %d/%d\n", npassed, ntests);
    return ntests - npassed;
}

static int test_load_cmp(void) {
    char sabuf[512];
    char sbbuf[512];
    char scbuf[512];
    char sdbuf[512];
    char iabuf[512];
    char ibbuf[512];
    char icbuf[512];
    char idbuf[512];
    tinybn.bn sa, sb, sc, sd, se;
    tinybn.bn ia, ib, ic, id;
    int i;
    int fails;

    fails = 0;
    tinybn.bignum_from_string(&sa, "000000FF", 8);
    tinybn.bignum_from_string(&sb, "0000FF00", 8);
    tinybn.bignum_from_string(&sc, "00FF0000", 8);
    tinybn.bignum_from_string(&sd, "FF000000", 8);
    tinybn.bignum_from_int(&ia, 0x000000FF);
    tinybn.bignum_from_int(&ib, 0x0000FF00);
    tinybn.bignum_from_int(&ic, 0x00FF0000);
    tinybn.bignum_from_int(&id, 0xFF000000);

    if (tinybn.bignum_cmp(&ia, &ib) != tinybn.SMALLER) { fail("cmp ia<ib"); fails += 1; }
    if (tinybn.bignum_cmp(&ia, &ic) != tinybn.SMALLER) { fail("cmp ia<ic"); fails += 1; }
    if (tinybn.bignum_cmp(&ia, &id) != tinybn.SMALLER) { fail("cmp ia<id"); fails += 1; }
    if (tinybn.bignum_cmp(&ib, &ia) != tinybn.LARGER) { fail("cmp ib>ia"); fails += 1; }
    if (tinybn.bignum_cmp(&ic, &ia) != tinybn.LARGER) { fail("cmp ic>ia"); fails += 1; }
    if (tinybn.bignum_cmp(&id, &ia) != tinybn.LARGER) { fail("cmp id>ia"); fails += 1; }
    if (tinybn.bignum_cmp(&sa, &sb) != tinybn.SMALLER) { fail("cmp sa<sb"); fails += 1; }
    if (tinybn.bignum_cmp(&sa, &sc) != tinybn.SMALLER) { fail("cmp sa<sc"); fails += 1; }
    if (tinybn.bignum_cmp(&sa, &sd) != tinybn.SMALLER) { fail("cmp sa<sd"); fails += 1; }
    if (tinybn.bignum_cmp(&sb, &sa) != tinybn.LARGER) { fail("cmp sb>sa"); fails += 1; }
    if (tinybn.bignum_cmp(&sc, &sa) != tinybn.LARGER) { fail("cmp sc>sa"); fails += 1; }
    if (tinybn.bignum_cmp(&sd, &sa) != tinybn.LARGER) { fail("cmp sd>sa"); fails += 1; }
    if (tinybn.bignum_cmp(&ia, &sa) != tinybn.EQUAL) { fail("cmp ia==sa"); fails += 1; }
    if (tinybn.bignum_cmp(&ib, &sb) != tinybn.EQUAL) { fail("cmp ib==sb"); fails += 1; }
    if (tinybn.bignum_cmp(&ic, &sc) != tinybn.EQUAL) { fail("cmp ic==sc"); fails += 1; }
    if (tinybn.bignum_cmp(&id, &sd) != tinybn.EQUAL) { fail("cmp id==sd"); fails += 1; }

    tinybn.bignum_to_string(&sa, sabuf, 512);
    tinybn.bignum_to_string(&sb, sbbuf, 512);
    tinybn.bignum_to_string(&sc, scbuf, 512);
    tinybn.bignum_to_string(&sd, sdbuf, 512);
    tinybn.bignum_to_string(&ia, iabuf, 512);
    tinybn.bignum_to_string(&ib, ibbuf, 512);
    tinybn.bignum_to_string(&ic, icbuf, 512);
    tinybn.bignum_to_string(&id, idbuf, 512);
    if (runtime.strcmp(sabuf, iabuf) != 0) { fail("to_string sa"); fails += 1; }
    if (runtime.strcmp(sbbuf, ibbuf) != 0) { fail("to_string sb"); fails += 1; }
    if (runtime.strcmp(scbuf, icbuf) != 0) { fail("to_string sc"); fails += 1; }
    if (runtime.strcmp(sdbuf, idbuf) != 0) { fail("to_string sd"); fails += 1; }

    tinybn.bignum_init(&sd);
    for (i = 0; i < 255; ++i) {
        tinybn.bignum_inc(&sd);
        if (tinybn.bignum_is_zero(&sd)) { fail("inc hit zero"); fails += 1; }
    }
    if (tinybn.bignum_cmp(&sd, &ia) != tinybn.EQUAL) { fail("inc 255"); fails += 1; }
    for (i = 0; i < 255; ++i) {
        if (tinybn.bignum_is_zero(&sd)) { fail("dec early zero"); fails += 1; }
        tinybn.bignum_dec(&sd);
    }
    if (!tinybn.bignum_is_zero(&sd)) { fail("dec to zero"); fails += 1; }

    tinybn.bignum_from_string(&sa, "000003E8", 8);
    tinybn.bignum_from_string(&sb, "000003E8", 8);
    tinybn.bignum_from_int(&sc, 0x3e8);
    if (tinybn.bignum_cmp(&sa, &sc) != tinybn.EQUAL) { fail("1000 string/int"); fails += 1; }
    tinybn.bignum_from_string(&sd, "000F4240", 8);
    tinybn.bignum_from_int(&se, 0xf4240);
    tinybn.bignum_mul(&sa, &sb, &sc);
    if (tinybn.bignum_cmp(&sc, &sd) != tinybn.EQUAL) { fail("1000*1000 string"); fails += 1; }
    if (tinybn.bignum_cmp(&sc, &se) != tinybn.EQUAL) { fail("1000*1000 int"); fails += 1; }

    runtime.printf("load_cmp: %s\n", fails ? "FAIL" : "ok");
    return fails;
}

static int test_hand_picked(void) {
    tinybn.bn a, b, c;
    int fails;

    fails = 0;
    /* Issue #2:  (max bignum) / 1  must not hang. */
    tinybn.bignum_from_int(&a, 1);
    tinybn.bignum_init(&b);
    tinybn.bignum_dec(&b);
    tinybn.bignum_div(&b, &a, &c);

    /* Issue #3:  0 - 1 + 3 == 2  (underflow wrap). */
    tinybn.bignum_from_int(&a, 0);
    tinybn.bignum_from_int(&b, 1);
    tinybn.bignum_sub(&a, &b, &a);
    tinybn.bignum_from_int(&b, 3);
    tinybn.bignum_add(&a, &b, &a);
    tinybn.bignum_from_int(&c, 2);
    if (tinybn.bignum_cmp(&a, &c) != tinybn.EQUAL) { fail("underflow 0-1+3"); fails += 1; }

    /* PR #7:  128-bit value >> 64. */
    tinybn.bignum_from_string(&a, "11112222333344445555666677778888", 32);
    tinybn.bignum_from_string(&c, "1111222233334444", 16);
    tinybn.bignum_rshift(&a, &b, 64);
    if (tinybn.bignum_cmp(&b, &c) != tinybn.EQUAL) { fail("rshift 64"); fails += 1; }

    runtime.printf("hand_picked: %s\n", fails ? "FAIL" : "ok");
    return fails;
}

static int test_factorial(void) {
    tinybn.bn num;
    tinybn.bn result;
    char buf[512];
    const char *want;

    want = "1b30964ec395dc24069528d54bbda40d16e966ef9a70eb21b5b2943a321cdf10391745570cca9420c6ecb3b72ed2ee8b02ea2735c61a000000000000000000000000";
    tinybn.bignum_from_int(&num, 100);
    factorial(&num, &result);
    tinybn.bignum_to_string(&result, buf, 512);
    if (runtime.strcmp(buf, want) != 0) {
        runtime.printf("FAIL factorial(100): got %s\n", buf);
        nfailed += 1;
        return 1;
    }
    runtime.printf("factorial(100): ok\n");
    return 0;
}

static int test_isqrt(void) {
    tinybn.bn a, b, c;
    int fails;

    fails = 0;
    tinybn.bignum_from_int(&a, 0);
    tinybn.bignum_isqrt(&a, &b);
    if (!tinybn.bignum_is_zero(&b)) { fail("isqrt 0"); fails += 1; }

    tinybn.bignum_from_int(&a, 1);
    tinybn.bignum_isqrt(&a, &b);
    tinybn.bignum_from_int(&c, 1);
    if (tinybn.bignum_cmp(&b, &c) != tinybn.EQUAL) { fail("isqrt 1"); fails += 1; }

    tinybn.bignum_from_int(&a, 4);
    tinybn.bignum_isqrt(&a, &b);
    tinybn.bignum_from_int(&c, 2);
    if (tinybn.bignum_cmp(&b, &c) != tinybn.EQUAL) { fail("isqrt 4"); fails += 1; }

    tinybn.bignum_from_int(&a, 5);
    tinybn.bignum_isqrt(&a, &b);
    tinybn.bignum_from_int(&c, 2);
    if (tinybn.bignum_cmp(&b, &c) != tinybn.EQUAL) { fail("isqrt 5"); fails += 1; }

    tinybn.bignum_from_int(&a, 100);
    tinybn.bignum_isqrt(&a, &b);
    tinybn.bignum_from_int(&c, 10);
    if (tinybn.bignum_cmp(&b, &c) != tinybn.EQUAL) { fail("isqrt 100"); fails += 1; }

    runtime.printf("isqrt: %s\n", fails ? "FAIL" : "ok");
    return fails;
}

int main(void) {
    nfailed = 0;
    test_golden();
    test_load_cmp();
    test_hand_picked();
    test_factorial();
    test_isqrt();
    if (nfailed) {
        runtime.printf("%d test(s) failed.\n", nfailed);
        return 1;
    }
    runtime.printf("ALL TINYBN TESTS PASSED\n");
    return 0;
}
