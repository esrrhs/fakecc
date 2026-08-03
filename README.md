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

### Slice 11 — struct + `.` + `->`

支持 `struct Foo { ... };` 定义、`struct Foo x;` 变量、`s.x` 成员访问、`p->x` 箭头（自动解糖为 `(*p).x`）、`sizeof(struct Foo)`、struct 里嵌套数组和指针成员。

```c
struct Node { int val; struct Node *next; };
int main() {
    struct Node a, b;
    a.val = 1; a.next = &b;
    b.val = 2; b.next = 0;
    return a.val + a.next->val;
}
```

- **Lexer** 新增 `struct` 关键字、`.` (TK_DOT)、`->` (TK_ARROW)
- **AST** `TypeKind` 新增 `TY_STRUCT`；`Type` 加 `char *tag`；新加 `StructRegistry`/`StructDef`/`StructMember`；`TranslationUnit.structs` 挂在 module 上；`Expr` 新增 `EX_MEMBER`
- **Parser** 顶层识别 `struct Foo { ... };` 定义（先注册到 registry，再解析成员算 offset）；解析类型时 `struct Foo` 查 registry 拿 size；postfix 支持 `.name` 和 `->name`（arrow 桥接为 `(*obj).name`）
- **Sema** 新增 file-scope `StructRegistry*` 指针；`EX_MEMBER` 校验对象是 TY_STRUCT、成员存在，结果类型是成员的 Type；lvalue 集合加入 EX_MEMBER
- **IR-gen** 新增 `lower_lvalue_addr` 统一算 lvalue 地址（EX_VAR/DEREF/INDEX/MEMBER 递归调用自己 + 加成员 offset）；struct 变量强制 pinned，`sizeof(struct)` 从 registry 拿 size；rvalue 用 `.` 或 `->` 取 struct 成员：标量 → LOAD_PTR，array/struct → 直接返回地址（衰减）

**布局**：自然对齐——char@1, short@2, int@4, long/ptr@8；每个成员先按对齐 pad 到边界，结构末尾再 pad 到 8 字节。`type_align` 是新的内部辅助函数。

**边界**：struct 值传参/返回、初始化器列表 `{1,2,3}`、位域、匿名 struct、union 都留到未来。

新增 5 个 e2e：`struct_basic / struct_arrow / struct_arr_member / struct_ptr_param / struct_sizeof`。101 个 e2e 全绿。

### Slice 10 — >6 参数走栈

参数数量上限从 6 提升到 16。前 6 个走 SysV 寄存器（rdi/rsi/rdx/rcx/r8/r9），第 7 个及以上按 SysV 约定右到左压栈，callee 通过 `[rbp + 16 + 8*(k-6)]` 读取。

```c
int sum16(int a, ..., int p) { return a+...+p; }
int main() { return sum16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16); }  // 136
```

- **`IR_CALL_MAX_ARGS`** 6 → 16
- **Parser/Sema** 参数上限、`FunSig.param_types` 数组同步扩容
- **Codegen — call site**：`nstack = max(0, nargs-6)`；先按倒序（arg N-1 → arg 6）压栈参数，再照旧对前 6 个做 push-then-pop 舞蹈进 arg regs；对齐算式：需 `nstack % 2 != 0` 时额外 `sub rsp, 8`；调用后 `add rsp, nstack*8 + pad` 回收
- **Codegen — 被调方 prologue**：前 6 个 param 走原有 push-pop 舞蹈；第 7 个及以上 `mov home, [rbp+16+8*(k-6)]`

新增 4 个 e2e：`call_8args / call_10args / call_16args / call_stack_recurse`。

### Slice 9 — for / break / continue

```c
package main;
int main() {
    int s = 0;
    for (int i = 1; i <= 100; i = i + 1) {
        if (i % 2 == 0) { continue; }
        if (i > 20) { break; }
        s = s + i;
    }
    return s;
}
```

- **Lexer/Parser** 新增关键字 `for`/`break`/`continue`；`for (init? ; cond? ; step?) body`——三段都可以为空；`init` 可以是 decl 也可以是 expr
- **AST** 新增 `ST_FOR / ST_BREAK / ST_CONTINUE`；for 的 init 是 `Stmt*`（允许 decl），cond/step 是 `Expr*`
- **Sema** 全局 `loop_depth` 计数器，`break`/`continue` 在深度为 0 时报错；`for` 引入自己的作用域给 init 里的 decl
- **IR-gen** for 下降为 `head → cond → body → step → head` 结构；`push_loop(cont=L_step, brk=L_exit)` 供内层 break/continue 查询；while 也复用同一栈；栈是文件作用域数组，深度上限 32

