# FakeCC · 第一版骨架实现计划（Slice 1）

## 0. 项目定位与语言设计目标

**FakeCC** 是一门新的系统级编程语言，目标是**保留 C 的执行模型和性能特性，去掉预处理器，用现代包管理替代头文件**。

### 语言核心原则

**与标准 C 完全一致的部分**（语法/语义直接照抄 C11）：
- 声明语法：`int x;`、`int (*fp)(int)`、struct、union、enum、typedef、指针、数组
- 语句：`if/else/while/for/switch/return/break/continue/goto`，语句末尾有分号
- 表达式：完整的运算符集合、优先级、隐式转换、指针算术、左值/右值规则
- 类型系统：完整整型/浮点/指针/struct/union/enum/qualifier（const/volatile/restrict）
- 函数签名：`int main(int argc, char **argv)`
- 手动内存管理（malloc/free），无 GC
- ABI 与 C 完全兼容，能调 libc
- `sizeof` / `_Alignof` 等编译期运算符保留

**与标准 C 不同的部分**（这门语言的关键差异）：
- **预处理器整个消失**：无 `#include` / `#define` / `#if` / `#ifdef` / `#pragma` / `#error` / 行拼接 `\` / trigraph
- **无宏**：对象宏、函数宏、`#`、`##`、`__VA_ARGS__` 全部不存在
- **无头文件**：`.c` 文件是唯一源码单位，符号声明由编译器自动跨文件解析；不再有前向声明的必要（同一 package 内符号可互相引用，无先后顺序限制）
- **package 组织**：每个文件顶部 `package foo;` 声明所属包
- **import 引入其他包**：`import "foo/bar";` 替代 `#include`
- **大小写可见性**：符号首字母大写 = 跨 package 可见（导出）；小写 = 包内私有
- **常量替代宏**：`#define MAX 100` 一律用 `const int MAX = 100;`
- **条件编译不做**：第一版不引入任何形式的条件编译；跨平台差异用运行时或 build tag 处理

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

### 关键决策一览

| 决策项 | 选择 |
|---|---|
| 文件后缀 | `.c`（保持不变） |
| 可见性规则 | 首字母大小写（大写导出，小写私有） |
| package 声明 | `package foo;`（C 风格分号） |
| import 语法 | `import "foo/bar";`（Go 风格字符串路径） |
| 宏替代 | `const int MAX = 100;` |
| 条件编译 | 不做 |
| `sizeof`/`_Alignof` | 保留 |
| 语言名 | **FakeCC** |
| 编译器程序名 | `fakecc` |

---

## 1. 目标（Slice 1）

实现一个能编译以下 FakeCC 程序的最小编译器：

```c
package main;

int main() {
    return N;   // N 是 0-255 的整数字面量
}
```

**验收标准**：
```bash
./fakecc examples/return42.c -o /tmp/a.out
/tmp/a.out
echo $?    # 输出 42
```

这是"薄切片"路线的第一刀：从源码到可执行文件全链路打通，每层只做最少的事。**不要扩大范围**——任何后续特性都留给后续切片。

Slice 1 只需要解析并接受 `package main;` 声明，还**不涉及**多文件 / 跨 package / import 的实现——那些留到后续切片。但顶部的 `package` 声明是**必需**的语法元素，从第一天就纳入。

---

## 2. 明确的非目标（Non-Goals for Slice 1）

以下**全部不做**，即使很简单也不做：

**语言层面**（属于 FakeCC，但 Slice 1 不实现）：
- 多文件编译 / 多 package 支持
- `import` 声明的实际解析与符号导入（Slice 1 只允许 `package main;`，不允许出现 `import`）
- 跨文件符号解析
- 首字母大小写可见性检查
- struct / union / enum / typedef
- 除 `int` 以外的类型（`char`/`void`/浮点/指针/数组）
- 除整数字面量以外的表达式（`+`、`-`、变量、函数调用）
- 除 `return` 以外的语句（`if`/`while`/赋值/声明）
- 除 `main` 以外的函数
- 函数参数
- 负数字面量、十六进制、后缀、浮点
- `const` 常量
- `sizeof`

