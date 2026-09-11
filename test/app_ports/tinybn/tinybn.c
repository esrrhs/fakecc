/*
 * tiny-bignum-c — arithmetic on 1024-bit unsigned integers
 * FakeCC port of https://github.com/kokke/tiny-bignum-c
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 *
 * FakeCC adaptations:
 *   - package tinybn; import runtime (no preprocessor / headers)
 *   - WORD_SIZE frozen at 4 (uint32_t words, uint64_t temporaries)
 *   - BN_ARRAY_SIZE = 32 (1024-bit numbers)
 *   - hex I/O written without sscanf
 *   - require()/assert elided (callers never pass null in tests)
 */

package tinybn;

import runtime;

typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

enum {
    WORD_SIZE = 4,
    BN_ARRAY_SIZE = 32
};

enum { SMALLER = -1, EQUAL = 0, LARGER = 1 };

struct bn {
    uint32_t array[32];
};
typedef struct bn bn;

void bignum_init(struct bn *n);
void bignum_from_int(struct bn *n, uint64_t i);
int bignum_to_int(struct bn *n);
void bignum_from_string(struct bn *n, char *str, int nbytes);
void bignum_to_string(struct bn *n, char *str, int nbytes);
void bignum_add(struct bn *a, struct bn *b, struct bn *c);
void bignum_sub(struct bn *a, struct bn *b, struct bn *c);
void bignum_mul(struct bn *a, struct bn *b, struct bn *c);
void bignum_div(struct bn *a, struct bn *b, struct bn *c);
void bignum_mod(struct bn *a, struct bn *b, struct bn *c);
void bignum_divmod(struct bn *a, struct bn *b, struct bn *c, struct bn *d);
void bignum_and(struct bn *a, struct bn *b, struct bn *c);
void bignum_or(struct bn *a, struct bn *b, struct bn *c);
void bignum_xor(struct bn *a, struct bn *b, struct bn *c);
void bignum_lshift(struct bn *a, struct bn *b, int nbits);
void bignum_rshift(struct bn *a, struct bn *b, int nbits);
int bignum_cmp(struct bn *a, struct bn *b);
int bignum_is_zero(struct bn *n);
void bignum_inc(struct bn *n);
void bignum_dec(struct bn *n);
void bignum_pow(struct bn *a, struct bn *b, struct bn *c);
void bignum_isqrt(struct bn *a, struct bn *b);
void bignum_assign(struct bn *dst, struct bn *src);

static void _lshift_one_bit(struct bn *a);
static void _rshift_one_bit(struct bn *a);
static void _lshift_word(struct bn *a, int nwords);
static void _rshift_word(struct bn *a, int nwords);

static uint32_t hex_digit(char c) {
    if (c >= '0' && c <= '9') return (uint32_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (uint32_t)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (uint32_t)(c - 'A' + 10);
    return 0;
}

static uint32_t parse_hex8(const char *s) {
    uint32_t v;
    int k;
    v = 0;
    for (k = 0; k < 8; ++k) {
        v = (v << 4) | hex_digit(s[k]);
    }
    return v;
}

static void put_hex8(char *dst, uint32_t v) {
    int k;
    unsigned d;
    for (k = 7; k >= 0; --k) {
        d = (v >> (k * 4)) & 0xf;
        if (d < 10) dst[7 - k] = (char)('0' + d);
        else dst[7 - k] = (char)('a' + d - 10);
    }
}

void bignum_init(struct bn *n) {
    int i;
    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        n->array[i] = 0;
    }
}

void bignum_from_int(struct bn *n, uint64_t i) {
    uint64_t num_32;
    uint64_t tmp;

    bignum_init(n);
    n->array[0] = (uint32_t)i;
    num_32 = 32;
    tmp = i >> num_32;
    n->array[1] = (uint32_t)tmp;
}

int bignum_to_int(struct bn *n) {
    return (int)n->array[0];
}

void bignum_from_string(struct bn *n, char *str, int nbytes) {
    int i;
    int j;

    bignum_init(n);
    i = nbytes - 8;
    j = 0;
    while (i >= 0) {
        n->array[j] = parse_hex8(&str[i]);
        i -= 8;
        j += 1;
    }
}

void bignum_to_string(struct bn *n, char *str, int nbytes) {
    int j;
    int i;

    j = BN_ARRAY_SIZE - 1;
    i = 0;
    while ((j >= 0) && (nbytes > (i + 1))) {
        put_hex8(&str[i], n->array[j]);
        i += 8;
        j -= 1;
    }
    str[i] = 0;

    j = 0;
    while (str[j] == '0') {
        j += 1;
    }
    for (i = 0; i < (nbytes - j); ++i) {
        str[i] = str[i + j];
    }
    str[i] = 0;
}

