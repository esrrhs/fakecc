/* __builtin_*_overflow checked arithmetic on the hot path. */
package main;
import runtime;

enum { N = 6000000 };

int main(void) {
    unsigned i, a, b, r;
    int sa, sb, sr;
    unsigned long long ua, ub, ur;
    unsigned ov, acc;

    a = 0xfffffff0u;
    b = 17u;
    sa = 0x7ffffff0;
    sb = 20;
    ua = 0xffffffffffffff00ULL;
    ub = 300ULL;
    ov = 0;
    acc = 0;
    for (i = 0; i < N; i++) {
        if (__builtin_add_overflow(a, b, &r))
            ov++;
        else
            a = r;
        a ^= i;
        b = (b * 3u) + (i & 31u) + 1u;

        if (__builtin_mul_overflow(sa, (int)(i & 7u) + 1, &sr))
            ov++;
        else
            sa = sr ^ (int)(i & 255u);
        if (__builtin_sub_overflow(sa, sb, &sr))
            ov++;
        else
            sa = sr;
        sb = (int)(i & 63u) - 31;

        if (__builtin_add_overflow(ua, ub, &ur))
            ov++;
        else
            ua = ur;
        ua ^= (unsigned long long)i << 3;
        ub = ub * 3ULL + (unsigned long long)(i & 15u) + 1ULL;

        acc += a ^ (unsigned)sa ^ (unsigned)(ua);
        acc ^= ov;
    }
    runtime.printf("overflow %u %u\n", acc, ov);
    return 0;
}