**工程层面**：
- 错误恢复（遇到语法错误直接报错退出即可）
- 优化 pass
- Windows / macOS 支持
- LLVM 后端（本切片直接生成 x86-64 汇编文本）

**严格遵守非目标**是本切片能在 1 周内完成的关键。

---

## 3. 技术选型

| 项 | 选择 |
|---|---|
| **实现语言** | **C99**（为将来自举做准备） |
| 允许依赖 | libc（`stdio.h` / `stdlib.h` / `string.h` / `stdarg.h` / `assert.h` / `ctype.h`） |
| 构建系统 | CMake 3.20+（C 项目） |
| 单元测试框架 | **手写极简 assert 框架**（20-50 行，同项目内） |
| e2e 测试 | Bash 脚本 |
| 目标平台 | x86-64 Linux（System V AMD64 ABI） |
| 汇编器/链接器 | 起步阶段调用 `gcc` 完成汇编+链接；后续切片再换 `as`+`ld` 直接调用 |
| Host C 编译器 | GCC 或 Clang（任意近代版本，支持 C99 即可） |

**Stage 0 编码约束**（为 Stage 1 自举翻译降低成本，详见 §11）：
- `#include` 只用来引 libc 和项目内部头文件
- `#define` 只能定义**无参**的常量（如 `#define MAX_TOKENS 65536`）或简短的**无害内联宏**（如 `#define ARRAY_LEN(a) (sizeof(a)/sizeof((a)[0]))`）
- **禁止**用宏做代码生成（`#define X_LIST(V) V(a) V(b) V(c)` 这种模式）
- **禁止**条件编译（`#if` / `#ifdef` / `#ifndef` 除了头文件 include guard 外一律不用）
- **禁止**变参宏（`__VA_ARGS__`）
- **允许**使用 `sizeof`、`_Alignof`、复合字面量、指定初始化（C99 特性）

后续切片会自建 SSA IR、自建 codegen、加 LLVM 兜底后端。本切片**不要**引入 IR，直接从 AST 到汇编，保持代码量最小。

---

## 4. 项目目录结构

```
fakecc/
├── CMakeLists.txt
├── README.md
├── include/
│   └── fakecc/
│       ├── common.h         # 通用工具（动态数组、字符串、错误报告）
│       ├── token.h
│       ├── lexer.h
│       ├── ast.h
│       ├── parser.h
│       ├── sema.h
│       └── codegen.h
├── src/
│   ├── main.c               # 驱动：解析命令行、串起各阶段
│   ├── common.c             # 动态数组、字符串、错误报告实现
│   ├── lexer.c
│   ├── parser.c
│   ├── sema.c
│   └── codegen.c
├── test/
│   ├── CMakeLists.txt
│   ├── test_framework.h     # 极简 assert 框架（宏 + 计数器）
│   ├── test_framework.c
│   ├── unit/
│   │   ├── test_lexer.c
│   │   ├── test_parser.c
│   │   └── test_codegen.c
│   └── e2e/
│       ├── run_e2e.sh
│       ├── return0.c
│       ├── return42.c
│       └── return255.c
└── examples/
    └── return42.c
```

**e2e 期望值约定**：每个 `test/e2e/*.c` 的第一行注释是 `// expect: N`。

---

## 5. 各模块规格

### 5.1 通用工具（`common.h/c`）

C 没有 `std::vector` / `std::string`，起步需要几个最小工具。**保持极简**——只做当前必需，够用即可。

```c
// 动态字节缓冲（用于源码读入、汇编输出）
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buffer;

void buffer_init(Buffer *b);
void buffer_free(Buffer *b);
void buffer_append(Buffer *b, const char *s, size_t n);
void buffer_appendf(Buffer *b, const char *fmt, ...);   // 用 vsnprintf

// 通用动态数组：Slice 1 用 macro-free 的显式类型即可（见 TokenArray、FuncArray）
// 不引入 generic vector 宏——违反 §3 编码约束

// 错误报告
void die_at(const char *file, int line, int col, const char *fmt, ...);
```

