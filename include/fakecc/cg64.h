#ifndef FAKECC_CG64_H
#define FAKECC_CG64_H

/* IR → A64 code generator (arm64-apple-macos backend).
 *
 * The module-level stream begins with the freestanding LC_MAIN entry
 * stub (which calls main and exits with its return value), followed by
 * the IR functions.  All intra-module calls are direct BLs resolved via
 * local labels; no symbol/relocation table is needed at this stage. */

#include "fakecc/emit.h"
#include "fakecc/ir.h"

void codegen64(const IRModule *ir, EmitModule *out, int want_debug);

#endif /* FAKECC_CG64_H */
