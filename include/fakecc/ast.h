#ifndef FAKECC_AST_H
#define FAKECC_AST_H

#include "fakecc/token.h"
#include <stddef.h>

typedef struct {
    int value;
    SourceLoc loc;
} ReturnStmt;

typedef struct {
    char *name;         /* strdup'd */
    ReturnStmt body;
    SourceLoc loc;
} FunctionDecl;

typedef struct {
    char *name;         /* strdup'd */
    SourceLoc loc;
} PackageDecl;

typedef struct {
    FunctionDecl *data;
    size_t len;
    size_t cap;
} FunctionArray;

typedef struct {
    PackageDecl package;
    FunctionArray functions;
} TranslationUnit;

void tu_init(TranslationUnit *tu);
void tu_free(TranslationUnit *tu);

#endif /* FAKECC_AST_H */
