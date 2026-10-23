/* Fakelua VarClosure: func_ptr + upvalue pointers, many small callees,
 * indirect calls through closure handles (FlCallClosure-ish). */
package main;
import runtime;

typedef struct CVar {
    int type_;
    int flag_;
    union {
        long long i;
        struct CVar *p;
    } data_;
} CVar;

typedef CVar (*NativeFn)(CVar *up0, CVar *up1, CVar arg);

typedef struct VarClosure {
    NativeFn func_ptr;
    int upvalue_count;
    int expected_arg_count;
    int is_vararg;
    CVar *upvalues[2];
} VarClosure;

enum { NCL = 16 };
enum { N = 5000000 };

static CVar uv_store[NCL * 2];
static VarClosure clos[NCL];

static CVar fn_add(CVar *up0, CVar *up1, CVar arg) {
    CVar r;
    r.type_ = 2;
    r.flag_ = 0;
    r.data_.i = up0->data_.i + up1->data_.i + arg.data_.i;
    up0->data_.i = r.data_.i;
    return r;
}

static CVar fn_xor(CVar *up0, CVar *up1, CVar arg) {
    CVar r;
    r.type_ = 2;
    r.flag_ = 0;
    r.data_.i = up0->data_.i ^ up1->data_.i ^ arg.data_.i;
    up1->data_.i += arg.data_.i;
    return r;
}

static CVar fn_mul(CVar *up0, CVar *up1, CVar arg) {
    CVar r;
    r.type_ = 2;
    r.flag_ = 0;
    r.data_.i = up0->data_.i * ((up1->data_.i | 1) + (arg.data_.i & 7));
    return r;
}

static CVar fn_mix(CVar *up0, CVar *up1, CVar arg) {
    CVar r;
    long long x;
    r.type_ = 2;
    r.flag_ = 0;
    x = (up0->data_.i << 1) ^ up1->data_.i;
    x += arg.data_.i;
    up0->data_.i = x;
    r.data_.i = x;
    return r;
}

static NativeFn table[4] = { fn_add, fn_xor, fn_mul, fn_mix };

static CVar call_closure(VarClosure *cl, CVar arg) {
    return cl->func_ptr(cl->upvalues[0], cl->upvalues[1], arg);
}

int main(void) {
    int i;
    unsigned acc;
    CVar arg, res;

    for (i = 0; i < NCL; i++) {
        uv_store[i * 2].type_ = 2;
        uv_store[i * 2].flag_ = 0;
        uv_store[i * 2].data_.i = (long long)(i + 1);
        uv_store[i * 2 + 1].type_ = 2;
        uv_store[i * 2 + 1].flag_ = 0;
        uv_store[i * 2 + 1].data_.i = (long long)(i * 3 + 7);
        clos[i].func_ptr = table[i & 3];
        clos[i].upvalue_count = 2;
        clos[i].expected_arg_count = 1;
        clos[i].is_vararg = 0;
        clos[i].upvalues[0] = &uv_store[i * 2];
        clos[i].upvalues[1] = &uv_store[i * 2 + 1];
    }

    acc = 0;
    arg.type_ = 2;
    arg.flag_ = 0;
    arg.data_.i = 1;
    for (i = 0; i < N; i++) {
        VarClosure *cl = &clos[i & (NCL - 1)];
        arg.data_.i = (long long)((i * 17) & 255);
        res = call_closure(cl, arg);
        /* Occasionally rebind func_ptr like a fresh FlMakeClosure. */
        if ((i & 1023) == 0)
            cl->func_ptr = table[(i >> 10) & 3];
        acc += (unsigned)res.data_.i;
    }
    runtime.printf("fl_closure %u\n", acc);
    return 0;
}