### 5.2 Token（`include/fakecc/token.h`）

```c
typedef enum {
    TK_KW_PACKAGE,   // "package"
    TK_KW_IMPORT,    // "import"（Slice 1 仅识别，不允许使用；见 §5.5）
    TK_KW_INT,       // "int"
    TK_KW_RETURN,    // "return"
    TK_IDENT,
    TK_INT_LITERAL,
    TK_STRING_LITERAL,   // 本切片仅为 import 保留，未使用
    TK_LPAREN, TK_RPAREN,
    TK_LBRACE, TK_RBRACE,
    TK_SEMICOLON,
    TK_EOF,
} TokenKind;

typedef struct {
    const char *file;   // 指向 driver 里长期存活的 filename 字符串
    int line;
    int col;
} SourceLoc;

typedef struct {
    TokenKind kind;
    char *text;         // strdup 出来的原始文本；owner = TokenArray
    SourceLoc loc;
} Token;

typedef struct {
    Token *data;
    size_t len;
    size_t cap;
} TokenArray;

void token_array_init(TokenArray *a);
void token_array_free(TokenArray *a);
void token_array_push(TokenArray *a, Token t);
```

### 5.3 Lexer（`lexer.h/c`）

**接口**：
```c
// 一次性把 source 全部 token 化，写入 out。失败时 die_at 退出。
void lex(const char *source, const char *filename, TokenArray *out);
```

**行为**：
- 跳过空白（空格、`\t`、`\n`、`\r`）
- 跳过 `//` 单行注释和 `/* */` 块注释
- 识别关键字 `package`、`import`、`int`、`return`
- 整数字面量：`[0-9]+`
- 字符串字面量：`"..."`，简单转义 `\\` `\"`（Slice 1 未真正消费）
- **明确拒绝预处理器语法**：若遇到行首 `#`，`die_at` 报错 `filename:line:col: error: preprocessor directives are not supported in FakeCC`
- 遇到其他未知字符：`die_at` 报 `error: unexpected character 'x'`
- 每个 token 记录 `SourceLoc`

### 5.4 AST（`ast.h`）

Slice 1 只需要四个节点类型，用 plain struct + 值语义即可。

```c
typedef struct {
    int value;
    SourceLoc loc;
} ReturnStmt;

typedef struct {
    char *name;         // strdup
    ReturnStmt body;
    SourceLoc loc;
} FunctionDecl;

typedef struct {
    char *name;         // strdup
    SourceLoc loc;
} PackageDecl;

typedef struct {
    FunctionDecl *data;
    size_t len;
    size_t cap;
} FunctionArray;

typedef struct {
    PackageDecl package;
    FunctionArray functions;
} TranslationUnit;

void tu_init(TranslationUnit *tu);
void tu_free(TranslationUnit *tu);
```

**故意不做通用 Expr/Stmt 基类**——C 里 tagged union 就够了，但 Slice 1 都不需要。**不为 import 建 AST 节点**。

### 5.5 Parser（`parser.h/c`）

**接口**：
```c
// 消费 tokens，产出 tu。失败 die_at 退出。
void parse(const TokenArray *tokens, TranslationUnit *tu);
```

**语法**（EBNF）：
```
translation-unit  = package-decl function-decl EOF
package-decl      = "package" IDENT ";"
function-decl     = "int" IDENT "(" ")" "{" return-stmt "}"
return-stmt       = "return" INT_LITERAL ";"
```

**行为**：
- 递归下降
- **必须**以 `package` 声明开头，否则报错 `error: expected 'package' declaration at start of file`
- 若遇到 `import` token，报错 `error: 'import' is not supported in Slice 1`
- 函数名必须是 `main`
- 其他语法错误：`error: expected X but got Y`

### 5.6 Sema（`sema.h/c`）

**本切片几乎是空的**，但必须存在这一层，为后续切片留位置。