void bignum_dec(struct bn *n) {
    uint32_t tmp;
    uint32_t res;
    int i;

    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        tmp = n->array[i];
        res = tmp - 1;
        n->array[i] = res;
        if (!(res > tmp)) {
            break;
        }
    }
}

void bignum_inc(struct bn *n) {
    uint32_t res;
    uint64_t tmp;
    int i;

    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        tmp = n->array[i];
        res = (uint32_t)(tmp + 1);
        n->array[i] = res;
        if (res > tmp) {
            break;
        }
    }
}

void bignum_add(struct bn *a, struct bn *b, struct bn *c) {
    uint64_t tmp;
    int carry;
    int i;

    carry = 0;
    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        tmp = (uint64_t)a->array[i] + (uint64_t)b->array[i] + (uint64_t)carry;
        carry = (tmp > 0xFFFFFFFFULL);
        c->array[i] = (uint32_t)(tmp & 0xFFFFFFFFULL);
    }
}

void bignum_sub(struct bn *a, struct bn *b, struct bn *c) {
    uint64_t res;
    uint64_t tmp1;
    uint64_t tmp2;
    int borrow;
    int i;

    borrow = 0;
    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        tmp1 = (uint64_t)a->array[i] + 0x100000000ULL;
        tmp2 = (uint64_t)b->array[i] + (uint64_t)borrow;
        res = tmp1 - tmp2;
        c->array[i] = (uint32_t)(res & 0xFFFFFFFFULL);
        borrow = (res <= 0xFFFFFFFFULL);
    }
}

void bignum_mul(struct bn *a, struct bn *b, struct bn *c) {
    struct bn row;
    struct bn tmp;
    uint64_t intermediate;
    int i;
    int j;

    bignum_init(c);
    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        bignum_init(&row);
        for (j = 0; j < BN_ARRAY_SIZE; ++j) {
            if (i + j < BN_ARRAY_SIZE) {
                bignum_init(&tmp);
                intermediate = (uint64_t)a->array[i] * (uint64_t)b->array[j];
                bignum_from_int(&tmp, intermediate);
                _lshift_word(&tmp, i + j);
                bignum_add(&tmp, &row, &row);
            }
        }
        bignum_add(c, &row, c);
    }
}

void bignum_div(struct bn *a, struct bn *b, struct bn *c) {
    struct bn current;
    struct bn denom;
    struct bn tmp;
    uint64_t half_max;
    int overflow;

    bignum_from_int(&current, 1);
    bignum_assign(&denom, b);
    bignum_assign(&tmp, a);
    half_max = 1ULL + (0xFFFFFFFFULL / 2ULL);
    overflow = 0;
    while (bignum_cmp(&denom, a) != LARGER) {
        if (denom.array[BN_ARRAY_SIZE - 1] >= (uint32_t)half_max) {
            overflow = 1;
            break;
        }
        _lshift_one_bit(&current);
        _lshift_one_bit(&denom);
    }
    if (!overflow) {
        _rshift_one_bit(&denom);
        _rshift_one_bit(&current);
    }
    bignum_init(c);
    while (!bignum_is_zero(&current)) {
        if (bignum_cmp(&tmp, &denom) != SMALLER) {
            bignum_sub(&tmp, &denom, &tmp);
            bignum_or(c, &current, c);
        }
        _rshift_one_bit(&current);
        _rshift_one_bit(&denom);
    }
}

void bignum_lshift(struct bn *a, struct bn *b, int nbits) {
    int nbits_pr_word;
    int nwords;
    int i;

    bignum_assign(b, a);
    nbits_pr_word = WORD_SIZE * 8;
    nwords = nbits / nbits_pr_word;
    if (nwords != 0) {
        _lshift_word(b, nwords);
        nbits -= nwords * nbits_pr_word;
    }
    if (nbits != 0) {
        for (i = BN_ARRAY_SIZE - 1; i > 0; --i) {
            b->array[i] = (b->array[i] << nbits) | (b->array[i - 1] >> (32 - nbits));
        }
        b->array[i] <<= nbits;
    }
}

void bignum_rshift(struct bn *a, struct bn *b, int nbits) {
    int nbits_pr_word;
    int nwords;
    int i;

    bignum_assign(b, a);
    nbits_pr_word = WORD_SIZE * 8;
    nwords = nbits / nbits_pr_word;
    if (nwords != 0) {
        _rshift_word(b, nwords);
        nbits -= nwords * nbits_pr_word;
    }
    if (nbits != 0) {
        for (i = 0; i < BN_ARRAY_SIZE - 1; ++i) {
            b->array[i] = (b->array[i] >> nbits) | (b->array[i + 1] << (32 - nbits));
        }
        b->array[i] >>= nbits;
    }
}

