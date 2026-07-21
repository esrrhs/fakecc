#ifndef FAKECC_IR_H
#define FAKECC_IR_H

#include "fakecc/common.h"
#include <stddef.h>

/* IR instruction opcodes — will grow in later slices */
typedef enum {
    IR_RETURN,    /* return <value> */
} IROpcode;

typedef struct {
    IROpcode op;
    int value;        /* for IR_RETURN: the integer constant */
    SourceLoc loc;    /* for error reporting / debug info */
} IRInst;

typedef struct {
    IRInst *data;
    size_t len;
    size_t cap;
} IRInstArray;

typedef struct {
    char *name;       /* function name, xstrdup'd */
    IRInstArray insts;
    SourceLoc loc;
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
