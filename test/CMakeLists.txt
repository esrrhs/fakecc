# Test framework
add_library(test_framework
    test_framework.c
)
target_include_directories(test_framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# --- Unit tests ---

add_executable(test_lexer unit/test_lexer.c)
target_link_libraries(test_lexer PRIVATE fakecc_core test_framework)
add_test(NAME test_lexer COMMAND test_lexer)

add_executable(test_parser unit/test_parser.c)
target_link_libraries(test_parser PRIVATE fakecc_core test_framework)
add_test(NAME test_parser COMMAND test_parser)

add_executable(test_codegen unit/test_codegen.c)
target_link_libraries(test_codegen PRIVATE fakecc_core test_framework)
add_test(NAME test_codegen COMMAND test_codegen)

add_executable(test_ir unit/test_ir.c)
target_link_libraries(test_ir PRIVATE fakecc_core test_framework)
add_test(NAME test_ir COMMAND test_ir)

add_executable(test_sema unit/test_sema.c)
target_link_libraries(test_sema PRIVATE fakecc_core test_framework)
add_test(NAME test_sema COMMAND test_sema)

add_executable(test_emit unit/test_emit.c)
target_link_libraries(test_emit PRIVATE fakecc_core test_framework)
add_test(NAME test_emit COMMAND test_emit)

add_executable(test_cfg unit/test_cfg.c)
target_link_libraries(test_cfg PRIVATE fakecc_core test_framework)
add_test(NAME test_cfg COMMAND test_cfg)

add_executable(test_domtree unit/test_domtree.c)
target_link_libraries(test_domtree PRIVATE fakecc_core test_framework)
add_test(NAME test_domtree COMMAND test_domtree)

add_executable(test_phi unit/test_phi.c)
target_link_libraries(test_phi PRIVATE fakecc_core test_framework)
add_test(NAME test_phi COMMAND test_phi)

add_executable(test_renaming unit/test_renaming.c)
target_link_libraries(test_renaming PRIVATE fakecc_core test_framework)
add_test(NAME test_renaming COMMAND test_renaming)

add_executable(test_regalloc unit/test_regalloc.c)
target_link_libraries(test_regalloc PRIVATE fakecc_core test_framework)
add_test(NAME test_regalloc COMMAND test_regalloc)

add_executable(test_mem2reg unit/test_mem2reg.c)
target_link_libraries(test_mem2reg PRIVATE fakecc_core test_framework)
add_test(NAME test_mem2reg COMMAND test_mem2reg)

add_executable(test_opt unit/test_opt.c)
target_link_libraries(test_opt PRIVATE fakecc_core test_framework)
add_test(NAME test_opt COMMAND test_opt)

add_executable(test_link unit/test_link.c)
target_link_libraries(test_link PRIVATE fakecc_core test_framework)
add_test(NAME test_link COMMAND test_link)

add_executable(test_debug unit/test_debug.c)
target_link_libraries(test_debug PRIVATE fakecc_core test_framework)
add_test(NAME test_debug COMMAND test_debug)

add_executable(test_pkg unit/test_pkg.c)
target_link_libraries(test_pkg PRIVATE fakecc_core test_framework)
add_test(NAME test_pkg COMMAND test_pkg)

add_executable(test_compiler unit/test_compiler.c)
target_link_libraries(test_compiler PRIVATE fakecc_core test_framework)
add_test(NAME test_compiler COMMAND test_compiler)

add_executable(test_dfp unit/test_dfp.c)
target_link_libraries(test_dfp PRIVATE fakecc_core test_framework)
add_test(NAME test_dfp COMMAND test_dfp)

add_executable(test_common unit/test_common.c)
target_link_libraries(test_common PRIVATE fakecc_core test_framework)
add_test(NAME test_common COMMAND test_common)

add_executable(test_ast unit/test_ast.c)
target_link_libraries(test_ast PRIVATE fakecc_core test_framework)
add_test(NAME test_ast COMMAND test_ast)

add_executable(test_die_at unit/test_die_at.c)
target_link_libraries(test_die_at PRIVATE fakecc_core test_framework)
add_test(NAME test_die_at COMMAND test_die_at)

add_executable(test_rare unit/test_rare.c)
target_link_libraries(test_rare PRIVATE fakecc_core test_framework)
add_test(NAME test_rare COMMAND test_rare)

add_executable(test_target unit/test_target.c)
target_link_libraries(test_target PRIVATE fakecc_core test_framework)
add_test(NAME test_target COMMAND test_target)

add_executable(test_a64 unit/test_a64.c)
target_link_libraries(test_a64 PRIVATE fakecc_core test_framework)
add_test(NAME test_a64 COMMAND test_a64)

add_executable(test_macho unit/test_macho.c)
target_link_libraries(test_macho PRIVATE fakecc_core test_framework)
add_test(NAME test_macho COMMAND test_macho)

add_executable(test_cg64_native unit/test_cg64_native.c)
target_link_libraries(test_cg64_native PRIVATE fakecc_core test_framework)
add_test(NAME test_cg64_native COMMAND test_cg64_native)

# --- app_ports tests ---
# Each application port runs once per optimization level to cover both the
# -O0 (scalars in memory) and -O1 (SSA) compiler paths.
foreach(lvl O0 O1)
    add_test(NAME app_ports_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/app_ports/run_all.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endforeach()


# --- e2e tests ---
#
# Each suite runs once per optimization level.  -O0 keeps scalars in memory and
# -O1 promotes them to SSA, so the two take substantially different paths
# through mem2reg, the register allocator and the debug-info emitter; passing
# under one says little about the other.
# The backend this host cannot execute.  fakecc emits either object format from
# either host, but only one of them can be *run* here, so the cross-target
# suite is compile-only by necessity -- which is exactly why the compile-only
# corpus is the right one for it.
#
# This is not redundant with the host run, it is the other half of it.  Measured
# on arm64, --target=x86_64-linux takes src/codegen.c from 0.10% to 70% line
# coverage while cg64/a64/macho fall to zero -- the precise mirror of the host
# run.  A single platform can therefore never see both backends' real numbers,
# and a codegen bug in the non-host backend used to have no automated coverage
# at all (the only check was hand-running objdump on one binary).
if(APPLE)
    set(CROSS_TARGET "x86_64-linux")
else()
    set(CROSS_TARGET "arm64-macos")
endif()

foreach(lvl O0 O1)
    add_test(NAME e2e_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_e2e.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_test(NAME e2e_multi_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_multi_e2e.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # gcc-oracle differential suite over a curated e2e manifest.  Every
    # platform installs a host gcc in CI (Homebrew on macOS), so the oracle
    # is the same everywhere -- ctest stays 1:1 across Linux and macOS.
    add_test(NAME difftest_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_difftest.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Shared-library linking: -l / -l: / -nodefaultlibs / .so path.
    add_test(NAME e2e_shlib_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_shlib_e2e.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Mach-O counterpart of the shared-library suite: -dynamiclib,
    # -install_name, LC_LOAD_DYLIB and LC_RPATH.  The script refuses to run
    # off Darwin on its own, so keeping it registered here leaves the ctest
    # roster identical on both platforms while the cases are real on macOS
    # (where e2e_shlib_* has nothing but ELF checks to skip).
    add_test(NAME e2e_dylib_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_dylib_e2e.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Compile-only robustness suite (2,003 GCC edge-case tests).
    add_test(NAME compile_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/compile/gcc_compile/run_compile.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Same corpus, opposite backend.  Compile-only, so it is immune to the one
    # thing that makes cross-compiled output untestable: you cannot execute it
    # here.  ~26 s for 1658 cases.
    add_test(NAME compile_cross_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/compile/gcc_compile/run_compile.sh $<TARGET_FILE:fakecc> --target=${CROSS_TARGET} -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endforeach()

# Install layout.  Every test above drives fakecc out of the build tree, where
# the runtime package sits next to the sources; an installed compiler reaches
# it through a different branch of the search entirely.  Nothing else here can
# notice when that breaks, so install into a scratch prefix and use the result.
add_test(NAME install_smoke
         COMMAND bash ${CMAKE_SOURCE_DIR}/test/install_smoke.sh ${CMAKE_BINARY_DIR}
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# DWARF against a real gdb.  Runs both -O0 and -O1 internally, since the
# expected answers differ between them.  Skipped gracefully if gdb is absent.
add_test(NAME e2e_gdb
         COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_gdb_e2e.sh $<TARGET_FILE:fakecc>
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# DWARF against a real lldb, the macOS counterpart of the gdb suite above.
# Same deal as e2e_dylib: registered everywhere, meaningful on Darwin.
add_test(NAME e2e_lldb
         COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_lldb_e2e.sh $<TARGET_FILE:fakecc>
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_test(NAME e2e_cli
         COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_cli_e2e.sh $<TARGET_FILE:fakecc>
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
