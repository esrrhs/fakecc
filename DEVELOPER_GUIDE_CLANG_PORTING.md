# Porting Clang Test Cases to FakeCC E2E Suite

This document outlines the process for porting test cases from the LLVM Test Suite to FakeCC's end-to-end (E2E) test suite.

## Overview

FakeCC's E2E test suite primarily consists of tests ported from GCC's `gcc.c-torture` execute tests. To validate FakeCC's compatibility with modern Clang behavior, we can port relevant test cases from the LLVM Test Suite.

## Directory Structure

```
test/
└── llvm-test-suite-original/     # Vendored copy of LLVM Test Suite (reference only)
└── e2e/
    └── cases/
        ├── clang_unittest/       # Ported UnitTests from LLVM Test Suite
        ├── clang_benchmarks/     # Ported Benchmarks from LLVM Test Suite
        └── clang_multisource/    # Ported MultiSource tests from LLVM Test Suite
```

## Porting Process

### SingleSource/UnitTests (Recommended Starting Point)

These 100 test files are ideal initial targets because:
- Each is a self-contained, executable C program
- Each includes a `.reference_output` file with expected stdout
- They cover fundamental C language features, library functions, and edge cases
- They require minimal adaptation to fit FakeCC's test format

#### Conversion Steps

For each test in `llvm-test-suite-original/SingleSource/UnitTests/`:
1. Copy `.c` file to `test/e2e/cases/clang_unittest/`
2. Rename to `clang_unittest_<original_name>.c` (convert hyphens to underscores)
3. Add FakeCC-required header:
   ```c
   // expect: 0
   // expect_stdout: <content from .reference_output file>
   package main;
   // [Add necessary extern declarations for used libc functions]
   ```
4. Add necessary `extern` declarations for libc functions used in the test
5. Add required `typedef`s for standard types (size_t, ptrdiff_t, etc.)
6. Preserve the existing test code (should work unchanged)
7. Verify against the `.reference_output` file

#### Example

**Original:** `2002-04-17-PrintfChar.c` + `2002-04-17-PrintfChar.reference_output`
**Ported becomes:** `test/e2e/cases/clang_unittest/clang_unittest_20020417_PrintfChar.c`

Content:
```c
// expect: 0
// expect_stdout: 'c' 'e'
package main;

extern int printf(const char*, ...);

void printArgsNoRet(char a3, char* a5) {
  printf("'%c' '%c'\n", (int)a3, (int)*a5);
}

int main() {
  printArgsNoRet('c', "e");
  return 0;
}
```

### Handling Special Cases

1. **Tests expecting non-zero exit codes**:
   - Change `// expect: 0` to `// expect: <expected_code>`
   - Verify program exits with that code

2. **Tests requiring specific compiler flags**:
   - Add `// flags: <flags>` line (e.g., `// flags: -lm`)
   - These flags will be passed to fakecc during test execution

3. **Multi-file tests from MultiSource/**:
   - These require more work but can test important features like:
     - Cross-file function calls and variables
     - Package import system with complex dependencies
     - Interop scenarios with GCC-compiled object files
   - Each file needs proper `package` declarations
   - May need to use `import` mechanism
   - Preserve directory structure or flatten appropriately

### Verivation Strategy

1. After porting a batch of tests, run them through:
   ```bash
   bash test/e2e/run_e2e.sh v0/fakecc-1 -O0
   bash test/e2e/run_e2e.sh v0/fakecc-1 -O1
   ```
2. Compare actual output with expected output from `.reference_output` files
3. Track pass/fail rates and investigate failures
4. Failures may indicate:
   - Bugs in FakeCC (to be fixed)
   - Missing features in FakeCC (to be added)
   - Tests requiring features FakeCC intentionally omits (to be documented)

## Maintenance

- Regularly sync with upstream LLVM Test Suite
- Port new tests as they are added
- Monitor which Clang-specific tests pass/fail to guide FakeCC development
- Use failing tests as a roadmap for feature implementation

## Notes on Specific LLVM Test Suite Directories

### SingleSource/UnitTests
- Best starting point - 100 self-contained executables with reference outputs
- Map directly to FakeCC's existing E2E test format

### SingleSource/Benchmarks
- Performance-focused tests
- May require special handling for timing-sensitive code
- Consider creating a separate benchmark suite

### MultiSource/
- Larger programs with multiple source files
- Test complex features and interoperability
- May require CMake adjustments for building
- Valuable for testing optimization and code generation

## Conclusion

Porting LLVM Test Suite cases to FakeCC's E2E suite provides:
- Increased test coverage beyond GCC tortue tests
- Validation against modern Clang behavior
- A pathway to identify and prioritize FakeCC feature improvements
- Sustained compatibility with evolving C language standards