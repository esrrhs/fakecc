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
| 头文件 | `.h`、前向声明 | 用 `extern` / 多文件链接；跨 package 用 `import` |
| 包组织 | 无 | 文件顶部 `package name;`；`import pkg;` 后写 `pkg.sym` / `pkg.Type` |
| 标准库 | 系统 libc | **默认内置 `rt/` 分包**（裸 syscall）；`-nostdlib -lc` 可选走系统库 |
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
import fmt;
int main(void) {
    fmt.printf("hello %d\n", 42);
    return 0;
}
```

```c
package main;
extern int printf(const char *fmt, ...);  /* 仍可用：与 gcc .o / -lc 互操作 */
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

默认链接时，驱动自动编译并静态链上 `rt/`（FakeCC 方言）。每个子目录是一个 package，目录名即包名；包内文件互相可见，跨包用限定名：

| 包 | 文件 | 内容 |
|---|---|---|
| `types` | `types/types.c` | 共享 `size_t` / `ssize_t`（其它 rt 包 `import types` 再 typedef） |
| `str` | `str/string.c` | `memcpy` / `memset` / `strlen` / `strcmp` / … |
| `ctype` | `ctype/ctype.c` | `isdigit` / `isspace` / … |
| `mem` | `mem/malloc.c` | `mmap` freelist：`malloc` / `free` / `calloc` / `realloc` |
| `io` | `io/stdio.c` | 迷你 `FILE`、`stdin`/`stdout`/`stderr`、缓冲、`fopen`/`fread`/… |
| `fmt` | `fmt/printf.c` | `printf` / `fprintf` / `snprintf` / `v*` |
| `std` | `std/stdlib.c` | `exit` / `abort` / `strto*` / `qsort` / `getenv` |

用户侧：`import fmt;` 之后写 `fmt.printf(...)`。ELF 符号不改名（`fmt.printf` 仍链接到 `printf`），因此原有的 `extern int printf(...);` 写法继续可用。

- 查找顺序：`FAKECC_RT` → `./rt` → `<argv0>/rt` → `<argv0>/../rt`
- `-nostdlib`：不链 `rt/`；再加 `-lc` 即走系统 libc（调试 / 互操作用）
- `-l` / `-L` / 直接传 `.so`：可选动态依赖，`-L` 写入 `DT_RUNPATH`

Stage0（`build/fakecc`，gcc 编）本身仍依赖系统 libc；**它编出来的程序**和自举产物 `v0/fakecc-1` 默认是静态零依赖 ELF。

## 自包含工具链

不依赖外部 `as` / `ld`。链接器直接写出 ELF64：

- 内部 x86-64 编码 + ELF 写入（PT_LOAD；动态时才有 INTERP / PT_DYNAMIC）
- 入口桩：设置 `argc`/`argv` → `call main` → 调用 `exit`（有 rt 或 `-lc` 时）或裸 `exit_group`
- 无未解析外部符号时：静态 ELF（`ldd` 报非动态可执行文件）

可执行文件默认带 `.symtab` / `.strtab`（对齐未 strip 的 gcc），gdb 可 `break main`、按函数名反汇编。加 `-g` 时额外发出 DWARF（行号、参数/局部变量、`.debug_frame`、`.debug_loc`），支持按源码行调试与 `print`。

`-g` 与 `-O` 正交，和 gcc 一致：`-g` 只增加调试节，绝不改变生成的指令（`test_debug` 与 gdb e2e 都逐字节比对 `.text` 来守住这条）。优化后变量常驻寄存器且位置随程序点变化，因此 mem2reg 会留下 `IR_DBG_VALUE` 标记，codegen 据此结合寄存器分配结果生成 DWARF 位置列表。该标记对 DCE、活跃区间和 SSA 重编号一律不可见，这正是它不影响代码的原因。

`.debug_frame` 逐步描述 prologue（entry / `push %rbp` 之后 / `mov %rsp,%rbp` 之后各一条规则），行表则用 `DW_LNS_set_prologue_end` 标出函数体的第一行。两者都是 `break <函数名>` 能用的前提：gdb 会自己跳过 prologue 停在中间，只给整段 prologue 一条 CFI 规则的话它会展开出一个并不存在的栈帧。

优化级别：`-O0` 让标量留在栈上（跳过 SSA 提升，调试信息最直白），`-O1`（默认）跑完整流水线。

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

跨 package 的 `import` / 限定名已实现（目录即包；同包文件互见；`static` 包私有）。

## 已知缺陷

- **DWARF 位置列表在块重排时可能偏保守**：`-g` 已提供行号、标量/指针/具名 struct（含成员 DIE，`print p.x` / `q->x` 可用）、位置列表，以及 `DW_OP_entry_value` + `DW_TAG_call_site`（优化后外层帧参数可恢复）。位置列表按线性指令流推导，块的入口值靠 φ 标记补齐，块被重排时个别范围可能偏保守。
- **工程主体仍是 `src/` + translate**：方言尚未成为唯一源码树；Stage0 仍需 gcc。
- **`rt/printf` 不认 `%Lf` 的 `L`**：长度修饰符被吃掉但实参仍按 `double` 取，long double 变参会打成 0；`va_arg(ap, long double)` 同样未走 SysV 的 MEMORY 类（16 字节栈槽），暂不支持。
- **`rt/` 的浮点打印只有 18 位有效数字**：`%f`/`%e`/`%g` 从 long double 展开十进制，足以覆盖 double 的 17 位；超出部分（如 `%f` 打印 `1e300`）按展开末尾补零，而不是 glibc 的精确二进制值。

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
