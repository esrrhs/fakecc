# arm64-apple-macos 原生后端 - 产品需求文档

## Overview
- **Summary**: 为 fakecc 新增一个完整的 arm64（AArch64）代码生成器与 Mach-O 链接后端，使 fakecc 在 Apple Silicon Mac 上能**本机**（不依赖 Rosetta、不依赖 QEMU/虚拟机）编译并运行其支持的全部代码，`ctest` 全部 36 个测试在 macOS arm64 上通过；同时保持现有 linux-x86_64 目标零回归。
- **Purpose**: 当前 fakecc 只有 x86-64 Linux ELF 单一后端，Mac 上产物只能进 QEMU 虚拟机运行；开发反馈环慢，且无法利用本机工具链（clang/lldb）做差分。原生 arm64 后端把开发/测试闭环搬到本机。
- **Target Users**: fakecc 编译器的开发者与 CI（macOS Apple Silicon runner）；在 Mac 上使用 fakecc 编译 freestanding 程序的用户。

## Goals
- 在 Apple Silicon Mac 上，fakecc 本体可构建、可运行，能把 fakecc 方言源码编译为**原生 arm64 Mach-O** 并直接执行。
- 覆盖现有编译器全部能力：-O0/-O1、`-g` 调试信息、`-shared`（dylib）、`-l/-L` 动态库互操作、线程 runtime、TLS、sanitizer、变长参数、向量类型、构造/析构函数数组、自举固定点。
- `ctest` 的 36 个测试目标在 macOS arm64 全部以退出码 0 通过（含 difftest 以本机 clang 为 oracle、debug 测试以 lldb 为驱动）。
- linux-x86_64 目标行为零回归。
- 架构上把"前端/IR/优化/寄存器分配算法"等中立层与"目标后端（指令选择、ABI、对象格式、链接、平台 runtime）"彻底分层，为未来第三个目标（如 aarch64-linux）留出干净的扩展点。

## Non-Goals
- 不交付 aarch64-linux（arm64 ELF）后端。
- 不交付 x86_64-apple-macos（Rosetta）输出后端；Rosetta 仅作为本项目前期 Mach-O 技术验证手段。
- 不支持 iOS/iPadOS（codesign/entitlement、沙盒、App Store 部署）。
- 不追求 arm64 代码生成质量追平 clang -O2；-O1 优化水准对标现有 x86-64 后端即可。
- 不重写前端、IR 序列化、包系统、C 预处理之外的既有中立逻辑。
- 不引入外部代码生成库（如 LLVM）；保持 fakecc 单仓库自包含风格。

## Background & Context

### 已实证的技术约束（2026-10-02，macOS 26.5.2 / Apple Silicon）

1. **arm64 macOS 内核拒绝纯静态可执行文件**：无 `LC_LOAD_DYLINKER` 的 arm64 Mach-O（无论是否 ad-hoc 签名）启动即被 SIGKILL（实测 137）。因此 arm64 产物**必须**经 dyld。
2. **freestanding 动态形态可行**：仅带 `LC_LOAD_DYLINKER`（/usr/lib/dyld）与一个 `LC_LOAD_DYLIB`（/usr/lib/libSystem.B.dylib，零符号导入也可）的二进制，自定义 `LC_MAIN` 入口（`-nostdlib -Wl,-e,_start -Wl,-lSystem` 实测）可正常运行；runtime 风格保持 Go 式 freestanding（自带 runtime 包，不调用 libc）。
3. **raw 系统调用可用**：在上述形态中直接执行 `svc #0x80`，x16=调用号（Darwin UNIX class = `0x2000000 | n`），x0-x5 传参，x0 返回——write/exit/mmap/munmap 实测全部成功。malloc 所依赖的匿名 mmap 可行。
4. **LC_MAIN 入口 ABI**：dyld 以函数调用方式进入，`x0=argc, x1=argv, x2=envp`，栈上为返回地址（实测 argc 随命令行参数精确变化：无参=1，两参数=3）。这比 Linux 读栈入口更直接，且天然提供环境变量（现有 Linux 路径靠读 `/proc/self/environ`，macOS 无 procfs）。
5. **arm64 强制 PIE**：arm64 Mach-O 可执行文件为位置无关可执行；A64 的 ADRP/ADD 页相对寻址天然支持，新 codegen 按 PIE 生成即可。
6. **ad-hoc 签名**：ld64 链接时自动写入 ad-hoc 代码签名；fakecc 自己写 Mach-O 时必须自行生成 `__LINKEDIT` 代码签名 blob（CodeDirectory 页哈希 + SuperBlob）或在输出后调用系统 `/usr/bin/codesign -s -`，产物需开箱可执行。
7. 对照证据（x86_64/Rosetta）：纯静态 LC_UNIXTHREAD + raw syscall 的 x86-64 Mach-O 可在 Rosetta 下运行（write/exit/mmap 全通），印证 Mach-O 布局知识；但这不是交付形态。

