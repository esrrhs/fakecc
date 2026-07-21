#include "fakecc/codegen.h"

void codegen(const TranslationUnit *tu, Buffer *out) {
    const FunctionDecl *fn = &tu->functions.data[0];

    buffer_appendf(out, "    .text\n");
    buffer_appendf(out, "    .globl %s\n", fn->name);
    buffer_appendf(out, "    .type %s, @function\n", fn->name);
    buffer_appendf(out, "%s:\n", fn->name);
    buffer_appendf(out, "    pushq %%rbp\n");
    buffer_appendf(out, "    movq %%rsp, %%rbp\n");
    buffer_appendf(out, "    movl $%d, %%eax\n", fn->body.value);
    buffer_appendf(out, "    popq %%rbp\n");
    buffer_appendf(out, "    ret\n");
    buffer_appendf(out, "    .section .note.GNU-stack,\"\",@progbits\n");
}
