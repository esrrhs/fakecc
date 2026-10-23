/* Fakelua-style tagged CVar: type_ + flag_ + union payload, SET_* helpers,
 * LIKELY int/int arith path, IsTrue-style tests, and box pointer juggling. */
package main;
import runtime;

enum {
    VAR_NIL = 0,
    VAR_BOOL = 1,
    VAR_INT = 2,
    VAR_FLOAT = 3,
    VAR_PTR = 4
};

typedef struct CVar {
    int type_;
    int flag_;
    union {
        int b;
        long long i;
        double f;
        struct CVar *p;
    } data_;
} CVar;

static CVar set_nil(void) {
    CVar v;
    v.type_ = VAR_NIL;
    v.flag_ = 0;
    v.data_.i = 0;
    return v;
}

static CVar set_bool(int val) {
    CVar v;
    v.type_ = VAR_BOOL;
    v.flag_ = 0;
    v.data_.i = 0;
    v.data_.b = val;
    return v;
}

static CVar set_int(long long val) {
    CVar v;
    v.type_ = VAR_INT;
    v.flag_ = 0;
    v.data_.i = val;
    return v;
}

static CVar set_float(double val) {
    CVar v;
    v.type_ = VAR_FLOAT;
    v.flag_ = 0;
    v.data_.f = val;
    return v;
}

static int is_true(CVar v) {
    if (__builtin_expect(v.type_ == VAR_NIL, 0))
        return 0;
    if (v.type_ == VAR_BOOL)
        return v.data_.b;
    return 1;
}

static double to_double(CVar v) {
    if (v.type_ == VAR_INT)
        return (double)v.data_.i;
    return v.data_.f;
}

static CVar op_add(CVar a, CVar b) {
    if (__builtin_expect(a.type_ == VAR_INT && b.type_ == VAR_INT, 1))
        return set_int(a.data_.i + b.data_.i);
    return set_float(to_double(a) + to_double(b));
}

static CVar op_sub(CVar a, CVar b) {
    if (__builtin_expect(a.type_ == VAR_INT && b.type_ == VAR_INT, 1))
        return set_int(a.data_.i - b.data_.i);
    return set_float(to_double(a) - to_double(b));
}

static CVar op_mul(CVar a, CVar b) {
    if (__builtin_expect(a.type_ == VAR_INT && b.type_ == VAR_INT, 1))
        return set_int(a.data_.i * b.data_.i);
    return set_float(to_double(a) * to_double(b));
}

static CVar op_band(CVar a, CVar b) {
    return set_int(a.data_.i & b.data_.i);
}

static CVar op_bxor(CVar a, CVar b) {
    return set_int(a.data_.i ^ b.data_.i);
}

enum { NREG = 64 };
enum { NBOX = 32 };
enum { ROUNDS = 120000 };

static CVar regs[NREG];
static CVar *boxes[NBOX];
static CVar box_store[NBOX];

int main(void) {
    int i, r;
    unsigned acc;
    CVar tmp;

    for (i = 0; i < NREG; i++) {
        if ((i & 7) == 0)
            regs[i] = set_float((double)(i & 15));
        else if ((i & 7) == 1)
            regs[i] = set_bool(i & 1);
        else if ((i & 7) == 2)
            regs[i] = set_nil();
        else
            regs[i] = set_int((long long)(i * 17 + 3));
    }
    for (i = 0; i < NBOX; i++) {
        box_store[i] = set_int((long long)(i + 1));
        boxes[i] = &box_store[i];
    }

    acc = 0;
    for (r = 0; r < ROUNDS; r++) {
        for (i = 0; i < NREG; i++) {
            int j = (i + r) & (NREG - 1);
            int k = (i * 3 + r) & (NREG - 1);
            CVar a = regs[i];
            CVar b = regs[j];
            switch ((i + r) & 7) {
            case 0: tmp = op_add(a, b); break;
            case 1: tmp = op_sub(a, b); break;
            case 2: tmp = op_mul(a, b); break;
            case 3:
                if (a.type_ == VAR_INT && b.type_ == VAR_INT)
                    tmp = op_band(a, b);
                else
                    tmp = op_add(a, b);
                break;
            case 4:
                if (a.type_ == VAR_INT && b.type_ == VAR_INT)
                    tmp = op_bxor(a, b);
                else
                    tmp = op_sub(a, b);
                break;
            case 5:
                tmp = set_bool(is_true(a));
                break;
            case 6:
                *boxes[i & (NBOX - 1)] = a;
                tmp = *boxes[i & (NBOX - 1)];
                break;
            default:
                if (__builtin_expect(a.type_ == VAR_NIL, 0))
                    tmp = set_int((long long)k);
                else
                    tmp = a;
                break;
            }
            regs[k] = tmp;
            if (tmp.type_ == VAR_INT)
                acc += (unsigned)tmp.data_.i;
            else if (tmp.type_ == VAR_FLOAT)
                acc += (unsigned)(long long)tmp.data_.f;
            else if (tmp.type_ == VAR_BOOL)
                acc += (unsigned)tmp.data_.b;
        }
    }
    runtime.printf("fl_cvar %u\n", acc);
    return 0;
}
