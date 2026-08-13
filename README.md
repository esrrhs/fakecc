# FakeCC

[![CI](https://github.com/esrrhs/fakecc/actions/workflows/ci.yml/badge.svg)](https://github.com/esrrhs/fakecc/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![Language: C99](https://img.shields.io/badge/language-C99-orange.svg)
![Self-hosting](https://img.shields.io/badge/self--hosting-yes-brightgreen.svg)

一门类 C 的系统级编程语言——保留 C 的执行模型和性能特性，去掉预处理器，用现代包管理替代头文件。**已自举**：fakecc 能编译自己，两级产物逐字节相同。

## 设计目标

**FakeCC** 保留与 C 完全一致的执行模型和性能特性，同时移除预处理器并采用现代包管理系统：

### 与标准 C 完全一致的部分

- 声明语法：`int x;`、`int (*fp)(int)`、struct、union、enum、typedef、指针、数组
- 语句：`if/else/while/for/switch/return/break/continue/goto`
- 表达式：完整的运算符集合、优先级、隐式转换、指针算术、左值/右值规则
- 类型系统：完整整型/浮点/指针/struct/union/enum/qualifier（const/volatile/restrict）
- 函数签名：`int main(int argc, char **argv)`
- 手动内存管理（malloc/free），无 GC
- ABI 与 C 完全兼容，能调 libc
- `sizeof` / `_Alignof` 等编译期运算符保留

### 与标准 C 不同的部分

| 特性 | C | FakeCC |
|---|---|---|
| 预处理器 | `#include` / `#define` / `#if` / `#ifdef` / `#pragma` / 行拼接 `\` / 三字符组 | **整个消失** |
| 宏 | 对象宏、函数宏、`#`、`##`、`__VA_ARGS__` | **全部不存在** |
| 头文件 | `.h` 文件、前向声明 | **消除**——编译器自动跨文件解析符号，不再有前向声明的必要 |
| 包组织 | 无 | 每个文件顶部 `package foo;` 声明所属包 |
| 引入其他包 | `#include` | `import "foo/bar";` |
| 可见性 | 所有符号可见 | 首字母大写 = 跨 package 可见（导出）；小写 = 包内私有 |
| 常量 | `#define MAX 100` | `const int MAX = 100;` |
| 条件编译 | `#if` / `#ifdef` / `#ifndef` | 第一版不引入；跨平台差异用运行时或 build tag 处理 |

### 一个最小 FakeCC 程序

```c
package main;

int main() {
    return 42;
}
```

### 一个跨 package 的例子（仅示意语言方向，尚未实现）

```c
// file: foo/util.c
package util;

int Add(int a, int b) {   // 首字母大写 → 导出
    return a + b;
}

int helper(int x) {       // 首字母小写 → 包内私有
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

## 编译器架构

```
源码 → Lexer → Tokens → Parser → AST → Sema → IR 生成 → IR → Codegen → 机器码 → ELF 写入 → 可执行文件
                  前端                                  中端           后端
```

- **前端**：Lexer + Parser + Sema + IR 生成——语言相关，平台无关
- **中端**：统一 SSA IR（LLVM/QBE 风格）
- **后端**：Codegen（x86-64 机器码）+ 内嵌 ELF 写入器——平台相关，零外部依赖

编译器全程使用**一套统一 SSA IR**，不像 GCC 那样使用双层 IR（GIMPLE+RTL）。优化 pass 将以 `void opt_xxx(IRModule *)` 的签名加入，就地修改 IR。

## 自包含工具链

FakeCC 内嵌汇编器和链接器，不依赖外部 `gcc`、`as` 或 `ld`。编译器直接写出最小静态 ELF64 可执行文件：

- 内部 x86-64 机器码编码（不经过汇编文本中间步骤）
- 内部 ELF64 写入器：ELF 头 + PT_LOAD 段 + `_start` 桩代码
- `_start` 桩代码：`call main` → `mov %eax,%edi` → `mov $60,%eax` → `syscall`
- 生成的二进制是合法的静态 ELF 可执行文件

## 自举路线图

FakeCC 的最终目标是**自己编译自己**。分三阶段：

- **Stage 0** — C99 实现：用系统 gcc 编译 `fakecc`，严格遵守编码约束（无代码生成宏、无变参宏、无条件编译，除 include guard 外）
- **Stage 1** — 机械翻译到 FakeCC：`v0/translate.py` 把 `src/*.c` 翻译成 FakeCC 源码，用 Stage 0 编译 → `fakecc-1`
- **Stage 2** — 自我编译验证：用 `fakecc-1` 编译同一份源码 → `fakecc-2`；若 `fakecc-2` 通过全部测试且与 `fakecc-1` 逐字节相同，自举成功

**三个阶段均已达成，且全程不借助外部工具链。** 两级自举的编译与链接都由 fakecc 自己完成：`fakecc-1` 通过全部 243 个 e2e 测试，与 Stage 0 结果一致；`fakecc-1` 与 `fakecc-2` 的 16 个目标文件**以及最终二进制**逐字节相同，编译器复现了自己。

```bash
v0/build_bootstrap.sh   # 翻译 + 用 Stage 0 编译并链接 → v0/bootstrap_fakecc
v0/stage2_check.sh      # 跑完整两级自举，逐字节比对目标文件与二进制
```

gcc 只在翻译阶段出现，充当 `src/*.c` 的预处理器——FakeCC 方言没有预处理器，这是一次性机械翻译的固有成本，不在构建路径上。两个脚本都接入了 CI。

## 语言特性

| 类别 | 已支持 |
|---|---|
| 声明与类型 | `char/short/int/long/long long` × `signed/unsigned`、`_Bool`、`void`、`struct`/`union`/`enum`、`typedef`、`const/volatile/restrict`、`inline`、`static/extern`、函数指针 |
| 整型字面量 | 十进制 / 十六进制 `0x` / 八进制 `0`、后缀 `u/U/l/L/ll/LL`、`long long` |
| 浮点 | `float` / `double`（SSE）、`long double`（x87 FPU，16 字节） |
| 表达式 | 完整运算符（算术、位运算 `&\|^~<<>>`、逻辑 `&&||!`、比较、赋值、复合赋值、`++/--`、三元 `?:`、逗号）、指针算术、显式 cast、`sizeof`/`_Alignof`、复合字面量 |
| 语句 | `if/else`、`while`/`do-while`/`for`、`break/continue`、`goto`、`switch/case/default`、块语句、声明混用（C99 风格声明可在语句后） |
| 初始化器 | 标量/数组/struct 初始化列表、指定初始化器 `.field=` / `[i]=` |
| 函数 | 多函数、递归/相互递归、最多 16 参数（前 6 走寄存器，≥7 走栈，SysV AMD64 ABI）、变参函数（`va_list`） |
| 全局 | 全局变量（`data`/`bss`/`rodata` 自动分段）、字符串字面量（rodata）、`static` 跨文件不冲突 |
| 后端 | SSA IR + CFG + 支配树 + mem2reg + constfold/DCE/peephole + 寄存器分配；内部 x86-64 编码器 + ELF64 写入器 |
| 链接 | 多文件编译 + 链接（`fakecc a.c b.c -o prog`）；`fakecc -c a.c -o a.o` 出 `.o`；`fakecc a.o b.o -o prog` 链接对象文件；`static`→LOCAL 绑定、GLOBAL 符号跨文件解析；libc 调用走 PLT/GOT |
| I/O | `__syscall(num, a0..a5)` intrinsic 直接发 Linux syscall；动态链接调 libc（`printf`/`malloc` 等） |

## 已知缺陷

**小结构体按值传参不符合 SysV AMD64。** fakecc 无论大小一律把结构体按值传参降级为传指针；SysV 规定 ≤16 字节的结构体拆成 eightbyte 装进寄存器（>16 字节走内存的路径是对的）。自举不受影响——纯 fakecc 构建两侧约定一致——但这破坏了「ABI 与 C 完全兼容」：调用任何按值收发小结构体的 libc 函数都会传错，fakecc 目标文件也无法与 gcc 目标文件混合链接。

## 调试工具

自举期的 bug 有个共同特点：症状出现在一个 1.2 MB 的二进制里，离病因十万八千里。`tools/` 下的两个脚本把这段距离缩短到可操作的范围：

- `tools/bisect_module.sh` — 混合链接二分。除一个模块外全部用 gcc 编译，那一个用 fakecc，看混合出的编译器是否还正常。把「自举编译器不对」变成「模块 M 被编译错了」。
- `tools/difftest.sh` — 以 gcc 为 oracle 的差分测试。同一份源码两边编译、运行、比对退出码与 stdout，不需要手算期望值（手算错一个期望值和真 bug 长得一模一样，排查代价却高得多）。

自举暴露出的后端 bug 都已收敛成 e2e 回归用例：

| 用例 | 缺陷 |
|---|---|
| `extern_block_scope*.c` | 块作用域 `extern` 被当成新的零初始化局部变量，而不是绑定到文件作用域符号 |
| `regalloc_trunc_spill.c` | `IR_TRUNC` 结果溢出到栈时，codegen 按寄存器分配**前**的槽位寻址，读到帧外内存 |
| `trunc_dirty_high_bits.c` | `IR_TRUNC` 不做掩码，而比较指令发的是 64 位 `cmp`，被丢弃的高半部分参与了比较 |
| `dynamic_printf_output.c` | 入口桩用裸 `exit_group` 退出，跳过 libc 的 atexit，stdout 缓冲区从不刷新 |

## 构建

```bash
cmake -S . -B build
cmake --build build --parallel
```

## 运行

```bash
./build/fakecc examples/return42.c -o /tmp/a.out
/tmp/a.out
echo $?    # 42
```

## 测试

```bash
ctest --test-dir build --output-on-failure
# 含单元测试、单文件 e2e、多文件链接 e2e

# 自举不动点（需先 build Stage 0）
bash v0/stage2_check.sh
```