新增 7 个 e2e：`for_basic / for_break_continue / for_nested_break / for_continue_odd / for_infinite / while_break / bad_break_toplevel`。

### Slice 8 — 全局变量 + 字符串字面量

从"只有栈变量"到"能持久保存状态"。文件作用域声明 `int x = 42;`；字符串字面量 `"hello"` 编译期入 rodata 全局；两者都通过 `lea r, [rip+disp32]` 定位。ELF 输出扩展为两个 PT_LOAD 段（`R+X` 的 code / `R+W` 的 data）。

```c
package main;
int counter = 0;
int bump() { counter = counter + 1; return counter; }
int main() {
    bump(); bump(); bump();
    return counter;   // 3
}
```

**前端**：
- **Lexer** 已有 `TK_STRING_LITERAL`，现在解析：strip 引号并处理 `\n \t \r \0 \\ \" \'` 转义
- **AST** 新增 `EX_STR`（bytes + len），`TranslationUnit.globals: StmtArray` 持有文件作用域 `ST_DECL`
- **Parser** 顶层用 lookahead 区分函数（`type IDENT (`）和全局（`type IDENT [ = expr ] ;`）；识别全局的 `[N]` 数组维度
- **Sema** 全局符号存到一个长生命期的 `SymTable`，函数体开始时导入到局部作用域底部；重复全局名和函数名冲突诊断；字符串字面量类型为 `char*`（衰减后）

**中/后端**：
- **IR** 新增 opcode `IR_GADDR`（`dst = &global`，name 存 `call_name`）；`IRModule.globals: IRGlobalArray` 持有 `{name, size, init_bytes, is_readonly}` 
- **IR-gen** 顶层扫 `tu->globals`，把标量初始化字面量转成 raw bytes 存进 `IRGlobal.init_bytes`；每个 EX_STR 分配 `__str.N` 匿名 rodata 全局；EX_VAR/EX_ADDR/EX_ASSIGN 在 IRSlot 打上 `is_global` 标志时走 IR_GADDR 路径
- **Codegen** 新增 `emit_lea_rip`（`REX.W 8D <mod=00 reg=dst rm=5> disp32=0`），emit 完后把 `{patch_off, target_name}` 塞入 `EmitModule.relocs`
- **ELF writer**：
  - `EmitModule` 加 `Buffer data` + `EmitGlobal[]` + `EmitGlobalReloc[]`
  - `emit_elf` 现在写两个 PT_LOAD：code 段（`PF_R|PF_X`，从 base 起）和 data 段（`PF_R|PF_W`，file 偏移和 vaddr 对齐到 `PAGE_SIZE`）
  - 最后一遍解析 relocs：`disp32 = data_vaddr + global.offset − (base + code_offset + patch_off + 4)`，直接改写 code buffer

**边界**：初始化器只允许整型字面量（`int x = 42;` / `int x = -7;`）；`char buf[16]` 之类的数组只支持零初始化；复合字面量、非常量表达式、`int a[3] = {1,2,3}` 留到未来 slice。

新增 7 个 e2e：`global_int / global_mutate / global_array / global_cross_fn / string_chars / string_len / string_addr`。85 个 e2e 全部通过。

### Slice 7b+7c — 指针、数组、cast、sizeof

从"只有整型"进入到"可以操纵内存"。支持任意多级指针（`int**`）、取址 `&`、解引用 `*`、任意固定长度数组含多维（`int a[M][N]`）、下标 `a[i]`、指针算术（按元素大小自动缩放）、数组到指针衰减、显式 cast `(T)expr`、`sizeof(T)` 与 `sizeof(expr)`。

```c
package main;
int swap(int *a, int *b) {
    int t = *a; *a = *b; *b = t;
    return 0;
}
int main() {
    int a[3][3];
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) { a[i][j] = i * 3 + j; j = j + 1; }
        i = i + 1;
    }
    return a[2][2] + a[1][0];   // 8 + 3 = 11
}
```

**类型系统**：`Type` 变成递归结构——`TY_PTR` 携带堆分配的 `Type *pointee`，`TY_ARRAY` 携带 `Type *elem_type` 与 `int length`。所有拷贝、free、size 计算都走 `type_clone / type_free / type_size` 三个 API，避免共享堆指针导致 double-free。

