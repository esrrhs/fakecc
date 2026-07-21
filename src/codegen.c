#include "fakecc/codegen.h"

void codegen(const IRModule *ir, Buffer *out) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        const IRFunction *fn = &ir->functions.data[i];

        buffer_appendf(out, "    .text\n");
        buffer_appendf(out, "    .globl %s\n", fn->name);
        buffer_appendf(out, "    .type %s, @function\n", fn->name);
        buffer_appendf(out, "%s:\n", fn->name);
        buffer_appendf(out, "    pushq %%rbp\n");
        buffer_appendf(out, "    movq %%rsp, %%rbp\n");

        for (size_t j = 0; j < fn->insts.len; j++) {
            const IRInst *inst = &fn->insts.data[j];
            switch (inst->op) {
            case IR_RETURN:
                buffer_appendf(out, "    movl $%d, %%eax\n", inst->value);
                break;
            }
        }

        buffer_appendf(out, "    popq %%rbp\n");
        buffer_appendf(out, "    ret\n");
        buffer_appendf(out, "    .section .note.GNU-stack,\"\",@progbits\n");
    }
}
