#ifndef FAKECC_COMPILER_H
#define FAKECC_COMPILER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compilation options for FakeCC high-level compiler APIs */
typedef struct {
    int opt_level;              /* Optimization level: 0 (-O0) or 1 (-O1, default) */
    int want_debug;             /* 1 to emit DWARF debug info, 0 otherwise (default: 0) */
    int nostdlib;               /* 1 to omit linking builtin runtime/ (default: 0 for exe, 1 for .so) */
    const char *rt_dir;         /* Path to runtime/ directory (NULL for auto-detection) */
    const char **needed;        /* Array of shared libraries to link against, e.g. "m" (or NULL) */
    size_t num_needed;          /* Number of libraries in needed array */
    const char **lib_paths;     /* Array of library search directories (-L paths, or NULL) */
    size_t num_lib_paths;       /* Number of paths in lib_paths array */
} FakeccOptions;

/* Initialize FakeccOptions with default values (-O1, debug off, default stdlib behavior). */
void fakecc_options_init(FakeccOptions *opts);

/* Compiles a C source string into an ELF shared library (.so).
 * Returns 0 on success, non-zero on error. */
int fakecc_compile_string_to_so(const char *source,
                                const char *output_so_path,
                                const FakeccOptions *opts);

/* Compiles a C source string into a standalone executable.
 * Returns 0 on success, non-zero on error. */
int fakecc_compile_string_to_executable(const char *source,
                                        const char *output_path,
                                        const FakeccOptions *opts);

/* Compiles a C source string into an ELF relocatable object file (.o).
 * Returns 0 on success, non-zero on error. */
int fakecc_compile_string_to_obj(const char *source,
                                 const char *output_obj_path,
                                 const FakeccOptions *opts);

/* Compiles a C source file into an ELF shared library (.so).
 * Returns 0 on success, non-zero on error. */
int fakecc_compile_file_to_so(const char *source_path,
                              const char *output_so_path,
                              const FakeccOptions *opts);

/* Compiles a C source file into a standalone executable.
 * Returns 0 on success, non-zero on error. */
int fakecc_compile_file_to_executable(const char *source_path,
                                      const char *output_path,
                                      const FakeccOptions *opts);

/* Compiles a C source file into an ELF relocatable object file (.o).
 * Returns 0 on success, non-zero on error. */
int fakecc_compile_file_to_obj(const char *source_path,
                               const char *output_obj_path,
                               const FakeccOptions *opts);

/* Compiles multiple C source files and/or .o files into an executable or shared library.
 * is_shared: 1 for shared library (.so), 0 for executable.
 * Returns 0 on success, non-zero on error. */
int fakecc_compile_files(const char **input_paths,
                         size_t num_inputs,
                         const char *output_path,
                         int is_shared,
                         const FakeccOptions *opts);

#ifdef __cplusplus
}
#endif

#endif /* FAKECC_COMPILER_H */
