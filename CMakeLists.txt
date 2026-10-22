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

if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter)
endif()
if(APPLE)
    add_compile_definitions(_DARWIN_C_SOURCE)
    add_compile_options(-Wno-deprecated-declarations)
endif()

# Sanitizers are selected with CMAKE_C_FLAGS (see the ASan legs in
# .github/workflows/ci.yml), which reaches the link line too.  There used to be
# an ENABLE_ASAN option for the same thing; nothing referenced it, and a second
# switch for one setting is worse than none -- it reads as the canonical way in
# when it is not wired to anything.
option(USE_COV "use cov" OFF)
if(USE_COV)
    message("build with cov")
    # Two flavours of instrumentation, one per compiler, because the two
    # platforms have opposite halves of the toolchain: Linux has gcov and lcov
    # but builds fakecc with gcc, while macOS has clang (and a matching
    # llvm-profdata/llvm-cov in the Command Line Tools) and no lcov at all.
    #
    # The point of the macOS leg is coverage the Linux leg structurally cannot
    # produce: cg64.c, a64.c and macho.c only execute when the compiler is
    # running on arm64, so on Linux they are permanently 0%.
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping -O0)
        add_link_options(-fprofile-instr-generate)
    else()
        add_compile_options(-fprofile-arcs -ftest-coverage -O0)
        add_link_options(-fprofile-arcs -ftest-coverage)
    endif()
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
    src/reg_arm64.c
    src/opt.c
    src/target.c
    src/a64.c
    src/codegen.c
    src/cg64.c
    src/debug.c
    src/emit.c
    src/link.c
    src/macho.c
    src/pe.c
    src/coff.c
    src/arcoff.c
    src/sha256.c
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

if(NOT WIN32)
    target_link_libraries(fakecc_core PUBLIC m)
endif()
if(USE_COV)
    # Same split as the flags above: gcc's gcov-format instrumentation needs an
    # explicit -lgcov, while clang's -fprofile-instr-generate already pulls in
    # its own profiler runtime.  Linking gcov under clang fails outright --
    # there is no libgcov on macOS -- so it has to be conditional, not assumed.
    if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_link_libraries(fakecc_core PUBLIC gcov)
    endif()
endif()

add_executable(fakecc src/main.c)
target_link_libraries(fakecc PRIVATE fakecc_core)

# Windows PE default stack is 1MB.  ASan frames blow through it while the host
# compiler parses large modules (cg64.c etc.), which shows up as
# AddressSanitizer stack-overflow / exit 139 on almost every compile.  pe.c
# already stamps 8MB on binaries fakecc emits; the clang-built host needs the
# same via the link line.  target_link_options covers fakecc (already added
# above); add_link_options covers later test exes.  16MB: 8MB still overflowed
# under clang ASan locally while compiling examples/return0.c.
if(WIN32 AND NOT MSVC)
    if(CMAKE_C_FLAGS MATCHES "-fsanitize")
        target_link_options(fakecc PRIVATE -Wl,--stack,16777216)
        add_link_options(-Wl,--stack,16777216)
    endif()
endif()

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