**前端**：
- **Lexer** 新增 `& [ ]` 与 `sizeof` 关键字
- **Parser** `parse_type` 后缀 `*` 折叠成 `TY_PTR`；声明子后缀 `[N]` 折叠成 `TY_ARRAY`（右到左，支持多维）；`int f(int a[])` 直接解糖为 `int *`。`parse_unary` 新增 `&`/`*` 前缀与 `sizeof`；`parse_primary` 用 lookahead 区分 `(T)expr` 与 `(expr)`；数组下标 `[i]` 走 postfix loop
- **Sema** 新增 `EX_ADDR/DEREF/INDEX/CAST/SIZEOF_TYPE/SIZEOF_EXPR` 类型检查；lvalue 集合扩充到 `EX_VAR | EX_DEREF | EX_INDEX`；binop 里 `p + i / p - i` 自动识别指针操作数

**中/后端**：
- **IR** 新增三条 opcode：`IR_ADDR`（`dst = &alloca_slot`，走 LEA）、`IR_LOAD_PTR`（`dst = *ptr`）、`IR_STORE_PTR`（`*ptr = val`）。老的 `IR_LOAD/STORE`（拿 alloca id 的）保留给可 promote 的标量；这样 mem2reg 对老路径无侵入
- **IR-gen** 做一遍"取址分析"：扫描函数体，如果某局部变量出现 `&name`，就把它标记为 **pinned**——它的 `IR_ALLOCA` 带 `alloca_bytes > 0`，后续读写走 `IR_ADDR + IR_LOAD_PTR/IR_STORE_PTR`。数组类型永远 pinned。指针算术按 `sizeof(pointee)` 缩放：`p + i` 下降为 `IR_ADD(p, IR_MUL(i8, esize))`
- **mem2reg** 加两条 pin 判据：`alloca_bytes > 0` 或 `dst 出现在 IR_ADDR 的操作数里`。被 pin 的 alloca 不再被 promote，其 `IR_ALLOCA` 指令保留下来供 codegen 读取 `alloca_bytes` 布局栈帧
- **scalar_opt** 把 `IR_STORE_PTR` 加入 side-effect 列表；`IR_ADDR / IR_LOAD_PTR / IR_TRUNC` 走单操作数分支自然处理
- **Codegen** 新增 `emit_lea_rbp`、`emit_load_via_ptr`（宽度感知 movsx/movzx/mov/movsxd）、`emit_store_via_ptr`（`mov byte/word/dword/qword ptr [reg], src`）；prologue 里新增 pinned-alloca 布局阶段：扫描 `IR_ALLOCA` 收集 `alloca_bytes`，按 8 字节对齐后追加到 spill area 之后；`IR_ADDR` 编码 `lea dst, [rbp+off]`

**同期修复**：mem2reg 之前对 *所有* `IR_ALLOCA` 一刀切标记为 dead——加了 pin 判据之后，pinned 的必须保留下来（否则 codegen 拿不到 `alloca_bytes`）。

新增 15 个 e2e：`ptr_basic / ptr_write_thru / ptr_swap / ptr_multilevel`；`arr_basic / arr_loop_sum / arr_char / arr_2d / arr_decay / param_array`；`cast_narrow / cast_ptr`；`sizeof_int / sizeof_ptr / sizeof_arr`；`bad_addr_rvalue / bad_deref_int`。

### Slice 7a — 整型类型系统 + 隐式转换

从"一切都是 int"扩展到完整的整型类型系统：`char / short / int / long` × `signed / unsigned`，以及 C 标准的 usual arithmetic conversions（§6.3.1.8）。

```c
package main;
int main() {
    unsigned int x = 4000000000;
    unsigned int y = x + 1000000000;
    if (y == 705032704) { return 1; }   // 无符号回绕
    return 0;
}
```

**设计**："i64 抽象机 + 显式宽度标注"。SSA 值在 64 位寄存器里流动；`Type = {kind, width∈{1,2,4,8}, is_unsigned}` 挂在 `Expr / Param / Stmt.decl / FunctionDecl.ret_type` 上；IR-gen 按 UAC 规则插入 `IR_SEXT / IR_ZEXT / IR_TRUNC` 归一化操作数；codegen 用宽度感知发射：算术后 `movzx` 掩到 `width` 字节，比较按 `is_unsigned` 选 `setb/seta/…` 或 `setl/setg/…`，除法按 signed 走 `cqto/idiv`、unsigned 走 `xor rdx / div`。