```c
void sema_check(const TranslationUnit *tu);
```

**行为**：
- 断言 `package.name == "main"`
- 断言恰好有一个函数且名为 `main`
- 断言 `return` 值在 `[0, 255]` 范围内

### 5.7 Codegen（`codegen.h/c`）

**接口**：
```c
// 生成 AT&T 语法 x86-64 汇编，写入 out（追加）
void codegen(const TranslationUnit *tu, Buffer *out);
```

**输出示例**（对于 `package main; int main() { return 42; }`）：
```asm
    .text
    .globl main
    .type main, @function
main:
    pushq %rbp
    movq %rsp, %rbp
    movl $42, %eax
    popq %rbp
    ret
```

序言/尾声即使冗余也保留——**为后续切片的栈帧留位置**。

### 5.8 驱动（`main.c`）

命令行：
```
fakecc <input.c> -o <output>
```

流程：
```
1. 读文件到 buffer
2. lex(source, filename, &tokens)
3. parse(&tokens, &tu)
4. sema_check(&tu)
5. codegen(&tu, &asm_buf)
6. 写入临时 .s 文件（/tmp/fakecc_<pid>.s）
7. system("gcc -x assembler /tmp/fakecc_<pid>.s -o <output>")
8. 删除临时文件
```

任何阶段失败，`die_at` 直接输出 stderr 并 `exit(1)`。

---

## 6. 构建

`CMakeLists.txt` 顶层：
```cmake
cmake_minimum_required(VERSION 3.20)
project(fakecc C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter)

add_library(fakecc_core
    src/common.c
    src/lexer.c
    src/parser.c
    src/sema.c
    src/codegen.c
)
target_include_directories(fakecc_core PUBLIC include)

add_executable(fakecc src/main.c)
target_link_libraries(fakecc PRIVATE fakecc_core)

enable_testing()
add_subdirectory(test)
```

构建：
```bash
cmake -S . -B build
cmake --build build --parallel
```

---

## 7. 测试

### 7.1 极简测试框架（`test/test_framework.h/c`）

不引入 GoogleTest/Unity 等第三方。手写 ~40 行，接口：

```c
// test_framework.h
extern int t_total, t_failed;

void t_report(const char *file, int line, const char *expr);

#define T_ASSERT(cond) do { \
    t_total++; \
    if (!(cond)) { t_failed++; t_report(__FILE__, __LINE__, #cond); } \
} while (0)

#define T_ASSERT_EQ_INT(a, b) T_ASSERT((a) == (b))
#define T_ASSERT_STR_EQ(a, b) T_ASSERT(strcmp((a), (b)) == 0)

int t_finalize(void);   // 打印总计，返回 exit code
```

每个 `test_*.c` 有一个 `int main()`，调用一系列 `test_xxx()` 函数，最后 `return t_finalize();`。CMake 里为每个 test 二进制注册 `add_test`。

**注**：这里 `T_ASSERT` 用了函数宏，属于 §3 允许的"简短无害内联宏"。翻译到 FakeCC 时，改成普通函数（丢失 `__FILE__/__LINE__` 精度可接受，测试代码不参与自举）。

### 7.2 单元测试用例

**Lexer**：
- `"package"` → `[TK_KW_PACKAGE, TK_EOF]`
- `"int"` → `[TK_KW_INT, TK_EOF]`
- `"return"` → `[TK_KW_RETURN, TK_EOF]`
- `"42"` → `[TK_INT_LITERAL("42"), TK_EOF]`
- `"package main; int main() { return 42; }"` → 完整 token 序列
- 未知字符 `"@"` → 报错并 exit
- **`#include <stdio.h>` → 报错，错误信息含 "preprocessor directives are not supported"**
- 空输入 → `[TK_EOF]`
- 注释：`"// hi\npackage"` → `[TK_KW_PACKAGE, TK_EOF]`
- 位置追踪：多行输入，line/col 正确

