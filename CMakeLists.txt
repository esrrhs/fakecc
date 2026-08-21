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

add_library(fakecc_core
    src/common.c
    src/lexer.c
    src/ast.c
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

add_executable(fakecc src/main.c)
target_link_libraries(fakecc PRIVATE fakecc_core)

enable_testing()
add_subdirectory(test)