### 代码架构现状（已核对）

- **中立层**（可复用，预期零或极小改动）：lexer/parser/sema/AST、IR（ir.c，11083 行）、cfg/domtree/phi、mem2reg、opt、scalar_opt、dfp；寄存器分配**算法**（图染色/MCS）与架构无关，仅寄存器枚举与 caller-saved 掩码为 x86 特定（regalloc.h 已有配置化雏形）；包系统、driver 框架、测试框架。
- **x86 特定层**（arm64 需平行实现）：
  - codegen.c（6229 行）：IR→x86-64 指令、SysV 调用约定、`__syscall`/`__clone` 内建；
  - emit.c（1181 行）：ELF ET_REL 对象读写，重定位为 R_X86_64_* 族；
  - link.c（3039 行）：ELF 可执行/共享对象链接、PLT/GOT/TLS、入口 stub；
  - debug.c（1399 行）：ELF DWARF 节区、x86-64 寄存器号与 CFI；
  - runtime/：14 类 Linux syscall（read/write/open/close/lseek/mmap/munmap/getpid/unlink/chmod/gettid/futex/clone/getdents64/exit_group）；thread.c 依赖 clone(2)+futex(202)+`%fs` TLS；
  - v0/translate.py 自举：FAKECC_SELFHOST 下 raw Linux syscall。
- 测试：36 个 ctest 目标中 22 个单元测试、14 个端到端（含 shlib ×2、gdb ×1）。gdb 脚本有 `command -v gdb` 守卫；shlib 脚本当前硬编码 readelf/gcc/.so/DT_NEEDED；difftest 以系统 cc 为 oracle。

### Darwin 关键 ABI 映射（实现依据，细节在 Plan/实现期固化）

- syscall：read=3、write=4、open=5、close=6、exit=1、mmap=197、munmap=73、lseek=199、unlink=10、chmod=15、getpid=20（均加 0x2000000 class）；线程用 `bsdthread_create/terminate`(360/362) + `__ulock_wait/__ulock_wake`(515/516)；目录枚举无 getdents64，用 `getdirentries64`(344)；无 exit_group，exit 即线程终止。
- 调用约定：AAPCS64 + Darwin 变体（x18 平台保留；x29 FP/x30 LR；x0-x7 参数，x0 返回；结构体按 HFA/IDA 规则，过大间接返回（x8 存结果地址）；栈 16 字节对齐；varargs 需 register save area）。
- TLS：arm64 Darwin 经 TPIDR_EL0（`mrs xN, tpidr_el0`），与 Linux x86 的 `%fs:0` 模型不同；每线程 TCB 由 bsdthread_create 的 udata 建立。
- flags 差异：mmap MAP_ANON=0x1000、open O_CREAT=0x200 等与 Linux 数值不同，须经平台常量表，不得在业务 runtime 代码里写死 Linux 值。

## Functional Requirements

