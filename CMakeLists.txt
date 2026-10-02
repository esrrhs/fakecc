cmake_minimum_required(VERSION 3.20)
project(fakecc VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Place binaries and libraries according to GNU standards
include(GNUInstallDirs)

# RPATH configuration
set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter)
if(APPLE)
    add_compile_definitions(_DARWIN_C_SOURCE)
    add_compile_options(-Wno-deprecated-declarations)
endif()

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
    src/compiler.c
)

target_include_directories(fakecc_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/fakecc>
)

set_target_properties(fakecc_core PROPERTIES
    OUTPUT_NAME fakecc
    EXPORT_NAME fakecc
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
)

target_link_libraries(fakecc_core PUBLIC m)
if(USE_COV)
    target_link_libraries(fakecc_core PUBLIC gcov)
endif()

add_executable(fakecc src/main.c)
target_link_libraries(fakecc PRIVATE fakecc_core)

set_target_properties(fakecc PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
)

# ── Installation & CMake Package Export ──

# Install public headers
install(FILES include/fakecc.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/fakecc/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fakecc
    FILES_MATCHING PATTERN "*.h"
)

# Install runtime package sources for freestanding execution
install(DIRECTORY runtime/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fakecc/runtime
    FILES_MATCHING PATTERN "*.c"
)

# Install fakecc_core library
install(TARGETS fakecc_core
    EXPORT fakeccTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install fakecc executable
install(TARGETS fakecc
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/fakeccConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fakeccConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/fakeccConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fakecc"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

install(EXPORT fakeccTargets
    FILE fakeccTargets.cmake
    NAMESPACE fakecc::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fakecc"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/fakeccConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/fakeccConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fakecc"
)

# pkg-config support
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fakecc.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/fakecc.pc"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fakecc.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

enable_testing()
add_subdirectory(test)