**前端**：
- **Lexer** 新增 `char / short / long / signed / unsigned` 关键字
- **AST** 新增 `Type` 结构；`FunctionDecl.ret_type`、`Param.type`、`Stmt.decl.type`、`Expr.type`（后者由 sema 填充）
- **Parser** 抽出 `parse_type()`——`[signed|unsigned] (char|short|int|long)`；替换掉原先三处硬编码 `expect_kind(TK_KW_INT)`
- **Sema** 每个 Expr 都标注类型；二元运算按 UAC 求结果类型；比较结果永远是 `int`；赋值/返回/传参处允许隐式收窄，语义与 C 一致

**中/后端**：
- **IR** `IRInst` 增加 `width` / `is_unsigned` 字段；新 opcode `IR_SEXT / IR_ZEXT / IR_TRUNC`（imm = 源宽度）；`IRFunction` 加 `value_width[]` / `ret_width` 表让 IR-gen 决定何时插入转换
- **Codegen** 引入 `emit_movsx_rr` / `emit_movzx_rr` / `mask_to_width`；算术、除法、比较都按 `inst->width` / `inst->is_unsigned` 分派

**同期修复**：`cfg_rpo` 里潜藏的越界读——`postorder[]` 只有 `po_len` 个有效项但循环 `n` 次；老代码靠"未初始化内存恰好是零"侥幸通过，7a 加类型系统扰动了栈内存后就崩了。改为只迭代 `po_len` 次并把不可达块标为 `INT_MAX` 排在最后。

新增 12 个 e2e：`type_char_basic` / `type_char_signed_ext` / `type_uchar_zero_ext` / `type_char_promotion` / `type_short_arith` / `type_uint_wrap`（无符号回绕验证）/ `type_return_narrow` / `type_long_arith` / `type_signed_unsigned_cmp`（C 混合比较语义）/ `type_param_narrow` / `type_signed_kw` / `bad_type_unknown`。

### Slice 6 — 函数调用与 System V AMD64 ABI

从"单函数玩具"跃迁到"能写真实程序"。支持多函数定义、任意函数调用、递归、相互递归、最多 6 个 `int` 参数。

```c
package main;
int fib(int n) {
    if (n < 2) { return n; }
    return fib(n - 1) + fib(n - 2);
}
int main() { return fib(10); }   // 55
```

前端：
- **Parser** 参数列表 `int a, int b, ...`（最多 6 个）；`IDENT "(" args ")"` 后缀调用；一个 TU 里多个函数
- **Sema** 建立模块级函数表，检查调用点 callee 存在且 arity 一致；相互递归自动支持（不需要前向声明）
- **AST** 新增 `EX_CALL`（callee 名 + `ExprArray`），`FunctionDecl` 加 `ParamArray`

中/后端：
- **IR** 新增 `IR_CALL`（callee 名 + up-to-6 SSA 参数值）和 `IR_PARAM`（imm = 参数位）；`IRInst` 增加 `call_name`/`call_args`/`call_nargs` 字段
- **Regalloc** 把 `call_args` 纳入 use 集合；在每个 CALL 处，凡是活到调用之后的值都禁止分配到 caller-saved 寄存器（RSI/RDI/R8/R9/R10/R11）——用一个 `forbid_mask[v]` 位图强制
- **Codegen**
  - Prologue：push 三个 callee-saved（rbx/r12/r13，按需）；对参数做 push-then-pop 舞蹈——先按倒序 push 所有传入 arg reg，再按正序 pop 到分配的家寄存器，彻底避免 arg-reg 之间的写-读依赖
  - `IR_CALL`：同样的 push-pop 舞蹈把参数放入 rdi/rsi/rdx/rcx/r8/r9；`call rel32` 带 patch；调用后从 rax 取返回值
  - 跨函数调用：整个 module 编译完后统一 patch，用 `EmitModule` 的符号表解析
  - Epilogue：反序 pop callee-saved 后 `ret`

栈对齐：每个函数入口处已对齐到 16 字节，push/pop 顺序确保 `call` 指令时 rsp 满足 SysV 要求。

**同期修复**：`ir_module_free` 会走每条指令 free `call_name`；mem2reg 写回的 COPY 指令必须显式初始化 `call_name = NULL`（栈变量的野指针会段错误）。

### Slice 5 — CFG-aware regalloc（循环也能寄存器分配）

删掉 Slice 4 中"含控制流走栈式回退"的技术债。现有的基于区间的 SSA chordal 分配器只对直线代码正确——在循环里，一个跨回边的值的活跃区间会环绕，简单的 `[first_def, last_use]` 完全 undercount。改造为标准的迭代式数据流分析：

