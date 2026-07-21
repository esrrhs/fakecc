#include "fakecc/codegen.h"
#include "fakecc/common.h"

#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* x86-64 instruction encoding helpers                                 */
/* ------------------------------------------------------------------ */

static void emit_byte(Buffer *b, uint8_t val) {
    buffer_append(b, (const char *)&val, 1);
}

static void emit_int32(Buffer *b, int32_t val) {
    buffer_append(b, (const char *)&val, 4);
}

/* ------------------------------------------------------------------ */
/* Function prologue/epilogue                                          */
/* ------------------------------------------------------------------ */

/*
 * pushq %rbp          → 55
 * movq  %rsp, %rbp    → 48 89 e5
 */
static void emit_prologue(Buffer *b) {
    emit_byte(b, 0x55);                 /* pushq %rbp */
    emit_byte(b, 0x48);                 /* movq %rsp, %rbp */
    emit_byte(b, 0x89);
    emit_byte(b, 0xe5);
}

/*
 * popq %rbp           → 5d
 * ret                 → c3
 */
static void emit_epilogue(Buffer *b) {
    emit_byte(b, 0x5d);                 /* popq %rbp */
    emit_byte(b, 0xc3);                 /* ret */
}

/* ------------------------------------------------------------------ */
/* IR → machine code                                                   */
/* ------------------------------------------------------------------ */

void codegen(const IRModule *ir, EmitModule *out) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        const IRFunction *fn = &ir->functions.data[i];
        size_t start_offset = out->code.len;

        emit_prologue(&out->code);

        for (size_t j = 0; j < fn->insts.len; j++) {
            const IRInst *inst = &fn->insts.data[j];
            switch (inst->op) {
            case IR_RETURN:
                /* movl $<value>, %eax → b8 <imm32> */
                emit_byte(&out->code, 0xb8);
                emit_int32(&out->code, inst->value);
                break;
            }
        }

        emit_epilogue(&out->code);

        size_t fn_size = out->code.len - start_offset;
        emit_module_add_symbol(out, fn->name, start_offset, fn_size);
    }
}