- **FR-1（目标选择）**：编译器支持显式 target 概念（至少 `x86_64-linux` 与 `arm64-macos` 两个），在 macOS arm64 主机默认 `arm64-macos`，在 Linux 主机默认 `x86_64-linux`，并提供显式覆盖（CLI 形式在实现时定，建议 `--target=`，同时不破坏既有选项）。
- **FR-2（arm64 代码生成）**：把现有全部 IR 运算符、控制流、聚合类型、变长参数、向量类型（NEON 128 位宽度）、`__syscall` 内建、构造/析构数组，以 -O0 与 -O1 两档正确 lowering 到 A64 指令；遵循 Darwin arm64 ABI。
- **FR-3（Mach-O 对象与链接）**：支持 `-c` 输出/读回 Mach-O 可重定位对象；链接生成 PIE 可执行（LC_MAIN、dyld、libSystem、__TEXT/__DATA/__LINKEDIT、arm64 重定位应用）；产物经 ad-hoc 签名后开箱可执行。
- **FR-4（dylib）**：`-shared` 生成 MH_DYLIB（导出 trie、未定义绑定、stub、必要的 chained-fixup/bind 编码）；`-l/-l:/ -L/-nostdlib` 在 macOS 语义下工作（LC_LOAD_DYLIB/LC_RPATH/LC_ID_DYLIB），支持双向互操作：fakecc 主程序调用 clang 生成的 .dylib，clang 主程序调用 fakecc 生成的 .dylib。
- **FR-5（Darwin runtime 平台层）**：runtime 包在 arm64-macos 目标下提供与 Linux 下等价的能力与行为：stdio 文件 I/O、malloc、字符串/转换、环境变量（入口 envp）、线程（pthread/thread create/join/self/exit、TLS `__thread`）、exit/退出码、sanitizer 影子内存、构造/析构函数。
- **FR-6（调试信息）**：`-g` 在 arm64-macos 下生成 lldb 可用的 DWARF（Mach-O `__DWARF` 段/节、arm64 寄存器号映射、正确 CFI）；`-g` 对代码生成零影响（与现有"加不加 -g 的 .text 字节一致"纪律相同）。
- **FR-7（自举）**：在 macOS arm64 上完成 v0/stage2 固定点（Stage0 编译自举源码→fakecc-1；fakecc-1 再编译→fakecc-2；二者字节一致），各阶段产物自动签名可执行。
- **FR-8（测试平台化）**：测试脚本识别主机平台：macOS 下 oracle 为系统 clang（arm64）、产物检查用 Mach-O 工具（otool/dyld_info 等）、动态库后缀 .dylib、debug 用 lldb；Linux 路径与 CI 保持现状；确属平台不适用的个例（如 STT_GNU_IFUNC）按显式、可审计的 skip 规则处理，不得静默吞失败。
- **FR-9（Linux 零回归）**：所有新增抽象不得以任何行为差异影响 x86_64-linux 目标；Linux CI 与 Alpine VM 的测试结论不劣化。

## Non-Functional Requirements

- **NFR-1（分层质量）**：目标相关代码集中在明确的后端边界（目标描述表、codegen_arm64、Mach-O emit/link、平台 runtime 头、arm64 debug 分支）；中立层不得出现 `#ifdef __APPLE__`/`#ifdef __aarch64__` 式散落分支，平台差异一律经目标描述/平台头表达。
- **NFR-2（-O0 代码质量基线）**：arm64 后端 -O0 在既有 bench 套件（可在 macOS 运行的子集）上相对 clang -O0 的几何平均运行时间比不高于 2.0×（新后端首版宽阈值，防止生成器失控；后续再收紧到 x86 后端同档水平）。
- **NFR-3（可维护性）**：新增代码通过项目既有的 -Wall -Wextra -Werror；arm64 指令编码与 ABI 决策有注释指向所依据的 ABI/内核行为；不引入新第三方构建依赖（codesign 为系统自带工具，不计为外部依赖）。
- **NFR-4（可重复验证）**：所有验收结论可用一条命令（ctest）与固定脚本复现；关键 Mach-O 结构/签名/重定位以自动化检查固定下来，不依赖人工肉眼判断。

## Constraints
- **Technical**:
  - 产物必须是 arm64 Mach-O 且经 dyld（内核禁止纯静态 arm64，FR-3 的形态约束）。
  - 只能使用 A64 指令集与 Darwin 支持的 syscall；x18 不可挪用；不得使用 arm64e PAC 指令（保持 CPU_SUBTYPE_ARM64_ALL 兼容）。
  - 代码生成仍需兼容 fakecc 自举方言（子集 C），自举源码经 v0/translate.py 生成后必须能被 arm64 后端编译。
  - 调试器只有 lldb 现实可用（macOS 上 gdb 对原生 arm64 进程不可行）。
- **Business**: 不改变 fakecc 对外 CLI 的既有语义，新增选项须向后兼容。
- **Dependencies**: Xcode Command Line Tools（clang/ld64/lldb/codesign，开发机已具备）；运行环境 macOS（Apple Silicon）。