- **use[b] / def[b]** 每块 upwards-exposed 使用与写入集合，用位图存储
- **live-in / live-out 定点迭代**：`in[b] = use[b] ∪ (out[b] \ def[b])`，`out[b] = ⋃ in[s]`，反向按块序迭代到收敛。循环回边就是普通后继边，自然把值从 body 的 out 带回 head 的 in
- **干扰图**：对每块反向扫描，起始 live = out[b]；每条指令，dst 与当前 live 集合中所有值互相干扰，然后 dst 出 live、use 入 live
- 沿用现有 MCS + 贪心染色

同步修了几个此前被 undercount 掩盖的机器码 bug：
- `emit_mov_rr/add/sub/cmp/xor/test` 等一直只发 REX.W（0x48），从未编码 REX.R/REX.B，导致分配到 R8..R15 的值被指令按低 8 寄存器编码 → 静默错乱。改造出统一的 `emit_rex_wrb(w, r_reg, rm_reg)`
- x86 两操作数指令 `dst = a op b` 中 `a` 与 `dst` 共用位置，如果 `reg[b] == dr` 则 `ensure_reg(a, dr)` 会先破坏 b。改为固定用 RAX/RCX 作暂存，最后 `mov dr, RAX`；配合把 RAX/RCX/RDX 从分配池里移出（专职暂存/ABI）

现在 `while` / 嵌套循环 / 阶乘 全部走寄存器分配路径。

### Slice 4 — 控制流与比较运算符

引入完整的 C 控制流骨架：`if/else`、`while`、块语句 `{...}`，以及六个比较运算符 `< <= > >= == !=`（按 C 优先级：`equality < relational < additive`）。

```c
package main;
int main() {
    int i = 1;
    int sum = 0;
    while (i <= 10) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;   // 55
}
```

实现要点：
- **Lexer** 双字符前瞻识别 `== != <= >=`；新增关键字 `if` / `else` / `while`
- **AST** 新增 `ST_IF` / `ST_WHILE` / `ST_BLOCK` 与 `BOP_EQ..BOP_GE`
- **Sema** 支持嵌套作用域（`ST_BLOCK` 引入新的 scope-mark），块内变量互不干扰
- **IR** 引入 `IR_EQ/NE/LT/LE/GT/GE`；`if` 下降为 `CBR → label`，`while` 下降为 `head/body/exit` 三块结构 + 回边
- **Codegen** 直接编码 `cmp/setcc/xor/test/jmp/jcc` 机器码，函数末尾用 rel32 patch 表回填标签偏移

（Slice 4 结束时，含循环的函数临时走栈式 codegen；Slice 5 把 regalloc 改造为 CFG 感知，回退路径已删除。）

### Slice 3 — 局部变量与赋值

函数体从单条 `return` 语句扩展为**语句序列**，引入局部变量声明、赋值和变量引用。支持：

- 变量声明 `int x;` 与带初始化的声明 `int x = expr;`
- 赋值语句 `x = expr;`，赋值是表达式（值为所赋的值），且**右结合**，支持链式赋值 `x = y = 7;`
- 变量引用：变量可出现在任意表达式位置
- 语句序列：`decl-stmt` / `return-stmt` / `expr-stmt` 任意组合

```c
package main;
int main() {
    int a;
    int b;
    a = 3;
    b = 4;
    return a * a + b * b;   // 25
}
```

变量一律走 `alloca`/`load`/`store` 的显式内存形式：声明时 `IR_ALLOCA` 预留栈槽，引用时 `IR_LOAD` 读出，赋值时 `IR_STORE` 写入。引入扁平函数级符号表做语义检查（未声明使用、重复声明、对非左值赋值、缺少 `return`）。

### Slice 2 — 整型算术表达式

`return` 后面现在可以接一个真正的整型算术表达式。支持：

- 二元运算符 `+ - * / %`（带优先级，`* / %` 高于 `+ -`，左结合）
- 一元运算符 `+ -`（如 `-5`、`--5`）
- 括号 `( )` 改变优先级

```c
package main;
int main() { return 100 - 2 * (3 + 4); }   // 86
```

实现上引入了 SSA 风格 IR（虚拟寄存器 + 三地址码）和栈式求值的 codegen：每个表达式求值结果存放在栈槽位 `[rbp - 8*(v+1)]`，`return` 时把对应槽位 load 到 `%rax` 返回。

### Slice 1 — 最小编译器（已完成）

打通「源码 → Lexer → Parser → Sema → IR → Codegen → ELF」全链路，仅支持：

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