**Parser**：
- 有效程序 → 正确 AST，`tu.package.name == "main"`
- 缺 `package` 声明 → 报错
- 缺 `;` → 报错
- 出现 `import` → 报错
- 函数名不是 `main` → 报错
- `return` 后无数字 → 报错

**Codegen**：
- `return 0;` → 汇编含 `movl $0, %eax`
- `return 42;` → 汇编含 `movl $42, %eax`

单元测试中"报错并 exit"的验证：把 lex/parse 的错误分支单独暴露一个非退出版本，或在测试里 `fork` 子进程调用 die 分支后检查退出码。**Slice 1 推荐后者不做**——错误路径只在 e2e 层验证（跑一个 `bad_*.c` 期望 fakecc 退出码非零）。

### 7.3 e2e 测试

`test/e2e/run_e2e.sh`：
```bash
#!/usr/bin/env bash
set -euo pipefail
FAKECC=${1:-./build/fakecc}
FAIL=0
for src in test/e2e/*.c; do
    expect=$(grep -oP '(?<=// expect: )\d+' "$src" | head -1)
    "$FAKECC" "$src" -o /tmp/fakecc_e2e.out
    /tmp/fakecc_e2e.out; got=$?
    if [ "$got" = "$expect" ]; then
        echo "PASS $src"
    else
        echo "FAIL $src (expected $expect, got $got)"
        FAIL=1
    fi
done
exit $FAIL
```

初始 e2e 用例：
```c
// test/e2e/return0.c
// expect: 0
package main;
int main() { return 0; }
```
```c
// test/e2e/return42.c
// expect: 42
package main;
int main() { return 42; }
```
```c
// test/e2e/return255.c
// expect: 255
package main;
int main() { return 255; }
```

CTest 注册：
```cmake
add_test(NAME e2e
         COMMAND bash ${CMAKE_SOURCE_DIR}/test/e2e/run_e2e.sh $<TARGET_FILE:fakecc>
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
```

---

## 8. 实现推荐顺序（TDD）

**每一步都是"先写测试红 → 写代码绿 → 提交"**。

1. **CMake + 空 main.c**：能构建、能运行、退出码 0。
2. **common.c 里 Buffer + die_at**：最小工具。
3. **test_framework**：40 行 assert 框架。
4. **Token 定义 + Lexer 最小实现**：只识别 `package`。补单元测试。
5. **Lexer 扩展**：识别所有关键字、标识符、整数、标点、字符串。
6. **Lexer 反预处理器检查**：遇到 `#` 报专门错误。
7. **AST 结构体**：只定义。
8. **Parser 最小实现**：只解析 `package main; int main() { return 0; }`。
9. **Parser 扩展**：接受任意整数字面量、任意 package 名。
10. **Sema 空壳**：校验 package 名、main 存在、退出码范围。
11. **Codegen**：hardcode 输出 `return 0` 的汇编。
12. **Codegen 扩展**：把 return 值填进 `movl`。
13. **Driver**：串起来，能生成 `.s` 文件。
14. **Driver 扩展**：调 `gcc` 汇编+链接。
15. **e2e 脚本 + return0.c**：端到端跑通。
16. **添加 return42.c、return255.c**。
17. **CI**：GitHub Actions 跑 `ctest`。

**第 15 步是核心里程碑**——端到端跑通 `return 0`。**之前不做任何 return != 0 的支持**。

---

## 9. 验收清单

- [ ] `cmake --build build` 无警告无错误（`-Wall -Wextra -Werror`）
- [ ] `ctest --test-dir build` 全绿
- [ ] `./build/fakecc examples/return42.c -o /tmp/a.out && /tmp/a.out; echo $?` 输出 42
- [ ] 至少 3 个 e2e 用例（return0/42/255）全部通过
- [ ] 至少 12 个单元测试（含预处理器拒绝用例）
- [ ] 全部源码是 **C99**，且遵守 §3 的"Stage 0 编码约束"（无代码生成宏、无条件编译、无变参宏）
- [ ] README 说明构建、运行、FakeCC 与 C 的差异、以及自举路线（§11）
- [ ] 代码总量在 800-1500 行 C 之间
- [ ] 错误报告格式为 `filename:line:col: error: <message>`，退出码非零
- [ ] 见到 `#include`、`#define` 等预处理器指令时给出**明确指出这是语言设计约束**的错误信息

