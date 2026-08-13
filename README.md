# FakeCC

[![CI](https://github.com/esrrhs/fakecc/actions/workflows/ci.yml/badge.svg)](https://github.com/esrrhs/fakecc/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![Language: C99](https://img.shields.io/badge/language-C99-orange.svg)
![Self-hosting](https://img.shields.io/badge/self--hosting-yes-brightgreen.svg)
![Freestanding](https://img.shields.io/badge/default-zero--dep-brightgreen.svg)

一门类 C 的系统级编程语言——保留 C 的执行模型，去掉预处理器，自带 runtime。**已自举**：fakecc 能编译自己，两级产物逐字节相同；**默认零依赖**：用户程序与自举产物不链系统 libc（像 Go；`-l` 可选互操作，像 cgo）。

## 设计目标

### 与标准 C 一致的部分

- 声明语法：`int x;`、`int (*fp)(int)`、struct、union、enum、typedef、指针、数组
- 语句：`if/else/while/for/switch/return/break/continue/goto`
- 表达式：完整运算符、优先级、隐式转换、指针算术、左值/右值
- 类型系统：整型/浮点/指针/struct/union/enum、`const/volatile/restrict`
- 函数签名：`int main(int argc, char **argv)`
- 手动内存管理（`malloc`/`free`），无 GC
- `sizeof` / `_Alignof` 等编译期运算符

### 与标准 C 不同的部分

| 特性 | C | FakeCC |
|---|---|---|
| 预处理器 | `#include` / `#define` / `#if` / … | **整个消失** |
| 宏 | 对象宏、函数宏、`#`、`##` | **全部不存在** |
| 头文件 | `.h`、前向声明 | 用 `extern` / 多文件链接；跨 package 的 `import` 仍在规划 |
| 包组织 | 无 | 文件顶部 `package main;`（当前实现） |
| 标准库 | 系统 libc | **默认内置 `rt/`**（裸 syscall）；`-nostdlib -lc` 可选走系统库 |
| 条件编译 | `#if` / `#ifdef` | 第一版不引入 |

### 最小程序

```c
package main;

int main() {
    return 42;
}
```

```c
package main;
extern int printf(const char *fmt, ...);
int main(void) {
    printf("hello %d\n", 42);
    return 0;
}
```

```bash
./build/fakecc hello.c -o hello
ldd hello          # 「不是动态可执行文件」— 无 libc
./hello            # hello 42
```

## 编译器架构

```
源码 → Lexer → Tokens → Parser → AST → Sema → IR → Opt → Codegen → ELF/链接 → 可执行文件
```

- **前端**：Lexer + Parser + Sema + IR 生成
- **中端**：统一 SSA IR（CFG、支配树、mem2reg、constfold/DCE/peephole、寄存器分配）
- **后端**：x86-64 编码器 + 内嵌 ELF 写入器 / 链接器（不调用外部 `as`/`ld`）

## 零依赖 runtime（`rt/`）

默认链接时，驱动自动编译并静态链上 `rt/`（FakeCC 方言，约 950 行）：

| 文件 | 内容 |
|---|---|
| `string.c` | `memcpy` / `memset` / `strlen` / `strcmp` / … |
| `ctype.c` | `isdigit` / `isspace` / … |
| `malloc.c` | `mmap` freelist：`malloc` / `free` / `calloc` / `realloc` |
| `stdio.c` | 迷你 `FILE`、`stdin`/`stdout`/`stderr`、缓冲、`fopen`/`fread`/`fwrite`/`puts`/`putchar`/… |
| `printf.c` | `printf` / `fprintf` / `snprintf` / `v*`（`%s%c%d%i%u%x%p` 等） |
| `stdlib.c` | `exit`（先 fflush）/ `abort` / `strto*` / `qsort` / `chmod` / `getenv` |

- 查找顺序：`FAKECC_RT` → `./rt` → `<argv0>/rt` → `<argv0>/../rt`
- `-nostdlib`：不链 `rt/`；再加 `-lc` 即走系统 libc（调试 / 互操作用）
- `-l` / `-L` / 直接传 `.so`：可选动态依赖，`-L` 写入 `DT_RUNPATH`

Stage0（`build/fakecc`，gcc 编）本身仍依赖系统 libc；**它编出来的程序**和自举产物 `v0/fakecc-1` 默认是静态零依赖 ELF。

## 自包含工具链

不依赖外部 `as` / `ld`。链接器直接写出 ELF64：

- 内部 x86-64 编码 + ELF 写入（PT_LOAD；动态时才有 INTERP / PT_DYNAMIC）
- 入口桩：设置 `argc`/`argv` → `call main` → 调用 `exit`（有 rt 或 `-lc` 时）或裸 `exit_group`
- 无未解析外部符号时：静态 ELF（`ldd` 报非动态可执行文件）

可执行文件当前**不带** `.symtab` / DWARF，gdb 只能按地址看指令，不能 `break main` 或按源码行调试。

## 自举

主体源码仍是 `src/*.c`（C99，给 gcc 做 Stage0）；`v0/translate.py` 机械翻译成 FakeCC 方言后自举：

- **Stage 0** — gcc 编译 `src/` → `build/fakecc`
- **Stage 1** — Stage0 编译 `v0/*.c`（+ 默认 `rt/`）→ `fakecc-1`
- **Stage 2** — `fakecc-1` 再编译同一份 → `fakecc-2`；与 `fakecc-1` 逐字节相同即不动点

```bash
v0/build_bootstrap.sh   # 翻译 + Stage0 编译链接
v0/stage2_check.sh      # 两级自举 + 逐字节比对
```

gcc 只出现在：编 Stage0，以及 `translate.py` 里对 `src/*.c` 做预处理（方言无预处理器）。自举编译与链接全程由 fakecc 完成。CI bootstrap job 在不动点之后用 `v0/fakecc-1` 再跑 e2e / e2e_multi / difftest / e2e_shlib。

## 语言与实现现状

| 类别 | 已支持 |
|---|---|
| 声明与类型 | `char/short/int/long/long long` × signed/unsigned、`_Bool`、`void`、`struct`/`union`/`enum`、`typedef`、qualifier、`inline`、`static`/`extern`、函数指针 |
| 字面量 | 十进制 / `0x` / 八进制 `0`、后缀、`float`/`double`（SSE）、`long double`（x87，16 字节） |
| 表达式 / 语句 | 完整运算符、指针算术、cast、`sizeof`/`_Alignof`、复合字面量；`if`/`while`/`for`/`do`/`switch`/`goto`/… |
| 函数 | 多函数、递归、最多 16 参数（SysV 前 6 寄存器）、变参（`va_list`） |
| 链接 | 多文件 / `.o`；默认静态链 `rt/`；可选 `-l`/`-L`/`.so` |
| I/O | `__syscall`；`printf`/`malloc`/FILE 来自 `rt/` |

跨 package 的 `import` / 导出可见性规则仍是语言方向，**尚未实现**（当前用多文件 + `extern`）。

## 已知缺陷

- **小结构体按值传参不符合 SysV AMD64**：一律降级为传指针；纯 fakecc 自洽，但与 gcc/libc 按值小结构体 ABI 不混用。
- **无调试信息**：链接产物无符号表 / DWARF。
- **工程主体仍是 `src/` + translate**：方言尚未成为唯一源码树；Stage0 仍需 gcc。

## 调试工具

- `tools/bisect_module.sh` — 混合链接二分（定位自举错在哪个模块）
- `tools/difftest.sh` — 以 gcc 为 oracle 的退出码 / stdout 差分

自举期修过的后端问题已落成 e2e，例如 `extern_block_scope*.c`、`regalloc_trunc_spill.c`、`trunc_dirty_high_bits.c`、`dynamic_printf_output.c`。

## 构建

```bash
cmake -S . -B build
cmake --build build --parallel
```

## 运行

```bash
./build/fakecc examples/return42.c -o /tmp/a.out
/tmp/a.out; echo $?    # 42

# 可选：不用 rt，改链系统 libc
./build/fakecc hello.c -nostdlib -lc -o /tmp/hello_libc
```

## 测试

```bash
ctest --test-dir build --output-on-failure
# 单元测试、单文件 e2e、多文件 e2e、共享库 -l、gcc difftest

bash v0/stage2_check.sh
bash test/e2e/run_e2e.sh v0/fakecc-1
bash test/e2e/run_multi_e2e.sh v0/fakecc-1
bash test/e2e/run_difftest.sh v0/fakecc-1
bash test/e2e/run_shlib_e2e.sh v0/fakecc-1
```
