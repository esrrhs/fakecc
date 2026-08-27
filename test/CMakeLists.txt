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

# --- e2e tests ---
#
# Each suite runs once per optimization level.  -O0 keeps scalars in memory and
# -O1 promotes them to SSA, so the two take substantially different paths
# through mem2reg, the register allocator and the debug-info emitter; passing
# under one says little about the other.
foreach(lvl O0 O1)
    add_test(NAME e2e_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_e2e.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_test(NAME e2e_multi_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_multi_e2e.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # gcc-oracle differential suite over a curated e2e manifest.
    add_test(NAME difftest_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_difftest.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Shared-library linking: -l / -l: / -nodefaultlibs / .so path.
    add_test(NAME e2e_shlib_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_shlib_e2e.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Compile-only robustness suite (2,003 GCC edge-case tests).
    add_test(NAME compile_${lvl}
             COMMAND bash ${CMAKE_SOURCE_DIR}/test/compile/run_compile.sh $<TARGET_FILE:fakecc> -${lvl}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endforeach()

# DWARF against a real gdb.  Runs both -O0 and -O1 internally, since the
# expected answers differ between them.  Skipped gracefully if gdb is absent.
add_test(NAME e2e_gdb
         COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_gdb_e2e.sh $<TARGET_FILE:fakecc>
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