---

## 10. 交付给下一个切片的接口

后续切片会分几条主线推进：

**语言核心增强**：
- 扩展 `TokenKind`（运算符、更多关键字）
- 扩展 `AST`（引入 `Expr` tagged union）
- 扩展 `Parser`（表达式优先级）
- 让 `Sema` 真正做类型检查
- 引入 IR 层（当支持控制流时）

**FakeCC 特有能力**：
- 多文件编译（同一 package 多 `.c` 文件合并）
- `import` 语句解析与符号导入
- 跨 package 符号解析（替代头文件的核心机制）
- 首字母大小写可见性检查
- package 路径映射到目录结构

**唯一原则**：**做最小实现，但不写会阻碍扩展的死路**。可接受的死路（下切片重构）：
- `ReturnStmt.value` 存 `int`（下切片必须换成 `Expr` union）
- Codegen 里 hardcode `main` 函数名
- `TranslationUnit` 未来升级为 `Package`（含多个 `TranslationUnit`）

不做过度设计：不预先建 IR、不预先支持多后端、不预先实现 import。

---

## 11. 自举路线图（Bootstrapping）

FakeCC 的最终目标是**自己编译自己**。分三阶段：

### Stage 0 — C99 实现（本计划所在阶段）

- 用 C99 编写 `fakecc-c`，用系统 gcc 编译
- 严格遵守 §3 编码约束：
  - `#include` 只用于 libc 和项目内部头文件（内部头文件用 `#ifndef` include guard，这是唯一允许的条件编译）
  - `#define` 只能定义**无参**常量或**简短无害内联宏**（如 `ARRAY_LEN`、测试框架的 `T_ASSERT`）
  - 禁止代码生成宏、变参宏、条件编译（除 include guard 外）
- Stage 0 覆盖到能**编译 FakeCC 本身**所需的全部语言特性为止（Slice 1 到 Slice N）

### Stage 1 — 机械翻译到 FakeCC

一旦 Stage 0 的语言能力足以覆盖编译器自身，写一个**一次性翻译脚本**（Python 或 shell）将 `src/*.c` 翻译成 FakeCC 版本：

- 展开所有 `#include`（内部头文件）为其内容，去掉 include guard
- 保留 libc 头（后期可能引入 FakeCC 的 stdlib 绑定层）
- 展开无害宏（常量宏和简短函数宏，直接 inline）
- 在每个文件顶部加 `package fakecc;`
- 首字母大小写按导出/私有语义手动调整（或翻译脚本按启发式处理）
- **产物**：`src-fakecc/*.c` — 用 FakeCC 编写的 fakecc

用 Stage 0 编译 Stage 1 的源码 → 得到 `fakecc-1` 二进制。若 `fakecc-1` 能通过全部现有测试，Stage 1 完成。

### Stage 2 — 自我编译验证

用 `fakecc-1` 编译 `src-fakecc/*.c` → 得到 `fakecc-2`。

若 `fakecc-2` 的二进制与 `fakecc-1` **逐字节相同**（或至少能通过全部测试），**自举成功**。

之后开发只维护 `src-fakecc/`，`src/` 只作为紧急回退和参考。

---

## 完成信号（Slice 1）

在一台干净的 Ubuntu 上：
```bash
git clone <repo> && cd fakecc
cmake -S . -B build && cmake --build build --parallel
ctest --test-dir build --output-on-failure
```
全绿，且：
```bash
cat > /tmp/t.c <<'EOF'
package main;
int main() { return 123; }
EOF
./build/fakecc /tmp/t.c -o /tmp/t
/tmp/t; echo $?    # 输出 123
```
成功 → Slice 1 完成，进入 Slice 2（算术表达式）。
