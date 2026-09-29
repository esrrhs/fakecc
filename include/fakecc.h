#ifndef FAKECC_H
#define FAKECC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fakecc/common.h"
#include "fakecc/token.h"
#include "fakecc/lexer.h"
#include "fakecc/ast.h"
#include "fakecc/dfp.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/ir.h"
#include "fakecc/cfg.h"
#include "fakecc/domtree.h"
#include "fakecc/mem2reg.h"
#include "fakecc/scalar_opt.h"
#include "fakecc/regalloc.h"
#include "fakecc/opt.h"
#include "fakecc/codegen.h"
#include "fakecc/debug.h"
#include "fakecc/emit.h"
#include "fakecc/pkg.h"
#include "fakecc/compiler.h"

#ifdef __cplusplus
}
#endif

#endif /* FAKECC_H */
