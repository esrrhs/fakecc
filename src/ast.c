#include "fakecc/ast.h"

#include <stdlib.h>
#include <string.h>

void tu_init(TranslationUnit *tu) {
    tu->package.name = NULL;
    tu->package.loc.file = NULL;
    tu->package.loc.line = 0;
    tu->package.loc.col = 0;
    tu->functions.data = NULL;
    tu->functions.len = 0;
    tu->functions.cap = 0;
}

void tu_free(TranslationUnit *tu) {
    free(tu->package.name);
    for (size_t i = 0; i < tu->functions.len; i++) {
        free(tu->functions.data[i].name);
    }
    free(tu->functions.data);
}