void bignum_mod(struct bn *a, struct bn *b, struct bn *c) {
    struct bn tmp;
    bignum_divmod(a, b, &tmp, c);
}

void bignum_divmod(struct bn *a, struct bn *b, struct bn *c, struct bn *d) {
    struct bn tmp;
    bignum_div(a, b, c);
    bignum_mul(c, b, &tmp);
    bignum_sub(a, &tmp, d);
}

void bignum_and(struct bn *a, struct bn *b, struct bn *c) {
    int i;
    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        c->array[i] = a->array[i] & b->array[i];
    }
}

void bignum_or(struct bn *a, struct bn *b, struct bn *c) {
    int i;
    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        c->array[i] = a->array[i] | b->array[i];
    }
}

void bignum_xor(struct bn *a, struct bn *b, struct bn *c) {
    int i;
    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        c->array[i] = a->array[i] ^ b->array[i];
    }
}

int bignum_cmp(struct bn *a, struct bn *b) {
    int i;

    i = BN_ARRAY_SIZE;
    do {
        i -= 1;
        if (a->array[i] > b->array[i]) {
            return LARGER;
        } else if (a->array[i] < b->array[i]) {
            return SMALLER;
        }
    } while (i != 0);
    return EQUAL;
}

int bignum_is_zero(struct bn *n) {
    int i;
    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        if (n->array[i]) {
            return 0;
        }
    }
    return 1;
}

void bignum_pow(struct bn *a, struct bn *b, struct bn *c) {
    struct bn tmp;
    struct bn bcopy;

    bignum_init(c);
    if (bignum_cmp(b, c) == EQUAL) {
        bignum_inc(c);
    } else {
        bignum_assign(&bcopy, b);
        bignum_assign(&tmp, a);
        bignum_dec(&bcopy);
        while (!bignum_is_zero(&bcopy)) {
            bignum_mul(&tmp, a, c);
            bignum_dec(&bcopy);
            bignum_assign(&tmp, c);
        }
        bignum_assign(c, &tmp);
    }
}

void bignum_isqrt(struct bn *a, struct bn *b) {
    struct bn low;
    struct bn high;
    struct bn mid;
    struct bn tmp;

    bignum_init(&low);
    bignum_assign(&high, a);
    bignum_rshift(&high, &mid, 1);
    bignum_inc(&mid);
    while (bignum_cmp(&high, &low) > 0) {
        bignum_mul(&mid, &mid, &tmp);
        if (bignum_cmp(&tmp, a) > 0) {
            bignum_assign(&high, &mid);
            bignum_dec(&high);
        } else {
            bignum_assign(&low, &mid);
        }
        bignum_sub(&high, &low, &mid);
        _rshift_one_bit(&mid);
        bignum_add(&low, &mid, &mid);
        bignum_inc(&mid);
    }
    bignum_assign(b, &low);
}

void bignum_assign(struct bn *dst, struct bn *src) {
    int i;
    for (i = 0; i < BN_ARRAY_SIZE; ++i) {
        dst->array[i] = src->array[i];
    }
}

static void _rshift_word(struct bn *a, int nwords) {
    int i;

    if (nwords >= BN_ARRAY_SIZE) {
        for (i = 0; i < BN_ARRAY_SIZE; ++i) {
            a->array[i] = 0;
        }
        return;
    }
    for (i = 0; i < BN_ARRAY_SIZE - nwords; ++i) {
        a->array[i] = a->array[i + nwords];
    }
    for (; i < BN_ARRAY_SIZE; ++i) {
        a->array[i] = 0;
    }
}

static void _lshift_word(struct bn *a, int nwords) {
    int i;

    for (i = BN_ARRAY_SIZE - 1; i >= nwords; --i) {
        a->array[i] = a->array[i - nwords];
    }
    for (; i >= 0; --i) {
        a->array[i] = 0;
    }
}

static void _lshift_one_bit(struct bn *a) {
    int i;
    for (i = BN_ARRAY_SIZE - 1; i > 0; --i) {
        a->array[i] = (a->array[i] << 1) | (a->array[i - 1] >> 31);
    }
    a->array[0] <<= 1;
}

static void _rshift_one_bit(struct bn *a) {
    int i;
    for (i = 0; i < BN_ARRAY_SIZE - 1; ++i) {
        a->array[i] = (a->array[i] >> 1) | (a->array[i + 1] << 31);
    }
    a->array[BN_ARRAY_SIZE - 1] >>= 1;
}