## Assumptions
- 开发机为 Apple Silicon、macOS ≥ 当前实测版本（26.x），已装 Xcode CLT 与 ad-hoc 签名所需能力；CI 可使用 macos arm64 runner。
- Darwin raw syscall 与 freestanding 动态形态在目标 macOS 版本上持续可用（已对当前版本实测；若未来内核收紧，退路是把少量入口点改为经 libSystem trampoline，不影响整体分层）。
- 现有 bench 套件可在 macOS 直接编译运行（平台化小改在任务内消化）。
- 36 个 ctest 中 Linux 专有用例（ifunc 等）数量很少，显式平台 skip 不削弱 macOS 验收的有效性；difftest 以 clang arm64 为 oracle 时，既有"fakecc 与 gcc 行为一致"的用例若依赖 glibc/musl 差异，按已有原则（环境差异非编译器回归）处理。

## Acceptance Criteria

### AC-1: macOS 上 fakecc 本体原生构建
- **Type**: `rule`
- **Given**: Apple Silicon Mac、Xcode CLT 已安装、仓库干净
- **When**: 在 build 目录执行 cmake 配置与构建
- **Then**: 生成 arm64 原生 fakecc 可执行文件（`file` 显示 Mach-O arm64），可直接运行（`fakecc` 无参数/帮助路径不崩溃）
- **Pass Condition**: 构建零错误，fakecc 本体为原生 arm64 Mach-O 且本机运行退出正常
- **Evidence**: 构建日志、`file build/fakecc`、运行输出

### AC-2: freestanding 程序本机编译执行
- **Type**: `rule`
- **Given**: 含 `import runtime; runtime.printf(...)` 与 main 返回指定退出码的 fakecc 方言程序
- **When**: 用本机 fakecc 编译并执行产物
- **Then**: 标准输出内容正确、退出码精确为主程序返回值；`file` 显示 Mach-O arm64（非 x86_64）；产物含 LC_MAIN/dyld 且已签名，直接执行无需用户手动 codesign
- **Pass Condition**: 输出与退出码逐字节正确，二进制为原生 arm64 PIE 且开箱可运行
- **Evidence**: 编译/运行命令输出、`file`/`otool -l`/`codesign -dv` 结果

### AC-3: ctest 全部 36 个测试目标在 macOS arm64 通过
- **Type**: `rule`
- **Given**: 全新构建目录
- **When**: 执行 `ctest`（全部 1-36，含 -O0/-O1 两轮端到端、difftest、shlib、gdb→lldb、cli）
- **Then**: 36/36 测试目标退出码 0；平台不适用个例必须由脚本显式标注 skip 原因（ifunc 等 Linux 专有），除显式清单外不允许用例失败或被静默跳过
- **Pass Condition**: ctest 汇总 "100% tests passed, 0 tests failed out of 36"，skip 清单写入任务证据并经 Review 核对
- **Evidence**: ctest 完整输出、显式 skip 清单

### AC-4: arm64 调试体验（lldb）
- **Type**: `rule`
- **Given**: cases/debug 用例以 -g 编译
- **When**: 用 lldb 在 `// BRK` 行断点停靠，按用例 gdb 注解执行等价的变量打印/回溯
- **Then**: 每个 `expect`/`gdb_expect` 等价断言成立、`gdb_reject`（如 "optimized out"）不出现；-O0 与 -O1 两档均通过；带不带 -g 的 __TEXT 代码字节一致
- **Pass Condition**: e2e_gdb（lldb 平台路径）退出 0，且代码字节一致性检查通过
- **Evidence**: 测试输出与 lldb 会话记录

### AC-5: dylib 双向互操作
- **Type**: `rule`
- **Given**: (a) clang 生成 libadd.dylib，fakecc 主程序 extern 调用；(b) fakecc `-shared` 生成 dylib，clang 主程序调用；(c) fakecc 程序调系统 libSystem 中的函数（如 printf）
- **When**: 分别编译执行
- **Then**: 三个程序运行结果正确；otool/dyld_info 可见正确的 LC_LOAD_DYLIB/LC_RPATH/LC_ID_DYLIB；fakecc 生成的 dylib 为合法 arm64 MH_DYLIB
- **Pass Condition**: 三向运行均返回预期结果且动态标签检查通过
- **Evidence**: e2e_shlib macOS 路径输出与二进制标签检查

### AC-6: 线程与 TLS 在 Darwin 原生工作
- **Type**: `rule`
- **Given**: 使用 pthread/thread create+join、`__thread` 变量、线程局部写读、join 回收栈/TLS 的多线程程序
- **When**: 本机编译并重复执行（含 -O0/-O1）
- **Then**: 线程函数执行、每线程 TLS 值隔离正确、join 拿到返回值、进程干净退出（无挂死、无崩溃、退出码 0）
- **Pass Condition**: 多线程用例稳定通过（连续多次，含 e2e 多线程相关用例）
- **Evidence**: 用例运行记录（重复次数与结果）

