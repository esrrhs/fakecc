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

# --- e2e tests ---

add_test(NAME e2e
         COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_e2e.sh $<TARGET_FILE:fakecc>
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
