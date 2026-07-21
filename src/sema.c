#include "fakecc/sema.h"
#include "fakecc/common.h"

#include <string.h>

void sema_check(const TranslationUnit *tu) {
    /* package must be "main" */
    if (tu->package.name == NULL || strcmp(tu->package.name, "main") != 0) {
        die_at(tu->package.loc.file, tu->package.loc.line, tu->package.loc.col,
               "package must be 'main' in Slice 1");
    }

    /* must have exactly one function */
    if (tu->functions.len != 1) {
        die_at(tu->package.loc.file, tu->package.loc.line, tu->package.loc.col,
               "expected exactly one function in Slice 1");
    }

    /* function must be named "main" */
    const FunctionDecl *fn = &tu->functions.data[0];
    if (strcmp(fn->name, "main") != 0) {
        die_at(fn->loc.file, fn->loc.line, fn->loc.col,
               "function must be 'main' in Slice 1");
    }

    /* return value must be in [0, 255] */
    if (fn->body.value < 0 || fn->body.value > 255) {
        die_at(fn->body.loc.file, fn->body.loc.line, fn->body.loc.col,
               "return value %d out of range [0, 255]", fn->body.value);
    }
}
