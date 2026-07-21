# FakeCC

[![CI](https://github.com/esrrhs/fakecc/actions/workflows/ci.yml/badge.svg)](https://github.com/esrrhs/fakecc/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![Language: C99](https://img.shields.io/badge/language-C99-orange.svg)

A C-like systems programming language — keeping C's execution model and performance, removing the preprocessor, and replacing header files with modern package management.

## Design Goals

**FakeCC** retains full compatibility with C's execution model and performance characteristics, while removing the preprocessor and adopting a modern package system:

### What stays the same as C (syntax/semantics directly from C11)

- Declaration syntax: `int x;`, `int (*fp)(int)`, struct, union, enum, typedef, pointers, arrays
- Statements: `if/else/while/for/switch/return/break/continue/goto`
- Expressions: full operator set, precedence, implicit conversions, pointer arithmetic, lvalue/rvalue rules
- Type system: complete integer/floating-point/pointer/struct/union/enum/qualifier (const/volatile/restrict)
- Function signatures: `int main(int argc, char **argv)`
- Manual memory management (malloc/free), no GC
- ABI fully compatible with C, can call libc
- `sizeof` / `_Alignof` and other compile-time operators preserved

### What's different from C

| Feature | C | FakeCC |
|---|---|---|
| Preprocessor | `#include` / `#define` / `#if` / `#ifdef` / `#pragma` / line splicing `\` / trigraphs | **Entirely removed** |
| Macros | Object macros, function macros, `#`, `##`, `__VA_ARGS__` | **Do not exist** |
| Header files | `.h` files, forward declarations | **Eliminated** — compiler resolves symbols across files automatically; no forward declarations needed |
| Package organization | N/A | `package foo;` at top of each file |
| Importing | `#include` | `import "foo/bar";` |
| Visibility | All symbols visible | Capitalized first letter = exported (cross-package visible); lowercase = package-private |
| Constants | `#define MAX 100` | `const int MAX = 100;` |
| Conditional compilation | `#if` / `#ifdef` / `#ifndef` | Not supported in v1; cross-platform differences handled at runtime or via build tags |

### A minimal FakeCC program

```c
package main;

int main() {
    return 42;
}
```

### A cross-package example (not in Slice 1, illustrative only)

```c
// file: foo/util.c
package util;

int Add(int a, int b) {   // Capitalized → exported
    return a + b;
}

int helper(int x) {       // lowercase → package-private
    return x * 2;
}
```

```c
// file: main.c
package main;
import "foo/util";

int main() {
    return util.Add(1, 2);
}
```

## Bootstrapping Roadmap

FakeCC aims to **compile itself** in three stages:

- **Stage 0** — C99 implementation (current): Build `fakecc-c` with system gcc, strictly following coding constraints (no code-generation macros, no variadic macros, no conditional compilation except include guards)
- **Stage 1** — Mechanical translation to FakeCC: Write a one-time translation script to convert `src/*.c` into FakeCC source, compile with Stage 0 → `fakecc-1`
- **Stage 2** — Self-compilation verification: Use `fakecc-1` to compile the FakeCC source → `fakecc-2`; if `fakecc-2` passes all tests (or is byte-identical), bootstrapping succeeds

## Current Status

Slice 1 — minimal compiler supporting only:

```c
package main;
int main() { return N; }   // N is an integer literal 0-255
```

## Building

```bash
cmake -S . -B build
cmake --build build --parallel
```

## Running

```bash
./build/fakecc examples/return42.c -o /tmp/a.out
/tmp/a.out
echo $?    # 42
```

## Testing

```bash
ctest --test-dir build --output-on-failure
```
