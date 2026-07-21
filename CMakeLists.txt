cmake_minimum_required(VERSION 3.20)
project(fakecc C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter)

add_library(fakecc_core
    src/common.c
    src/lexer.c
    src/ast.c
    src/parser.c
    src/sema.c
    src/codegen.c
)
target_include_directories(fakecc_core PUBLIC include)

add_executable(fakecc src/main.c)
target_link_libraries(fakecc PRIVATE fakecc_core)

enable_testing()
add_subdirectory(test)
