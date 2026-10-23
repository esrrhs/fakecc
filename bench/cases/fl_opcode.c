/* Fakelua interp-style dense opcode switch over an 8-byte Inst and a
 * register file of tagged values (MOVE/LOADK/ADD/GETTABLE/... subset).
 * Relative jumps are forward-only so the stream always terminates. */
package main;
import runtime;

enum {
    OP_MOVE = 0,
    OP_LOADK,
    OP_LOADNIL,
    OP_LOADBOOL,
    OP_GETUPVAL,
    OP_SETUPVAL,
    OP_GETTABLE,
    OP_SETTABLE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_BAND,
    OP_BXOR,
    OP_BOR,
    OP_SHL,
    OP_SHR,
    OP_EQ,
    OP_LT,
    OP_NOT,
    OP_LEN,
    OP_JMP,
    OP_TESTJMP,
    OP_CALL,
    OP_RETURN,
    OP_COUNT
};

typedef struct Inst {
    unsigned char op;
    unsigned char a;
    unsigned char b;
    unsigned char c;
    int sbx;
} Inst;

typedef struct CVar {
    int type_;
    int flag_;
    union {
        int b;
        long long i;
        unsigned long long u;
    } data_;
} CVar;

enum { NREG = 32 };
enum { NCODE = 1024 };
enum { ROUNDS = 8000 };

static Inst code[NCODE];
static CVar regs[NREG];
static CVar upvals[8];
static long long consts[64];
static CVar table_arr[64];

static void emit_prog(void) {
    int i;
    for (i = 0; i < NCODE; i++) {
        Inst ins;
        unsigned char op = (unsigned char)((i * 17 + 5) % OP_COUNT);
        ins.op = op;
        ins.a = (unsigned char)(i & (NREG - 1));
        ins.b = (unsigned char)((i * 3) & (NREG - 1));
        ins.c = (unsigned char)((i * 7) & (NREG - 1));
        /* Forward-only relative skip (0..3). */
        ins.sbx = (i & 3);
        code[i] = ins;
    }
}

int main(void) {
    int i, r, pc;
    unsigned acc;
    long long len;

    emit_prog();
    for (i = 0; i < 64; i++)
        consts[i] = (long long)(i * 13 + 1);
    for (i = 0; i < NREG; i++) {
        regs[i].type_ = 2;
        regs[i].flag_ = 0;
        regs[i].data_.i = (long long)(i + 1);
    }
    for (i = 0; i < 8; i++) {
        upvals[i].type_ = 2;
        upvals[i].flag_ = 0;
        upvals[i].data_.i = (long long)(100 + i);
    }
    for (i = 0; i < 64; i++) {
        table_arr[i].type_ = 2;
        table_arr[i].flag_ = 0;
        table_arr[i].data_.i = (long long)(i * i + 1);
    }

    acc = 0;
    for (r = 0; r < ROUNDS; r++) {
        pc = 0;
        while (pc < NCODE) {
            Inst ins = code[pc];
            CVar *ra = &regs[ins.a & (NREG - 1)];
            CVar *rb = &regs[ins.b & (NREG - 1)];
            CVar *rc = &regs[ins.c & (NREG - 1)];
            switch (ins.op) {
            case OP_MOVE:
                *ra = *rb;
                break;
            case OP_LOADK:
                ra->type_ = 2;
                ra->data_.i = consts[ins.b & 63];
                break;
            case OP_LOADNIL:
                ra->type_ = 0;
                ra->data_.i = 0;
                break;
            case OP_LOADBOOL:
                ra->type_ = 1;
                ra->data_.b = ins.b != 0;
                break;
            case OP_GETUPVAL:
                *ra = upvals[ins.b & 7];
                break;
            case OP_SETUPVAL:
                upvals[ins.b & 7] = *ra;
                break;
            case OP_GETTABLE:
                *ra = table_arr[(unsigned)(rb->data_.i + rc->data_.i) & 63u];
                break;
            case OP_SETTABLE:
                table_arr[(unsigned)(rb->data_.i) & 63u] = *rc;
                break;
            case OP_ADD:
                ra->type_ = 2;
                ra->data_.i = rb->data_.i + rc->data_.i;
                break;
            case OP_SUB:
                ra->type_ = 2;
                ra->data_.i = rb->data_.i - rc->data_.i;
                break;
            case OP_MUL:
                ra->type_ = 2;
                ra->data_.i = rb->data_.i * (rc->data_.i | 1);
                break;
            case OP_BAND:
                ra->type_ = 2;
                ra->data_.i = rb->data_.i & rc->data_.i;
                break;
            case OP_BXOR:
                ra->type_ = 2;
                ra->data_.i = rb->data_.i ^ rc->data_.i;
                break;
            case OP_BOR:
                ra->type_ = 2;
                ra->data_.i = rb->data_.i | rc->data_.i;
                break;
            case OP_SHL:
                ra->type_ = 2;
                ra->data_.i = (long long)((unsigned long long)rb->data_.i << ((unsigned)rc->data_.i & 31u));
                break;
            case OP_SHR:
                ra->type_ = 2;
                ra->data_.i = (long long)((unsigned long long)rb->data_.i >> ((unsigned)rc->data_.i & 31u));
                break;
            case OP_EQ:
                ra->type_ = 1;
                ra->data_.b = rb->data_.i == rc->data_.i;
                break;
            case OP_LT:
                ra->type_ = 1;
                ra->data_.b = rb->data_.i < rc->data_.i;
                break;
            case OP_NOT:
                ra->type_ = 1;
                ra->data_.b = !(rb->type_ != 0 && (rb->type_ != 1 || rb->data_.b));
                break;
            case OP_LEN:
                /* Cheap # proxy: array-part occupancy using sbx as start hint. */
                len = table_arr[(unsigned)ins.sbx & 63u].type_ != 0;
                ra->type_ = 2;
                ra->data_.i = len + (long long)(ins.a);
                break;
            case OP_JMP:
                pc += ins.sbx;
                break;
            case OP_TESTJMP:
                if ((ra->type_ != 0 && (ra->type_ != 1 || ra->data_.b)) == (ins.b != 0))
                    pc += ins.sbx;
                break;
            case OP_CALL:
                ra->type_ = 2;
                ra->data_.i = rb->data_.i + rc->data_.i + (long long)ins.a;
                break;
            case OP_RETURN:
            default:
                acc += (unsigned)ra->data_.i;
                break;
            }
            pc++;
            acc += (unsigned)regs[ins.a & (NREG - 1)].data_.i;
        }
    }
    runtime.printf("fl_opcode %u\n", acc);
    return 0;
}
