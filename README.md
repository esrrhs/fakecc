# FakeCC

[![CI](https://github.com/esrrhs/fakecc/actions/workflows/ci.yml/badge.svg)](https://github.com/esrrhs/fakecc/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![Language: C99 + GCC Extensions](https://img.shields.io/badge/language-C99%20%2B%20GCC-orange.svg)
![Self-hosting](https://img.shields.io/badge/self--hosting-yes-brightgreen.svg)
![Freestanding](https://img.shields.io/badge/default-zero--dep-brightgreen.svg)

一门类 C 的系统级编程语言——**完整支持 C99 标准及主流 GCC 语言扩展**，保留 C 语言高效直接的底层执行模型与内存布局，**彻底摒弃 C 语言的预处理器宏（`#define`）与文本头文件（`#include`），转而采用类似 Go 语言的现代 Package 模块体系**，自带轻量零依赖 runtime。

- **已完全自举**：fakecc 能编译自身源码，两级自举生成产物逐字节完全一致（Fixed Point）；
- **默认零依赖**：用户程序与自举产物默认不链接系统 libc，直接生成静态独立 ELF64 可执行文件；
- **高严苛测试覆盖**：忠实移植了 GCC 官方 **`gcc.c-torture/execute` 960+ 个完整执行用例**，全量 1680+ E2E 测试在 `-O0` 和 `-O1` 下 100% 全部通过。

---

## 核心设计与语言特性

### 1. 现代 Package 模块系统（替代宏与头文件）

传统 C 语言依赖预处理器进行文本级拼接（`#include`）与宏展开（`#define`），极易引发命名污染、符号冲突以及编译膨胀。FakeCC 彻底移除了预处理器，改用现代模块系统：

- **文件声明包名**：每个源文件顶部通过 `package pkg_name;` 声明所属包；
- **同包自动互见**：同一个 package 目录下的所有源文件自动共享 typedef、struct、enum 和全局声明，无需维护头文件；
- **包间导入**：跨包调用使用 `import other_pkg;`，在代码中通过限定名访问：
  ```c
  package main;
  import runtime;

  int main(void) {
      runtime.printf("hello %d\n", 42);
      return 0;
  }
  ```
- **包级作用域**：`static` 声明将符号限制在当前包内，外部不可见；非 static 符号导出供其他包使用。

---

### 2. 完整 C99 核心语言支持

FakeCC 完整实现了 C99 标准规范的核心语法与语义特性：

- **基础类型系统**：
  - 整型：`char` / `short` / `int` / `long` / `long long`（及其 `unsigned` 变体）、`_Bool`、`__int128`
  - 浮点：`float`、`double`（SSE 浮点指令）、`long double`（80 位扩展精度 x87 FPU）
  - 复数类型：`_Complex float`、`_Complex double`、`_Complex long double`
  - 派生类型：多级指针、定长数组、多维数组、`struct`、`union`、`enum`、函数指针
  - 类型修饰符：`const`、`volatile`、`restrict`、`inline`
- **C99 进阶特性**：
  - **变长数组（VLA）**：支持运行时动态长度数组分配与多维 VLA（如 `int arr[n][m]`），支持在循环与复杂控制流中跨作用域自动回收栈空间；
  - **复合字面量（Compound Literals）**：如 `(struct Point){ .x = 1, .y = 2 }` 或 `(int[]){ 1, 2, 3 }`；
  - **指定初始化器（Designated Initializers）**：结构体成员指定初始化 `{.field = val}`、数组下标指定初始化 `{[3] = val}` 以及嵌套初始化；
  - **灵活数组成员（Flexible Array Members）**：结构体末尾的 `type array[]` 柔性数组；
  - **声明位置自由**：支持在代码块任意位置声明局部变量，以及 `for (int i = 0; i < n; i++)` 循环头局部变量声明。
- **表达式与控制流**：
  - 完整运算符优先级与隐式类型提升/转换规则（Integer Promotion、Arithmetic Conversions）；
  - `if` / `else`、`switch`（支持多 case/default 与任意跨作用域跳转）、`while`、`do-while`、`for`、`goto`、`break`、`continue`、`return`。

---

### 3. 主流 GCC 扩展支持

为了无缝运行底层系统级代码与复杂的开源测试集，FakeCC 深入支持了主流的 GCC 编译器扩展：

- **语句表达式（Statement Expressions）**：`({ int x = f(); x * 2; })` 允许在表达式内嵌入代码块并返回值；
- **计算跳转（Computed Gotos）**：支持取标签地址 `&&label` 与间接跳转 `goto *expr;`，支持静态标签跳转表；
- **GCC 属性系统（`__attribute__`）**：
  - 对齐与打包：`aligned(N)`、`packed`
  - 别名与重命名：`alias("target")`、`__asm__("symbol")`
  - 向量类型：`vector_size(N)`
  - 字节序调整：`scalar_storage_order("big-endian" / "little-endian")`
  - 函数剖析控制：`no_instrument_function`
  - 机器模式：`mode(QI/HI/SI/DI/TI/word/byte)`
- **SIMD 向量扩展**：支持 `__attribute__((vector_size(N)))` 定义的向量类型，支持按元素向量加减乘除、位运算、按元素比较生成全 1/全 0 掩码以及向量初始化；
- **初始化扩展**：数组范围指定初始化器（如 `[0 ... 9] = 1`）；
- **函数级性能剖析**：支持 `-finstrument-functions` 编译选项，在非 `no_instrument_function` 函数的出入口自动注入 `__cyg_profile_func_enter` 与 `__cyg_profile_func_exit` 钩子调用；
- **丰富的内置函数（GCC Builtins）**：
  - 控制流与优化提示：`__builtin_expect`, `__builtin_unreachable`, `__builtin_constant_p`, `__builtin_trap`
  - 位运算指令：`__builtin_clz/clzl/clzll`, `__builtin_ctz/ctzl/ctzll`, `__builtin_popcount/popcountll`, `__builtin_ffs/ffsll`, `__builtin_bswap16/32/64`
  - 溢出安全算术：`__builtin_add_overflow`, `__builtin_sub_overflow`, `__builtin_mul_overflow`（支持 8/16/32/64 位有符号与无符号全类型及 `_p` 变体）
  - 栈与执行上下文：`__builtin_frame_address`, `__builtin_return_address`, `__builtin_stack_save`, `__builtin_stack_restore`, `__builtin_setjmp`, `__builtin_longjmp`, `__builtin_alloca`
  - 内存与标准运算：`__builtin_memcpy`, `__builtin_memset`, `__builtin_memcmp`, `__builtin_abs`, `__builtin_labs`, `__builtin_llabs`, `__builtin_copysign` 等。

---

## 编译器架构

```
源码 (.c) → Lexer → Tokens → Parser → AST → Sema → SSA IR → 中端优化 (Opt) → Codegen (x86-64) → 内嵌链接器 → ELF64 可执行文件
```

- **前端（Frontend）**：
  - 手写递归下降解析器，直接构建 AST；
  - 语义分析（Sema）负责类型检查、隐式转换插入、常量折叠与作用域符号表解析；
  - AST 降级为中端统一三地址 SSA IR。
- **中端（Middle-end）**：
  - **控制流图（CFG）与支配树（Dominator Tree）**；
  - **mem2reg**：基于支配前沿（Dominance Frontiers）自动插入 φ 函数，将栈上的局部标量变量提升至 SSA 寄存器；
  - **标量优化**：常量折叠（Constant Folding）、代数化简、无效代码消除（DCE）、窥孔优化（Peephole）；
  - **寄存器分配**：基于图着色 / 线性扫描的 SysV AMD64 寄存器分配器（支持 GP 与 XMM 寄存器分配、溢出处理与调用约定保存）。
- **后端与链接器（Backend & Linker）**：
  - 原生 x86-64 机器码生成器（不依赖外部 GNU `as`）；
  - 内嵌 ELF64 文件生成器与静态/动态链接器（不依赖 GNU `ld`），直接写出可执行 ELF。

---

## 零依赖 Runtime（`runtime/`）

FakeCC 源码树自带纯 C / FakeCC 实现的独立 runtime（位于 `runtime/` 目录），默认编译时自动将其静态编译并链接入最终二进制中。

| 模块 | 包含的核心能力 |
|---|---|
| `builtin.c` | 基础类型定义（`size_t`, `ssize_t`, `FILE`, `va_list`） |
| `string.c` | `memcpy`, `memmove`, `memset`, `memcmp`, `strlen`, `strcpy`, `strcmp`, `strchr` 等 |
| `ctype.c` | `isdigit`, `isalpha`, `isspace`, `toupper`, `tolower` 等 |
| `malloc.c` | 基于 Linux `mmap` 系统调用的独立内存分配器：`malloc`, `free`, `calloc`, `realloc` |
| `stdio.c` | `stdin`/`stdout`/`stderr` 标准流、用户态 I/O 缓冲区、`fopen`, `fclose`, `fread`, `fwrite`, `fputs` 等 |
| `printf.c` | `printf`, `fprintf`, `sprintf`, `snprintf`, `vprintf`, `vfprintf`, `vsnprintf` |
| `stdlib.c` | `exit`, `abort`, `strtol`, `strtoul`, `qsort`, `getenv`, `abs` |

- **与宿主系统互操作**：
  - 使用 `-nostdlib` 可禁用内置 runtime；
  - 搭配 `-lc` / `-lm` 可与系统 glibc / libm 等标准共享库无缝链接互操作。

---

## 自举流程

FakeCC 的自举验证机制保证了编译器自身的逻辑自洽性与稳定性：

```
[ Stage 0 ] gcc 编译 src/*.c                  → build/fakecc
     ↓
[ Stage 1 ] build/fakecc 编译 v0/*.c + runtime → v0/fakecc-1
     ↓
[ Stage 2 ] v0/fakecc-1 再次编译 v0/*.c + runtime → v0/fakecc-2
     ↓
[ 验证 ]   比对 v0/fakecc-1 与 v0/fakecc-2（逐字节 100% 完全一致）
```

运行自举检查：
```bash
v0/stage2_check.sh
```

---

## 构建

```bash
cmake -S . -B build
cmake --build build --parallel
```

## 运行

```bash
./build/fakecc examples/return42.c -o /tmp/a.out
/tmp/a.out; echo $?    # 42

# 带 DWARF，可用 gdb 按行调试（不影响优化，也不影响生成的指令）
./build/fakecc -g examples/return42.c -o /tmp/a.out

# 变量全部留在栈上，调试体验最直白（代价是更慢）
./build/fakecc -O0 -g examples/return42.c -o /tmp/a.out

# 可选：不用 rt，改链系统 libc
./build/fakecc hello.c -nostdlib -lc -o /tmp/hello_libc
```

## 测试

```bash
ctest --test-dir build --output-on-failure
# 单元测试 + 四类 e2e（单文件 / 多文件 / 共享库 -l / gcc difftest）各跑 -O0 -O1 两轮
# 外加 gdb 调试信息套件（内部自己跑两级）

bash v0/stage2_check.sh                        # 自举不动点
bash test/e2e/run_e2e.sh v0/fakecc-1 -O0       # 再用自举产物验证一遍
bash test/e2e/run_e2e.sh v0/fakecc-1 -O1
bash test/e2e/run_gdb_e2e.sh v0/fakecc-1       # 需要 gdb
```

每个 e2e 套件都跑 `-O0` 和 `-O1` 两轮，因为两级走的是很不一样的路径：`-O0` 让标量留在内存里，`-O1` 把它们提升成 SSA 再做寄存器分配，一级通过说明不了另一级。这一点不是理论顾虑——分级跑起来第一次就抓出了一个只在 `-O1` 出错的实参传递 bug（宽字面量传给较窄的无符号形参时被符号扩展，现已在 sema 于调用处按形参类型插入隐式转换修复，回归用例 `types/param_narrow_unsigned.c`）。小结构体按值传参/返回已按 SysV AMD64 分类（≤16 走寄存器，更大走栈），多文件套件含与 gcc `.o` 的互操作用例。

用例按语言特性分目录，加用例就是往对应目录里丢文件，runner 递归发现：

```
test/e2e/
├── run_e2e.sh / run_multi_e2e.sh / run_difftest.sh / run_shlib_e2e.sh / run_gdb_e2e.sh
├── difftest_manifest.txt        # difftest 取用的用例清单（category/name.c）
└── cases/
    ├── basics/        return / 变量 / 字面量
    ├── operators/     算术、位运算、比较、逻辑、三元、自增自减、复合赋值、强制转换
    ├── control_flow/  if / while / for / do-while / switch / goto / 嵌套循环
    ├── types/         整型宽度与符号、typedef、enum、sizeof / alignof、const / volatile
    ├── floats/        float / double / long double
    ├── aggregates/    数组、struct、union、位域、指定初始化、复合字面量
    ├── pointers/      指针、多级指针、函数指针
    ├── functions/     调用与递归、多参数、变参、argc/argv
    ├── chars_strings/ 字符、转义、字符串
    ├── linkage/       global / static / extern
    ├── runtime/       libc 动态链接、裸系统调用
    ├── codegen/       寄存器分配与窄类型高位的回归用例
    ├── errors/        必须被拒绝并给出诊断的程序
    └── debug/         `-g` 的 gdb 用例（见下）
```

`cases/debug/` 里的用例由 `run_gdb_e2e.sh` 驱动真实 gdb，用注解声明期望，断点用 `// BRK` 标记所在行而不写死行号：

```c
// expect: 42                  程序自身退出码
// gdb: break {brk}            gdb 命令，按顺序执行
// gdb_expect: a = 40          输出必须匹配的正则
// gdb_reject: optimized out   输出不允许出现的正则
// gdb_expect_O0: n = 1        只在某一优化级别检查
```

这些用例同时被单文件 e2e 套件当普通用例跑（它们都有 `// expect:`），并且每个都额外比对加 `-g` 与不加 `-g` 的 `.text` 是否逐字节相同。
