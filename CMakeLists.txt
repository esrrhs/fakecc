cmake_minimum_required(VERSION 3.20)
project(fakecc C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter)

option(ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined -g)
    add_link_options(-fsanitize=address,undefined)
endif()

option(USE_COV "use cov" OFF)
if(USE_COV)
    message("build with cov")
    add_compile_options(-fprofile-arcs -ftest-coverage -O0)
    add_link_options(-fprofile-arcs -ftest-coverage)
endif()

add_library(fakecc_core
    src/common.c
    src/lexer.c
    src/ast.c
    src/dfp.c
    src/parser.c
    src/sema.c
    src/ir.c
    src/cfg.c
    src/domtree.c
    src/mem2reg.c
    src/scalar_opt.c
    src/regalloc.c
    src/opt.c
    src/codegen.c
    src/debug.c
    src/emit.c
    src/link.c
    src/pkg.c
)
target_include_directories(fakecc_core PUBLIC include)
target_link_libraries(fakecc_core PUBLIC m)
if(USE_COV)
    target_link_libraries(fakecc_core PUBLIC gcov)
endif()

add_executable(fakecc src/main.c)
target_link_libraries(fakecc PRIVATE fakecc_core)

enable_testing()
add_subdirectory(test)
