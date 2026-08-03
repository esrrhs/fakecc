#ifndef FAKECC_IR_H
#define FAKECC_IR_H

#include "fakecc/common.h"
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* SSA virtual register — grows monotonically per function             */
/* ------------------------------------------------------------------ */

typedef int IRValue;   /* SSA virtual register id, incrementing from 0 */

/* ------------------------------------------------------------------ */
/* IR instruction opcodes                                              */
/* ------------------------------------------------------------------ */

typedef enum {
    IR_CONST,       /* dst = imm */
    IR_ADD,         /* dst = a + b */
    IR_SUB,         /* dst = a - b */
    IR_MUL,         /* dst = a * b */
    IR_DIV,         /* dst = a / b  (signed) */
    IR_MOD,         /* dst = a % b  (signed) */
    IR_NEG,         /* dst = -a */
    IR_EQ,          /* dst = (a == b) ? 1 : 0 */
    IR_NE,          /* dst = (a != b) ? 1 : 0 */
    IR_LT,          /* dst = (a <  b) ? 1 : 0 (signed) */
    IR_LE,          /* dst = (a <= b) ? 1 : 0 (signed) */
    IR_GT,          /* dst = (a >  b) ? 1 : 0 (signed) */
    IR_GE,          /* dst = (a >= b) ? 1 : 0 (signed) */
    IR_ALLOCA,      /* dst = stack slot for a variable (codegen no-op) */
    IR_LOAD,        /* dst = [a]   — read variable slot a → dst */
    IR_STORE,       /* [a] = b    — write b into variable slot a; dst unused */
    IR_COPY,        /* dst = a    — simple move (mem2reg / φ resolution product) */
    IR_LABEL,       /* imm = label_id — basic-block marker */
    IR_BR,          /* imm = target_label — unconditional branch */
    IR_CBR,         /* a = cond, imm = true_label, b = false_label — conditional branch */
    IR_PARAM,       /* dst = incoming param; imm = param_index (0..5) */
    IR_CALL,        /* dst = callee(call.args[0..nargs-1]); callee name in call.name */
    IR_RETURN,      /* return a */
} IROpcode;

/* Maximum arguments to IR_CALL — matches Slice 6 restriction of 6 params
 * (System V AMD64 first six int-class regs: rdi rsi rdx rcx r8 r9). */
#define IR_CALL_MAX_ARGS 6

typedef struct {
    IROpcode op;
    IRValue  dst;      /* meaningless for IR_RETURN/IR_BR/IR_LABEL, fill -1 */
    IRValue  a, b;     /* source operands; IR_CONST only uses imm */
    int      imm;      /* CONST value, LABEL id, BR/CBR target, PARAM index */
    SourceLoc loc;
    /* IR_CALL only: callee name + argument SSA values.  NULL for other ops. */
    char    *call_name;
    IRValue  call_args[IR_CALL_MAX_ARGS];
    int      call_nargs;
} IRInst;

/* ------------------------------------------------------------------ */
/* IR function & module                                                */
/* ------------------------------------------------------------------ */

typedef struct {
    IRInst *data;
    size_t len;
    size_t cap;
} IRInstArray;

typedef struct {
    char *name;       /* function name, xstrdup'd */
    IRInstArray insts;
    int next_value_id; /* SSA id counter, incremented by lower_expr */
    int next_label_id; /* label id counter, used by control-flow lowering */
    SourceLoc loc;
    void *ra;         /* RAResult*, set by reg_alloc, consumed by codegen.
                         NULL until register allocation runs. */
} IRFunction;

typedef struct {
    IRFunction *data;
    size_t len;
    size_t cap;
} IRFunctionArray;

typedef struct {
    IRFunctionArray functions;
} IRModule;

void ir_module_init(IRModule *m);
void ir_module_free(IRModule *m);

/* Lower AST to IR */
#include "fakecc/ast.h"
void ir_generate(const TranslationUnit *tu, IRModule *ir);

#endif /* FAKECC_IR_H */