### AC-7: macOS 自举固定点
- **Type**: `rule`
- **Given**: macOS 上的 Stage0 fakecc
- **When**: 执行 v0/stage2 固定点检查（平台化脚本）
- **Then**: fakecc-1 与 fakecc-2 字节完全一致（"FIXED POINT REACHED"），两阶段二进制均为已签名 arm64 Mach-O 且可运行
- **Pass Condition**: 固定点检查退出 0 并产出可运行的 fakecc-1/fakecc-2
- **Evidence**: stage2 脚本输出与产物 file/codesign 检查

### AC-8: linux-x86_64 零回归
- **Type**: `rule`
- **Given**: 完成全部改动后的仓库
- **When**: 在 Linux x86_64 环境（Alpine VM 与/或 CI）构建并运行 ctest
- **Then**: 测试结果不劣于改动前基线（既有的 28/36 环境性差异清单保持一致，不新增任何失败；unit 22/22 保持）
- **Pass Condition**: 与记录在案的基线逐项对比无新增失败，x86 产物字节不因重构而变化（关键 .o/二进制对比无差异或差异可解释）
- **Evidence**: Linux 侧 ctest 输出与基线对比表

### AC-9: 后端分层架构质量
- **Type**: `rubric`
- **Dimension**: 目标抽象与中立层保护
- **Scale**: 1-5
- **Anchors**: 1 = 平台差异以 #ifdef 散落在前端/IR/优化各处，x86 路径行为被污染；3 = 主要差异集中在新文件，但中立层仍有少量平台分支且无统一表达；5 = 全部平台差异经目标描述表/平台头/后端接口表达，中立层零平台分支，新增第三目标只需实现后端接口
- **Pass Threshold**: >= 4
- **Evidence**: 代码审查（中立文件 diff、目标接口清单、平台分支 grep 统计）

### AC-10: arm64 -O0 代码生成质量基线
- **Type**: `rubric`
- **Dimension**: 生成代码相对 clang -O0 的运行时性能
- **Scale**: 1-5
- **Anchors**: 1 = 几何平均 >3× clang -O0 或存在显著失控的指令膨胀；3 = 几何平均 ≤2.0×，热点形态（紧凑循环/函数调用/除法）无异常爆炸；5 = ≤1.3×，与 x86 后端同档水准
- **Pass Threshold**: >= 3（即几何平均 ≤2.0×，满足 NFR-2）
- **Evidence**: bench 套件双编译器计时结果（中位数/几何平均、分用例数据）

### AC-11: 工程可维护性与验证可重复性
- **Type**: `rubric`
- **Dimension**: 代码整洁度、注释与自动化验证完备性
- **Scale**: 1-5
- **Anchors**: 1 = 新增代码编译告警丛生、ABI/编码无出处、验证靠手工临时命令；3 = 无 -Werror 违规、关键 ABI 决策有注释、主路径有自动化检查；5 = 全部新模块有与风格一致的注释与测试覆盖、Mach-O/签名/ABI 有结构化自动断言、复现命令文档化于测试脚本
- **Pass Threshold**: >= 4
- **Evidence**: 构建告警记录、新增测试清单、复现命令与代码审查

## Open Questions
- [ ] 代码签名实现路径：fakecc 内置签名 blob 生成 vs 输出后 fork `/usr/bin/codesign`。倾向先 codesign（快速闭环），自举能力打通后内置（消除对外部进程的依赖）；最终取舍在 Plan 阶段按任务风险确定，但不影响 AC-2"开箱可运行"的要求。
- [ ] `--target=` 之外是否保留/如何处理既有 `-mavx/-mavx512f` 等 x86 专用向量开关在 arm64 目标下的语义（拒绝报错或映射为 NEON 宽度协商），Plan 阶段定。
- [ ] Darwin syscall errno 取值与 Linux errno 表不同，runtime 是否存在依赖具体 errno 数值的代码；实现期排查并经平台表归一。
- [ ] sanitizer 影子内存固定高地址 hint（0x100000000000 段）在 Darwin PIE/ASLR 下的可用性需实现期实测；若保留区间不可用，需 Darwin 专属影子基址策略（行为保持兼容）。
- [ ] bench 套件在 macOS 的可运行子集与计时方法（无 perf/TCG 稀释，直接本机计时），Plan 阶段确定用于 AC-10 的用例集。
