# FakeCC

[![CI](https://github.com/esrrhs/fakecc/actions/workflows/ci.yml/badge.svg)](https://github.com/esrrhs/fakecc/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![Language: C99](https://img.shields.io/badge/language-C99-orange.svg)

一门类 C 的系统级编程语言——保留 C 的执行模型和性能特性，去掉预处理器，用现代包管理替代头文件。

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

### 一个跨 package 的例子（**不在 Slice 1 范围内**，仅示意语言方向）

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
- **中端**：统一 SSA IR（LLVM/QBE 风格）——随切片逐步扩展
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

- **Stage 0** — C99 实现（当前阶段）：用系统 gcc 编译 `fakecc-c`，严格遵守编码约束（无代码生成宏、无变参宏、无条件编译，除 include guard 外）
- **Stage 1** — 机械翻译到 FakeCC：写一个一次性翻译脚本将 `src/*.c` 翻译成 FakeCC 源码，用 Stage 0 编译 → 得到 `fakecc-1` 二进制
- **Stage 2** — 自我编译验证：用 `fakecc-1` 编译 FakeCC 源码 → 得到 `fakecc-2`；若 `fakecc-2` 通过全部测试（或与 `fakecc-1` 逐字节相同），自举成功

## 当前进度

Slice 1 — 最小编译器，仅支持：

```c
package main;
int main() { return N; }   // N 是 0-255 的整数字面量
```

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
```
