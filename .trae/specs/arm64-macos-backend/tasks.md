# arm64-apple-macos 原生后端 - 实施计划

> 说明：任务按"最小垂直闭环优先"排序——尽早（T1-T5）打通 host 编译器→arm64 代码→Mach-O PIE→dyld→本机执行的完整链路，之后每个 codegen/runtime 补丁都能用本机运行自证。每个任务完成时必须记录 Completion Evidence。Linux x86_64 行为受架构重构影响的节点（T1/T2/T14/T15）执行前后做产物字节对比。

## Task 1: 目标三元组与 TargetDesc 抽象
- **Status**: `completed`
- **Priority**: high
- **Depends On**: None
- **Description**:
  - 定义 target 三元组（`x86_64-linux`、`arm64-macos`）与贯穿编译流水线的 `TargetDesc`：arch、os、指令集族、调用约定描述、syscall 表选择、对象格式（ELF/Mach-O）、向量宽度协商。
  - driver（main.c）：macOS arm64 主机默认 arm64-macos，Linux 主机默认 x86_64-linux；新增 `--target=<triple>` 显式覆盖；`-mavx*` 在 arm64 目标下按 Open Question 结论处理（明确报错或映射 NEON，需有测试锁定行为）。
  - 平台差异只允许出现在 TargetDesc/后端接口/平台 runtime 头；中立层（lexer/parser/sema/ir/cfg/domtree/phi/mem2reg/opt/scalar_opt/dfp）禁止新增平台分支。
- **Acceptance Criteria Addressed**: FR-1, FR-9, AC-3, AC-8, AC-9
- **Test Requirements**:
  - `rule` TR-1.1: 在 macOS arm64 主机 `fakecc --help`/无参运行正常，默认目标=arm64-macos；在 Linux 主机默认=x86_64-linux；`--target=` 双向可覆盖，非法 triple 明确报错（证据：两平台命令输出）。
  - `rule` TR-1.2: 本任务只做接线、不改 x86 codegen：重构前后对同一组 .c 在 x86_64-linux 目标下生成的对象/二进制与重构前字节一致（证据：重构前后产物 `cmp -l` 为空）。
  - `rubric` TR-1.3: 中立层保护；scale 1-5；anchors 同 AC-9；threshold >=4；证据：中立文件 diff 为空 + 全仓平台分支 grep 统计（仅允许 TargetDesc/后端/平台头命中）。
- **Notes**: 此时 arm64 目标可以"声明未实现并报清晰错误"，真正生成在后续任务接入。
- **Completion Evidence**:
  - TR-1.1（pass）：macOS arm64 默认目标实测——无 --target 编译返回 `fakecc: target 'arm64-macos' is not supported by this build yet`；`--target=x86_64-linux`/`-target x86_64-unknown-linux-gnu`/`--target=aarch64-apple-darwin` 等别名均正确解析；非法 triple（mips-linux）报 `unrecognized target triple ... (supported: x86_64-linux, arm64-macos)`；arm64 下 `-mavx` 明确拒绝、`-mno-avx` 作为 no-op 接受；usage 已更新。新增 test_target 单测 39/39 通过（canonical/aliases/bad/current/host-default）。Linux 默认分支经代码路径确认为 x86_64-linux（T25 VM 全量复验）。
  - TR-1.2（pass）：改动前基线保存于 /tmp/t1_baseline/（r42.o md5 8ff3dd33…、hello4 md5 24dd2c58…）；改动后 `--target=x86_64-linux` 产物 `cmp` 逐字节一致（OBJ_IDENTICAL / EXE_IDENTICAL）。ctest 单测 20/22，唯一失败仍是改动前即存在的 5 个"macOS 执行 x86 ELF"环境断言（test_emit 行 90/106/122、test_link 行 91/129，仅行号因插入位移 +1）。
  - TR-1.3（5/5）：host 探测宏（__APPLE__/__aarch64__/__linux__）全仓仅 src/target.c 一处；目标分支只出现在后端入口（codegen.c:3097、emit.c:310/706、link.c:1049）与 driver 选项校验（main.c:427）；lexer/parser/sema/ast/ir/cfg/domtree/phi/mem2reg/opt/scalar_opt/dfp/regalloc/common/pkg 零平台分支。新增第三目标仅需：target.c 加描述符 + 三个后端入口分发。实现文件：include/fakecc/target.h、src/target.c、CMakeLists.txt、src/main.c、src/{codegen,emit,link}.c 防御性分发、5 个 x86 后端单测显式 pin 目标、test/unit/test_target.c 新注册（现 ctest 共 23 个单元测试目标）。

## Task 2: 寄存器分配器架构参数化
- **Status**: `completed`
- **Priority**: high
- **Depends On**: T1
- **Description**:
  - 把 regalloc 的物理寄存器枚举、可分配集合、caller/callee-saved 掩码从 x86 硬编码改为按 TargetDesc 提供的寄存器文件配置（GP 与 SIMD 两组）。
  - 增加 arm64 配置：GP 可分配集（排除 x18 平台保留、x29/x30 FP/LR、SP；x0/x1 归返回/暂存约定，x19-x28 callee-saved）、SIMD v0-v31（v0-v7/v16-v31 caller-saved，v8-v15 低 64 位 callee-saved）。
  - 分配算法本体（liveness/MCS/染色/spill）不得改算法语义。
- **Acceptance Criteria Addressed**: FR-2, AC-9
- **Test Requirements**:
  - `rule` TR-2.1: x86 配置下 regalloc 输出与现状逐值一致，x86 全部单元测试与既有产物字节不变（证据：单测 + 产物 cmp）。
  - `rule` TR-2.2: 新增 arm64 寄存器文件单元测试：可分配集计数、caller/callee 分类、跨调用掩码、spill 对齐（16B）符合 Darwin ABI（证据：新单测通过）。
- **Completion Evidence**:
  - 实现：新增 [include/fakecc/reg_arm64.h](file:///Users/mingming/project/fakecc/include/fakecc/reg_arm64.h)（A64 GP/V 寄存器硬件编号 0-31，与指令编码字段一致；GP 24 集：caller x2-x15 colors 0-13、callee x19-x28 colors 14-23；SIMD 30 集：caller v0-v7+v16-v29 colors 0-21、callee v8-v15 colors 22-29；v30/v31 留作 scratch；掩码 0x3FFF/0x3FFFFF）与 [src/reg_arm64.c](file:///Users/mingming/project/fakecc/src/reg_arm64.c)。regalloc.h 暴露中立的 `RaRegClass/RaRegClasses`（regs/nregs/caller_saved/spill_bytes/exclude_16byte_ld），regalloc.c 用 `ra_classes_current()` 按 target 取 x86（原 ALLOCATABLE_REGS/XMM 集，数值与顺序未动）或 arm64 描述符；颜色掩码加宽为 uint32_t 以容纳 30 色（x86 仅 9/14 色，语义不变）；long-double 排除（x87）改为类标志 exclude_16byte_ld（x86=1，arm64=0）；spill 槽宽按类（GP 8B 保持成对 16B 取整，XMM 32/64 动态，NEON 16B）。算法本体（liveness/MCS/染色/eviction/spill-cost）零语义改动。
  - TR-2.1（pass）：x86 产物 r42.o / hello4 与 T1 基线 `cmp` 逐字节一致（OBJ_IDENTICAL/EXE_IDENTICAL）；test_regalloc 原 8 个用例（含 16 值 spill 压力）在显式 pin x86 下全过；ctest 单元 20/22，唯一失败仍是 test_emit/test_link 的 5 个 macOS 执行 x86 ELF 环境断言（与 T1 同集）。
  - TR-2.2（pass）：test_regalloc 扩到 211 个断言全绿，新增：x86 描述符不变量（GP 9/mask 0x3F/8B、XMM 14/mask/32|64/exclude=1）；arm64 描述符（GP 24 且颜色 0-13↔x2-x15 caller、14-23↔x19-x28 callee；SIMD 30 且 0-21 caller、22-29 callee；spill 16B；reserved x0/x1/x16/x17/x18/x29/x30/x31 与 v30/v31 不入集）；同一"跨 call 存活值"IR 在 arm64 必分到 x19-x28、在 x86 必分到 RBX/R12/R13；26 个同时活跃值（>24）触发 spill 且 stack_size 保持 16B 对齐。

## Task 3: A64 指令编码器（codegen_arm64 基础设施）
- **Status**: `completed`
- **Priority**: high
- **Depends On**: T2
- **Description**:
  - 新建 codegen_arm64 独立模块（不改动 codegen.c x86 路径）：可变长指令缓冲、标签/重定位 patch 机制；编码 movz/movk/movn（64 位立即数物化）、mov（寄存器）、add/sub/and/orr/eor（立即数与寄存器）、mvn/neg 别名、lsl/lsr/asr、mul/udiv/sdiv/msub、cmp + b.cond、cbz/cbnz、b/bl（B/BL 26 位偏移 patch）、ret、adr/adrp（ADRP 页 patch）、ldr/str（立即数有符号偏移 12 位 scaled、register offset、literal）、stp/ldp、svc #0x80、scvtf/fcvtzs/fmov/fadd/fmul/fdiv（标量 S/D）。
  - 所有编码附 A64 架构手册出处注释；不支持 arm64e/PAC 指令。
- **Acceptance Criteria Addressed**: FR-2, AC-11
- **Test Requirements**:
  - `rule` TR-3.1: 编码器单测：对每类指令的典型/边界操作数，与 `clang -arch arm64` 汇编同一助记符产出的机器码逐字节一致（抽样覆盖立即数边界 0/最大、页边界、正负分支偏移、scaled 偏移）（证据：单测表）。
  - `rule` TR-3.2: 标签/分支/ADRP patch 在 ±128MB/页边界的往返正确（证据：单测构造的远距离 patch 用例）。
- **Completion Evidence**:
  - 实现：[include/fakecc/a64.h](file:///Users/mingming/project/fakecc/include/fakecc/a64.h) + [src/a64.c](file:///Users/mingming/project/fakecc/src/a64.c)。A64Asm 本地标签汇编器（label/fixup 池；b/bl=B26、b.cond/cbz/cbnz=B19、adrp=21 位页差；带范围/4 字节对齐/未绑定诊断）；指令覆盖全部清单项：movz/movk/movn + mov_imm64 物化、add/sub 立即数(shift12)/移位寄存器、逻辑立即数（EncodeBitMask 完整算法）与寄存器、UBFM/SBFM 移位别名、MADD/MSUB（o0 在 bit15）、UDIV/SDIV、cmp、8/16/32/64 位 ldr/str（含 ldrsw/ldrsb/ldrsh 与寄存器偏移）、stp/ldp（signed/pre-index）、ret、svc、adrp、标量 S/D 的 fmov/fadd/fsub/fmul/fdiv/fcmp/scvtf/fcvtzs/访存。寄存器编号直接用硬件字段 0-31，与 reg_arm64.h 分配器输出对齐。
  - TR-3.1（pass）：新增 [test/unit/test_a64.c](file:///Users/mingming/project/fakecc/test/unit/test_a64.c)，以 `clang -arch arm64`+`objdump -d` 预生成的 100 条指令金标准（GP45/load-store25/分支14/FP16，三标签组含真实分支位移）逐字节对照 100/100 通过；mov_imm64 对 0/0xffff/0x10000/全 1/0x1234000000005678 的 1-4 词物化与 clang 一致；非位掩码立即数 0x12345 正确拒绝。参考汇编 /tmp/a64_ref/ref.s。
  - TR-3.2（pass）：B26 前向 2^27-4（imm 0x1FFFFFF）/精确 2^27 拒绝/真实后向 -4096/非 4 对齐/未绑定；B19 2^20-4（imm19=0x3FFFF）/精确 2^20 拒绝；ADRP 2^20-1 页 immlo/immhi 拆位往返、真实跨 1 页后向 pages=-1、未绑定拒绝（32 位容器内任意两页差 ≤2^20-1，越界 guard 为字段契约保留并注释）。
  - 回归：-Wall -Wextra -Werror 零警告；x86 r42.o/hello4 与 T1 基线 cmp 一致；ctest 单元 22/24（test_a64 34 断言全绿；唯二失败仍是 test_emit/test_link 的 macOS 执行 x86 ELF 环境断言）。

## Task 4: Mach-O PIE 可执行最小链接器（单模块）
- **Status**: `completed`
- **Priority**: high
- **Depends On**: T1
- **Description**:
  - link 层新增 Mach-O 后端：mach_header_64、LC_SEGMENT_64（__PAGEZERO/__TEXT/__LINKEDIT 布局）、LC_BUILD_VERSION、LC_UUID、LC_LOAD_DYLINKER(/usr/lib/dyld)、LC_LOAD_DYLIB(libSystem.B.dylib)、LC_MAIN、PIE（MH_PIE）、__text section。
  - arm64 重定位应用最小集（T14 起做对象格式；T4 手工缓冲直接 patch）。
  - 输出后 ad-hoc 签名：fork/exec `/usr/bin/codesign -s - -f`。
- **Acceptance Criteria Addressed**: FR-3, AC-2, AC-11
- **Test Requirements**:
  - `rule` TR-4.1: 用编码器手工构造的最小程序（write 固定字符串 + exit 42，经内部链接器）生成二进制，`file` 为 Mach-O arm64 PIE、含 LC_MAIN/dyld/libSystem、codesign 校验通过，本机执行输出与退出码正确。
  - `rule` TR-4.2: x86/ELF 链接路径字节与行为不变（证据：test_link 全过 + 产物 cmp）。
- **Completion Evidence**:
  - 实现：[include/fakecc/macho.h](file:///Users/mingming/project/fakecc/include/fakecc/macho.h) + [src/macho.c](file:///Users/mingming/project/fakecc/src/macho.c)。实测对齐 lld64 真实要求（macOS 26.5，本机 hw.pagesize=16384）：CPU_TYPE_ARM64=0x0100000C/cpusubtype=0（0x01000007 是 X86_64，曾误用导致 SIGKILL）；__PAGEZERO(4GiB,vmp 0/0) 必需，缺失 dyld 拒载；段文件偏移/尺寸 16KiB 对齐（4KiB 被 SIGKILL）；LC_UUID 必需（缺失 abort 134）；LC_MAIN（非 UNIXTHREAD）；flags NOUNDEFS|DYLDLINK|TWOLEVEL|PIE；__text 文件偏移 512，命令区预留 ≥16B 空隙供 codesign 注入 LC_CODE_SIGNATURE（否则吃掉代码头 16 字节）；__LINKEDIT 16KiB 空段由 codesign 填充；LC_BUILD_VERSION macOS 11、LC_LOAD_DYLINKER /usr/lib/dyld、LC_LOAD_DYLIB libSystem（零导入合法）；macho_codesign fork/exec /usr/bin/codesign 并 waitpid 传播失败。
  - TR-4.1（pass）：[test/unit/test_macho.c](file:///Users/mingming/project/fakecc/test/unit/test_macho.c)（Apple 平台）用 a64 编码器构造 write(2,msg,19)+exit(42)→macho_write_exec→codesign→fork/exec 本机执行：实测输出 "fakecc macho t4 ok\n"、退出码 42；magic/cputype/filetype/PIE 结构断言 12/12。注：Apple clang 21 对 0xFEEDFACFu 非 volatile 比较常量会错误折叠（实测），测试期望值按字节构造规避。
  - TR-4.2（pass）：x86 r42.o/hello4 与 T1 基线 cmp 一致；ELF 路径未触碰；ctest 单元 23/25（唯二失败仍是 test_emit/test_link 的 macOS 执行 x86 ELF 环境断言）。
  - 里程碑 M1 机械链路（A64→Mach-O→签名→原生执行）端到端打通。
- **Notes**: 段/符号/重定位/动态绑定全集在 T14/T15/T19 扩展。

## Task 5: 第一个端到端垂直切片：常量 main 本机运行
- **Status**: `completed`
- **Priority**: high
- **Depends On**: T3, T4
- **Description**:
  - arm64 函数 prologue/epilogue（stp x29/x30; mov x29,sp; … ldp; ret）、栈 16B 对齐、空帧函数；IR_RETURN 常量、IR_PHI 之外的零操作数路径）；LC_MAIN 入口 stub（暂直接到 main；构造数组调用在 T7）。
  - examples/return42.c 在 macOS 本机：fakecc 编译→arm64 Mach-O→执行退出 42。
- **Acceptance Criteria Addressed**: AC-1, AC-2
- **Test Requirements**:
  - `rule` TR-5.1: return42 与 return0 本机编译运行，退出码精确（证据：命令与 $?）。
  - `rule` TR-5.2: 产物 `otool -l` 结构、codesign、file 三项检查通过（证据：命令输出）。
- **Completion Evidence**（2026-10-02）:
  - 接续时 [src/macho.c](file:///Users/mingming/project/fakecc/src/macho.c) 正处在"单 __text → 完整 EmitModule（__text/__const/__data/__bss）"改造中途、无法编译，本次修复：
    1. [include/fakecc/emit.h](file:///Users/mingming/project/fakecc/include/fakecc/emit.h#L183) 给匿名 struct typedef 加标签 `struct EmitModule`，使 [macho.h](file:///Users/mingming/project/fakecc/include/fakecc/macho.h) 的前向声明与真实类型同一（修掉 macho.c/link.c 的 incompatible pointer types）；
    2. PAGEZERO cmdsize 笔误（`seg_linkedit_cmd`→`seg_plain_cmd`）；`APPEND_ZERO` 移到全部使用点之后 `#undef`，并改为可跨页循环填充；
    3. ncmds 随段数动态化（无 __DATA 时 8、有时 9）；__DATA/__LINKEDIT 布局修正：`data_file`=data 向上页取整、__bss 为其后零填充 VM 尾、`link_off`=data_page+整体页取整（修掉 bss-only 时与 LINKEDIT 的 VM 重叠）；
    4. dyld 实测拒绝空 section（"section '__const' start address 0x0 is before containing segment's address"）：__const/__data/__bss 及整个 __DATA 段均按内容有无条件发射，对应 cmdsize/nsects/ncmds 联动；
    5. [test/unit/test_cg64_native.c](file:///Users/mingming/project/fakecc/test/unit/test_cg64_native.c#L46) 同步新签名（&em）。
  - TR-5.1（pass）：驱动全链路（默认目标 arm64-macos，无需 --target）——`./build/fakecc examples/return42.c -o /tmp/return42` 后执行 `EXIT:42`；新增 [examples/return0.c](file:///Users/mingming/project/fakecc/examples/return0.c) 同样编译执行 `EXIT:0`。附带：`-g` 与 `-O1` 编译 return42 均退出 42（-g 的 DWARF 发射仍为 T20，当前 want_debug 在 codegen64 内忽略）。
  - TR-5.2（pass）：`file` = `Mach-O 64-bit executable arm64`；`codesign -dv` = `Format=Mach-O thin (arm64) … flags=0x2(adhoc)`；`otool -l` 依次为 __PAGEZERO/__TEXT(仅 __text)/__LINKEDIT、LC_BUILD_VERSION、LC_UUID、LC_MAIN(entryoff 1024)、LC_LOAD_DYLINKER(/usr/lib/dyld)、LC_LOAD_DYLIB(libSystem.B.dylib)，codesign 注入 LC_CODE_SIGNATURE。
  - 超额覆盖（T6/T7 地基，正式用例清单签收仍留 T6/T7）：test_cg64_native 16/16 本机真机执行——INT_MIN/-1 除模、无符号除模、变量移位（含 ≥位宽）、ASR/ROL、密集/稀疏 switch、char 符号扩展与窄存储、fib(15) 深递归、12 参（>8 栈传）函数；test_macho 12/12。
  - 回归：ctest 单元 1-26 为 24/26，唯一失败仍是 T1 起记录的同一组 5 个 macOS 执行 x86 ELF 环境断言（test_emit 行 90/106/122、test_link 行 91/129）；-Wall -Wextra -Werror 零警告。x86 产物对 T1 基线逐字节一致（r42.o `cmp` 空=OBJ_IDENTICAL；/tmp/hello4.c 可执行 `cmp` 空=EXE_IDENTICAL）。
  - 里程碑 M1（A64→Mach-O→签名→dyld→原生执行）经**驱动**端到端打通。

## Task 6: IR→A64 整数主体（-O0）
- **Status**: `completed`
- **Priority**: high
- **Depends On**: T5
- **Completion Evidence**（2026-10-02）:
  - **关键 bug 修复（regalloc 双重映射）**：接续 T6 探针时发现 >8 栈参程序在寄存器压力下错误（24/32/64/260 参全部错）。根因——regalloc 的 `ra->reg[]` 存的是**硬件寄存器号**（见 regalloc.c:994 的 `colors[v]=cls->regs[colors[v]]` 与 test_regalloc 的 A64_X19..A64_X28 断言），而 [src/cg64.c](file:///Users/mingming/project/fakecc/src/cg64.c) 的 `home_reg()` 又把它当颜色索引查了一次 `a64_gp_allocatable[]`。低颜色号时双重映射恰好是自置换（程序碰巧正确）；颜色 ≥20（即硬件 x25..x28）时索引越界读到**相邻的 `a64_vec_allocatable` 表**（v0..v3=0..3），不同值被别名到 x0..x4，且 caller/callee-saved 分类错位（颜色 20-23 是 callee-saved，物理落到 x1..x4 却不被 prologue 保存）。修复：`home_reg()` 直接用硬件号；prologue 的 callee-saved 扫描改为硬件区间 `A64_X19..A64_X28`。修复后 n=16/24/32/64/128/200/260 栈参全对；test_cg64_native 新增两个真机回归（many_stack_args 32 参=112、recursion_pressure 7 值跨递归调用=52）。
  - TR-6.1（pass，-O0 对 clang 全量清单）：扫描脚本 /tmp/t6_full.sh（fakecc -O0 vs clang，同源码 sed 去 package/import，比退出码+stdout，8s 超时），覆盖全部 2640 个 clang 可译 e2e 用例。核心整数/控制流目录：basics 13/13、codegen 3/3、control_flow 50/68（18 失败逐一核实全部含全局/static 局部/VLA=T8）、operators 128/164、functions 42/52、types 64/80、pointers 76/108、linkage 4/20。对**全部 1020 个 MISMATCH** 做特性归类（struct/union/数组/字符串字面量/文件作用域变量/static 局部/float/int128/vararg/extern abort），30 个"看似纯整数"的候选逐个核源码，**全部**依赖后续任务特性（T8 全局/聚合、T11 varargs、T12 浮点、T13 int128、T16/T19 extern 符号），无一个纯 T6 漏网。goto_basic/forward/backward、dowhile_*（4）、short-circuit、逗号、三元、char 符号扩展、密集/稀疏 switch 均 PASS。
  - TR-6.2（pass）：test_cg64_native 18/18 真机执行，含 INT_MIN/-1 除模（SDIV 饱和语义对齐 C 可观测结果）、无符号除模、变量移位（≥位宽 A64 硬件掩码）、ASR/ROL、密集+稀疏 switch、sxtb/zext/窄存储、fib(15) 深递归、12 参与 32 参（>8 栈传）、跨调用 callee-saved 压力递归。
  - 已知边界（显式留给后续任务）：默认档 -O1 下 260 参仍错（rc=249≠146），mem2reg 后的 SSA 路径是 T9；switch 跳转表/rodata 数据寻址随 T8；VLA(switch_range_vla) 随 T8 聚合内存。
  - 回归：ctest 单元 24/26（唯二仍是 test_emit/test_link 的 5 个 macOS 执行 x86 ELF 环境断言）；-Wall -Wextra -Werror 零警告；x86 r42.o / hello4 对 T1 基线仍逐字节一致（cg64 改动不触碰 x86 路径）。
- **Description**:
  - 覆盖 codegen.c 的整数主路径：全部二元运算/比较/布尔化、有符号无符号除模、移位（变量移位计数）、局部变量 load/store（栈槽）、if/else、while/for、do、switch（跳转表/比较链）、grep label、short-circuit、逗号表达式、条件表达式、整数提升与截断（w/x 寄存器宽度切换、32 位写零扩展）。
  - 复用现有 cmp+CBR 融合思路在 A64 上的等价实现（cbz/tbz/fcmp）。
- **Acceptance Criteria Addressed**: FR-2, AC-3
- **Test Requirements**:
  - `rule` TR-6.1: test/e2e/cases 中整数/控制流类用例（先逐文件建清单）在 -O0 下 macOS 本机编译运行，输出与 clang 同源码一致、退出码一致（证据：逐用例对照表）。
  - `rule` TR-6.2: 新增 arm64 codegen 单元测试覆盖除模（含 INT_MIN/-1、除零边界用例行为对齐）、移位 ≥位宽、switch 稀疏/密集两类（证据：单测）。

## Task 7: 函数调用 ABI 与进程入口
- **Status**: `completed`
- **Priority**: high
- **Depends On**: T6
- **Completion Evidence**（2026-10-02）:
  - 入口 stub 重写（[src/cg64.c](file:///Users/mingming/project/fakecc/src/cg64.c) codegen64）：48 字节帧（x29/x30 + x19..x22 保存），dyld 给的 x0=argc/x1=argv/x2=envp 先移入 x19/x20/x21 保存 → 按优先级升序 BL 构造函数（稳定插入排序，默认 65535，与 link.c sort_array_slots 一致）→ 恢复 x0/x1/x2 后 BL main → w22 保存 main 返回值 → 析构按优先级**逆序** BL（对齐 ELF _start 的 fini 反向遍历）→ x0=main 值，Darwin exit(x16=1; svc #0x80)。单模块阶段构造/析构用直接 BL（T14/T15 多模块时改为合并指针表）。**踩坑**：stub 里曾用 `a64_mov_reg(x29, sp)`，ORR 编码把寄存器号 31 解释成 XZR（x29 被置 0，后续 [x29+16] 保存直接段错误）；必须用 ADD 形式（mov_sp_like）。
  - `__syscall` 内建最小切片从 T13 前移（emit_syscall）：x16=号、call_args[1..6]→x0..x5（x17 做换环 scratch 的周期安全搬移，号最后入 x16）、svc #0x80、x0 返回。**踩坑 2**：__syscall 分流必须在 emit_call 通用参数搬移**之前**——否则通用块先把参数写进 x0..x7，覆盖 syscall 降级要重读的源 home（实测 write 的 buf 值 home=x3，被写成常数 3 后 x1=x3 传了错指针，稳定返回 EFAULT 14）。errno/carry 约定留 T13。
  - TR-7.1（pass，全部本机真机）：test_cg64_native 29/29，新增——argc/argv 经 stub 传参（带 2 个自定义参数 execv 校验）、构造函数优先级顺序（100 先于 200，用 exit(11)/exit(21) 观测）、默认优先级 ctor、析构逆序且在 main 之后（main 返回 9 不可见，dtor(200) exit 32）、无 ctor 时 main 正常（42）、signed/unsigned/混合窄类型 12 参栈传（250/78/230）、函数指针表+间接调用（T6 已有）、10000 层深递归。-O1 档下 ctor 顺序与 argc/argv 同样正确（mem2reg 路径整体仍归 T9）。
  - TR-7.2（pass，超出要求）：main(int,char**,char**) 遍历 envp 找到 PATH=，用原始 write(4) 系统调用把值写到 stdout，测试进程通过管道捕获并与 `getenv("PATH")` **逐字节相等**（非空），不依赖任何 libc/runtime。
  - BL 26 位越界：单模块单 TU 远小于 ±128MB；超出时 a64_resolve 已有明确诊断（"fakecc: a64 b/bl out of range" → codegen64 die，绝不出错码）。自动 veneer/trampoline 推迟到 T14（多模块链接会重新设计跨对象调用）。IR 无独立尾调用算子（尾调用即普通 call），深递归行为与 x86 一致（10000 层实测）。
  - 回归：ctest 单元 24/26（唯二仍是 test_emit/test_link 的 5 个 macOS 执行 x86 ELF 环境断言）；零编译警告；x86 r42.o/hello4 对 T1 基线仍逐字节一致（仅动 arm64 路径）。
- **Description**:
  - bl 调用 + 26 位越界长尾（veneer/trampoline 或寄存器间接）；x0-x7 参数编排、第 9+ 参数栈传递（16B 对齐槽）、callee-saved（x19-x28、v8-v15 低部）在被使用函数 prologue 保存；调用点 SP 对齐保证。
  - LC_MAIN 入口 stub：x0=argc/x1=argv/x2=envp 入栈保存，调用 init_array 构造函数（PIE 下指针重定位），构造数组遍历，call main，main 返回后 atexit 注册路径（fini 经 atexit 式注册；析构数组逆序）；Darwin exit(返回值)（无 exit_group，class 0x2000000 无，exit=1）。
- **Acceptance Criteria Addressed**: FR-2, FR-5, AC-2, AC-3
- **Test Requirements**:
  - `rule` TR-7.1: 多参数（含 >8 参、混合宽度）、深递归、函数指针、构造/析构数组用例本机通过；main 可读取 argc/argv（证据：运行对照 clang）。
  - `rule` TR-7.2: 入口环境：`envp` 可经平台层读取到至少一个已知变量（证据：打印 PATH 的测试程序输出非空）。

## Task 8: 全局数据、字符串、聚合内存与 PIE 寻址
- **Status**: `completed`
- **Priority**: high
- **Depends On**: T7
- **Completion Evidence**（2026-10-03）:
  - 全局/静态/字符串/只读常量进 `__TEXT,__const`，已初始化可变对象进 `__DATA,__data`，零初始化进 `__DATA,__bss`。`IR_GADDR` 用 ADRP+ADD，在最终 text 长度确定后按镜像内偏移回填（与 `macho_section_offsets` / `macho_write_exec` 同一布局）。
  - 含指针的初值（含本应只读的函数指针表、`char *s = "..."`、`&arr[n]`）放在可写 `__DATA`：dyld 不能改 `__TEXT`。槽位编码为 `DYLD_CHAINED_PTR_64_OFFSET`（format 6，target 为 mach header 相对偏移），并写入 `LC_DYLD_CHAINED_FIXUPS` + 空 `LC_DYLD_EXPORTS_TRIE`。blob 与同形态 ld64 产物逐字节一致。`dyld_info -fixup_chains` 可见链；`dyld_info -fixups` 会在符号化时因尚无 nlist（T14）崩溃，故测试用 `-fixup_chains`。
  - 大于 64 字节的结构体赋值在 IR 里是 `memcpy`。运行时库要到 T16 才链接，因此本模块未定义 `memcpy`/`memmove`/`memset` 时由 cg64 内联字节循环（用户若自己定义同名函数则仍走 BL）。
  - TLS（`IR_GADDR_TLS`）与外部全局仍明确报错，且 `lower_one_tu` 在 codegen `die_at` 之后不再写出二进制。
  - TR-8.1：`test_cg64_native`（-O0）44/44（含全局/bss/常量、字符串、static local、结构体拷贝、二维数组、位域、union、指针比较、80 字节 memcpy、`__builtin_memset`、函数指针表+指针初值）。另用驱动默认 -O1 扫了 `test/e2e/cases/{aggregates,pointers,chars_strings}` 里无 `import` 的用例：227 通过 / 33 失败。失败集中在后续任务：结构体按值传参与返回（T10）、向量（T12）、浮点 union（T12）、`__thread`（T17）、varargs（T11），以及一条与全局无关的 64 位除法边界（`divconst-2`，改成局部数组同样失败）。
  - TR-8.2：`int *p = &g` 连续两次执行均返回 7（ASLR 下地址不同、结果相同）；`dyld_info -fixup_chains` 报告 `pointer_format 6` 与 `start[0]`。
  - 回归：`test_macho` 12/12。无指针初值的镜像不增加 load command。
- **Description**:
  - 全局/静态变量、字符串字面量、const 数据、零填充公共符号（__common/__bss）；ADRP+ADD（页地址）、ADRP+LDR（GOT 风格绝对槽，PIE）、PAGEOFF12 数据读写；GEP/数组/多维数组/结构体/位域/union/offsetof；memcpy/memset 结构体块拷贝；取地址与指针比较。
  - 数据段内部指针 fixup 的 arm64 重定位（指向内部函数/数据的指针表、构造数组、跳转表）。
- **Acceptance Criteria Addressed**: FR-2, FR-3, AC-3
- **Test Requirements**:
  - `rule` TR-8.1: gcc-torture 式聚合/指针/全局类用例在 macOS -O0 下与 clang 输出一致（证据：用例清单与对照结果）。
  - `rule` TR-8.2: 含函数指针表/全局指针初始化/跳转表的程序在随机加载基址下正确（PIE 证据：多次运行 + `dyld_info` 看 fixup）。

## Task 9: -O1 优化档在 arm64 后端生效
- **Status**: `completed`
- **Priority**: high
- **Depends On**: T8
- **Completion Evidence**（2026-10-03）:
  - mem2reg 之后大量值溢出。槽位低于 `fp-256` 时，第二条操作数的地址被算进 x16，冲掉已经装进 x16 的第一条操作数；求和再 `% 256` 后退出码随栈地址变化。修复：大偏移先在目标寄存器里形成地址，再 `ldr rt, [rt]`，不再占用另一只 scratch。32 参 `many_stack_args` 在 -O1 得到 112；`call_many_args`（260 参）-O0/-O1 都得到 146。
  - 除法和旋转的商/取负临时量固定用 x17。两个操作数都溢出时 x17 里已经是除数或被旋转值，结果被覆盖。临时量改为避开操作数（必要时用不可分配的 x0）。帧大于 4095 时 `sub sp` 按 4080 字节分段，因为 imm12 到 4095 为止。
  - 死参数仍会分到一个寄存器，并且可以和活参数是同一个。序言把死参数拷进这个寄存器，活参数丢失。若这个寄存器又是另一条入参的源，周期消解会从尾巴走进去，找不到闭环后空转（`die_at` 只打印一次）。未使用的参数不再参与序言搬移；消解失败时停止而不是空转。`gcc_torture_divconst_2`（未使用的 `denom` 覆盖了商）和 `gcc_torture_20030307_1`（未使用的 `fsp` 卡死编译）在 -O1 都返回 0。
  - TR-9.1：`test_cg64_native` -O0 与 `CG64_OPT=1` 均为 44/44。T6/T8 目录（basics、codegen、control_flow、operators、functions、types、pointers、linkage、aggregates、chars_strings）双档对照：与 -O0 一样通过的用例在 -O1 仍通过。原先仅 -O1 失败的两条已复测通过。两边都失败的仍是后续任务（结构体按值、浮点、向量、varargs、extern）。
  - TR-9.2：`int hot(int n)` 的 `while (i<n) s+=i` 在 -O0 为 44 条指令（循环里反复 `ldrsw`/`str`），-O1 为 31 条，累加器和归纳变量留在寄存器里。
- **Description**:
  - SSA mem2reg 后的寄存器化代码路径（值常住寄存器、spill/reload、location 变动）；常量折叠/强度削减后的 arm64 选择（立即数形式、cbz/tbz、移位合并）；确保 T6/T8 全部用例在 -O1 同样正确。
- **Acceptance Criteria Addressed**: FR-2, AC-3
- **Test Requirements**:
  - `rule` TR-9.1: T6/T8 用例集 -O1 全通过且与 clang 行为一致（证据：双档对照表）。
  - `rule` TR-9.2: 至少一个热点函数 -O1 指令数显著少于 -O0（证据：反汇编计数，防"优化空转"）。

## Task 10: 聚合类型 ABI（结构体参数/返回、HFA/间接返回）
- **Status**: `completed`
- **Priority**: high
- **Depends On**: T9
- **Description**:
  - AAPCS64 参数分类：HFA/HVA 经 V 寄存器、≤16B 整数聚合拆寄存器、>16B 或不符合拆分规则者经 x8 间接返回/栈拷贝；调用方与被调用方对称；结构体按值嵌套、混合 int/fp、数组包装。
- **Acceptance Criteria Addressed**: FR-2, AC-3
- **Test Requirements**:
  - `rule` TR-10.1: 结构体 ABI 专项用例（尺寸 1/2/3/7/8/9/16/17/32B、HFA 2×double/4×float、嵌套、返回大结构体）与 clang 交叉互调结果一致（证据：专项测试，含 fakecc↔clang 双向调用结构体）。
- **Completion Evidence**:
  - 实现：`TargetDesc` 增加 `gp_arg_regs`（x86 6 / arm64 8）与 `sret_uses_gp`（SysV 占用 rdi，Darwin 的间接返回指针在 x8、不占 x0）。`ast.c` 的 `abi_indirect_agg` / `abi_is_hfa` 只在 arm64 上生效：非 HFA 且大于 16 字节走“指针指向调用方副本”；1–4 个相同 float/double 的聚合是 HFA。`ir.c` 用这些函数改分类，x86 的 SysV 路径（6 个 GP、sret 占 rdi、MEMORY 栈拷贝）不变；`test_ir` 显式 pin `x86_64-linux`。HFA 的 SSA 仍是整数位型，调用边界用 `FMOV` 进出 v 寄存器（`CALL_ARG_HFA`、`A64_MARK_HFA`）；大于 16 字节的返回把 `call_args[0]` 标成 `A64_MARK_SRET`，cg64 放进 x8。16 字节整数返回同时写 x0 和 x1。混合 `{int, double}` 两个八字节都走 GP（与 clang 一致，不是 HFA）。
  - TR-10.1（pass）：`test_cg64_native` 在 -O0 与 `CG64_OPT=1` 均为 82/82。尺寸 1/2/3/7/8/9/16/17/32、嵌套、`many(int, struct s16, int)`、第 8 个参数仍是寄存器的 `struct s8`、HFA 2×double / 4×float、混合 int/double 的退出码与手算一致。交叉链接（可重定位对象要等 T14，测试把函数体抽出来交给 clang 链接）：clang 调用 fakecc 的 s32 返回 11、HFA 2×double 返回 1；fakecc 调用 clang 的同一对函数分别返回 11 和 1；clang 调用 fakecc 的 `many` 返回 34。另用脚本确认 clang→fakecc 的 4×float 返回 10、17 字节结构返回 5。
  - 限制：3–4 个 double 的 HFA 超过两个八字节，当前 IR 的 `dst`/`b` 放不下，编译期报错，不在本任务的 2×double / 4×float 范围内。标量浮点运算仍是 T12。

## Task 11: 变长参数（varargs）
- **Status**: `completed`
- **Priority**: high
- **Depends On**: T10
- **Description**:
  - Darwin AAPCS64 varargs 约定：register save area（GPR x1-x7 中 8 个槽位与 SIMD v0-v7 8 个槽位的位图 grsave/vrisave）、va_start/va_arg、寄存器→栈溢出区、按尺寸/对齐提升；printf/scanf 家族在 runtime 下全形态（%d/%s/%x/%f/%g/宽度精度/n 等）。
- **Acceptance Criteria Addressed**: FR-2, FR-5, AC-3
- **Test Requirements**:
  - `rule` TR-11.1: 现有 printf/scanf/varargs 用例 macOS 双档通过，输出与 clang 一致（证据：difftest 子集）。
  - `rule` TR-11.2: 混合 int/pointer/double 的多参数（>8 参）varargs 专项用例通过（证据：新增专项）。
- **Completion Evidence**（2026-10-03）:
  - 本机 clang（-O0 与 -O2）的约定和说明书里的寄存器保存区不同：有名参数仍走 x0–x7，无名参数全部落在栈上的 8 字节槽里。`va_list` 是 8 字节游标（`__va_list_tag` 只有一个指针成员），`va_start` 写入 `fp + save_total + 8 * 有名栈槽数`，`va_arg` 读当前槽再加 8。≤16 字节的聚合按八字节摊开，更大的聚合是指向调用方副本的一个指针槽。常量 double 以整数位型写入槽（`1.0` 为 `0x3ff0000000000000`）。x86 的 24 字节 `va_list` 与 `codegen.c` 的保存区路径没有改。
  - sema 把没有原型的内建（`__builtin_memset` 等）标成 0 个参数的变参函数，只为通过实参数量检查。这些调用仍按普通寄存器传参，内联的 `memset` 才能从 x0–x2 取到参数。
  - TR-11.2（pass）：`test_cg64_native` 在 -O0 与 `CG64_OPT=1` 均为 103/103。覆盖单个 int、负 int 的符号扩展、10 个栈上 int（和为 55）、8 个有名参数之后的变参（1+9+10=20）、`va_copy`、变参 `struct s16` / `struct s32`、int/指针/`1.0` 位型混合。交叉链接：clang 调用 fakecc 的 `sum3(3,10,20,30)` 返回 60，fakecc 的 `check` 调用 clang 的 `sum3` 也返回 60。
  - 整数 e2e：`variadic_sum`、`variadic_single`、`variadic_vla_before_vastart`、`variadic_fmt_live_loop` 在 -O0 与 -O1 的退出码分别为 60、5、60、6。
  - 限制：`runtime/printf.c` 的 `%f` 分支含浮点运算，整份翻译单元要等 T12 才能在 arm64 上编译，所以 TR-11.1 的 printf/scanf 全形态未在本任务跑通。非常量的变参 double（需要浮点算术）同样留给 T12。

## Task 12: 浮点、标量转换与 NEON 向量 ABI
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T11
- **Description**:
  - f32/f64 算术/比较/布尔化、int↔float 转换（有符号/无符号）、fcmp+b.cond、long double（按现 x86 后端 dfp/soft-fp 策略移植）；常量池与 rodata 编码。
  - NEON 128 位向量类型：向量传参/返回（v 寄存器 HFA）、向量 load/store 对齐、向量按元素算术（对标现有 -mavx 32B 协商：arm64 固定 16B）；x86 向量 ABI 开关在 arm64 目标下的行为按 T1 决策锁定。
- **Acceptance Criteria Addressed**: FR-2, AC-3
- **Test Requirements**:
  - `rule` TR-12.1: 浮点/转换用例（含 NaN/Inf/舍入、unsigned↔double）与向量用例 macOS 双档对 clang 一致（证据：对照结果）。
  - `rule` TR-12.2: -mavx/-mavx512f 在 arm64 目标行为有锁定测试（报错或协商，按 T1 决策）（证据：CLI 测试）。

## Task 13: 特殊内建与 __int128、平台内建
- **Status**: `pending`
- **Priority**: medium
- **Depends On**: T12
- **Description**:
  - __int128 算术/除模（对标 x86 后端，A64 可用 ADDS/ADC 或库调用）、builtin 溢出检查/expect/frame_address 等现有支持集；`__syscall` 内建 arm64 降低（svc #0x80，x16 号 + x0-x5 参数 + x0 返回，clobber x9 条件码约定）；`__clone` 在 arm64 不直接对应，由 runtime darwin 线程实现（T17），前端暴露面保持。
- **Acceptance Criteria Addressed**: FR-2, AC-3
- **Test Requirements**:
  - `rule` TR-13.1: __int128 与各 builtin 现有用例 macOS 通过（证据：用例结果）。
  - `rule` TR-13.2: 直接 __syscall 调用的最小程序（write/exit/mmap）正确（证据：专项）。

## Task 14: Mach-O 可重定位对象（-c）读写与多模块
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T9
- **Description**:
  - EmitModule 泛化重定位类型族；arm64 .o：nlist_64 符号表、ARM64_RELOC_*（UNSIGNED/BRANCH26/GOT_LOAD/LOAD/PAGE21/PAGEOFF12/ADDEND/TLVP 等）、section 对齐、subsections-via-symbols 所需属性（链接正确性需要）；emit_obj/emit_obj_read 的 Mach-O 实现。
  - 多模块合并链接（包系统、extern/static、弱符号、common 合并、重复定义诊断）。
- **Acceptance Criteria Addressed**: FR-3, AC-3, AC-8
- **Test Requirements**:
  - `rule` TR-14.1: 现有对象往返/多模块单测在 macOS arm64 通过；fakecc -c 产出可被 otool 反汇编且可回读再链接（证据：单测 + 往返测试）。
  - `rule` TR-14.2: ELF 对象路径字节不变（证据：Linux 侧 test_obj_roundtrip/产物 cmp）。

## Task 15: Mach-O 链接器完整版（段布局、init/fini、对齐、诊断）
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T14, T10
- **Description**:
  - 全段布局与对齐（__TEXT/__DATA_CONST/__DATA/__LINKEDIT/__DWARF 占位）、zerofill、section 对齐上取整；init_array→__DATA __init_array（Darwin 下为普通指针表，入口 stub 遍历；无 .init_array 原生语义时由 stub 实现等价）；fini 经 atexit 式注册/退出路径；未定义符号诊断、no-main 诊断、-nodefaultlibs/-nostdlib 在 macOS 的语义。
  - （静态产物内部符号全部消解；外部库绑定在 T19。）
- **Acceptance Criteria Addressed**: FR-3, AC-3
- **Test Requirements**:
  - `rule` TR-15.1: test_link 全部场景（含 no-main 报错、static 不冲突、构造数组执行顺序）macOS 通过（证据：单测）。
  - `rule` TR-15.2: 随机/极端对齐（64B 全局、大 zero-fill、跨页 rodata）程序正确（证据：专项运行）。

## Task 16: Darwin runtime 平台层（I/O、内存、环境变量、errno）
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T13, T15
- **Description**:
  - 新建平台头（按 target 注入）：syscall 号 class 0x2000000 映射表、mmap/open/lseek 等 flags/权限常量、errno 表归一；runtime 代码只引用平台常量。
  - stdio.c/stdlib.c/printf.c/malloc.c/string.c 等 Darwin 适配：open/write/read/close/lseek/mmap/munmap/unlink/chmod/getpid 换号；MAP_ANON=0x1000、O_CREAT=0x200 等；环境变量改从入口 envp（T7 保存）构建，删除 /proc/self/environ 依赖；exit 路径（exit=1，无 exit_group）。
- **Acceptance Criteria Addressed**: FR-5, AC-2, AC-3
- **Test Requirements**:
  - `rule` TR-16.1: hello/文件读写（fopen/fwrite/fseek/ftell）/malloc 压力（超单页 munmap 裁剪路径）/getenv-setenv/exit code 用例 macOS 双档对 clang 一致（证据：difftest 子集）。
  - `rule` TR-16.2: runtime 源码中无裸 Linux 数字残留（平台常量外）（证据：grep + 审查）。

## Task 17: Darwin 线程运行时（bsdthread + ulock + TPIDR_EL0 TLS）
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T16
- **Description**:
  - thread.c Darwin 双轨：mmap 栈/TLS 块（沿用对齐分配），经 bsdthread_create(360) 建线程（参数：函数/参数/栈/栈大小/udata→TLS 基址/flags）、bsdthread_terminate(362) 退出；futex→__ulock_wait(515)/__ulock_wake(516)（UL_COMPARE_AND_WAIT/WAKE_ALL 语义）。
  - TLS：arm64 Darwin 用 mrs tpidr_el0；主线程 TCB 由内核/平台给（TPIDR 已指向系统 pthread 区——自管 TLS 需在 udata 布局中自管且不踩系统区；实现期按实测确定：自建 TPB 并把 TID 字段放在固定偏移）；`__thread` 变量访问（IE 模型，TPIDR+offset），与 dylib TLS（T19）兼容。
- **Acceptance Criteria Addressed**: FR-5, AC-6
- **Test Requirements**:
  - `rule` TR-17.1: 多线程 create/join/self/exit、每线程 __thread 隔离、join 返回值、并发计数（ulock 唤醒不丢）专项用例 macOS -O0/-O1 连续 50 次运行零失败（证据：50 次运行记录）。
  - `rule` TR-17.2: e2e 多线程相关用例通过（证据：ctest 相关项）。

## Task 18: sanitizer 与其余 runtime Darwin 细节
- **Status**: `pending`
- **Priority**: medium
- **Depends On**: T17
- **Description**:
  - sanitizer 影子基址策略在 Darwin PIE/ASLR 实测（0x100000000000 hint 回退链）；int128/decimal/ctype 等纯逻辑模块的平台兼容核查；getdirentries64(344) 供自举期目录枚举（T24 也需要）。
- **Acceptance Criteria Addressed**: FR-5, AC-3
- **Test Requirements**:
  - `rule` TR-18.1: ASAN 影子用例（越界/use-after-free 检测、报告写出到 stderr、退出码）macOS 通过（证据：sanitizer 用例结果）。
  - `rule` TR-18.2: 纯逻辑 runtime（string/stdlib 转换等）difftest 全通过（证据：difftest）。

## Task 19: dylib 动态链接后端（-shared / -l / -L）
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T16
- **Description**:
  - MH_DYLIB 输出：LC_ID_DYLIB、导出 trie（exports trie）、未定义符号→bind（经典 opcode bind 或 chained fixups，二选一并注释依据）、GOT/la_symbol stub 岛与 veneer；-fPIC 语义（arm64 代码本就 PIE 风格，校验跨 dylib 调用经 GOT/stub）。
  - 可执行文件引用 dylib：LC_LOAD_DYLIB（-l 名→libX.dylib 解析）、LC_RPATH（-L/可执行目录）、-l: 精确名、weak 引用语义、-nostdlib/-nodefaultlibs；平台强制的 libSystem 始终保留。
  - 双向互操作：clang 主程序链接 fakecc dylib；fakecc 主程序调 clang dylib 与 libSystem（printf）。
- **Acceptance Criteria Addressed**: FR-4, AC-5
- **Test Requirements**:
  - `rule` TR-19.1: run_shlib_e2e macOS 路径全场景通过（构建/运行/标签检查：otool/dyld_info 验证 LC_LOAD_DYLIB/LC_RPATH/LC_ID_DYLIB/导出符号）（证据：测试输出）。
  - `rule` TR-19.2: 三个互操作程序（AC-5 a/b/c）运行正确（证据：运行记录）。
  - `rule` TR-19.3: 弱符号未定义场景与 Linux 现有宽容语义一致（证据：对应用例）。

## Task 20: -g 调试信息（arm64 DWARF / Mach-O）
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T15, T18
- **Description**:
  - debug.c 目标分支：DWARF 寄存器号表 arm64（x0-x28=0-28、FP=29、LR=30、SP=31；v0-v31=64-95）、CFI 初值/帧描述（x29/x30 保存、SP 规则）；Mach-O `__DWARF` segment（__debug_info/line/abbrev/str/loc/ranges/frame）与调试映射（section 地址→链接后 vaddr 正确）；location list（-O1）。
  - 保持"-g 纯增量、__TEXT 字节不变"纪律（arm64 同测）。
- **Acceptance Criteria Addressed**: FR-6, AC-4
- **Test Requirements**:
  - `rule` TR-20.1: -g 与无 -g 的 __text 字节一致（证据：cmp，两档）。
  - `rule` TR-20.2: lldb image dump 能解析编译单元/函数/行表/变量（证据：lldb 脚本输出）。

## Task 21: 端到端测试脚本平台化（非 debug/shlib 部分）
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T18
- **Description**:
  - run_e2e/run_multi_e2e/run_difftest/run_cli_e2e/app_ports/compile 测试平台化：oracle=clang（arm64）、产物类型/导出符号检查分支（ELF readelf/nm ↔ Mach-O otool/nm/dyld_info）、文件后缀、超时；difftest 比对 fakecc vs clang 同源码 stdout/exit；建立并冻结"平台不适用"显式 skip 清单（每项写明原因，预期仅 ifunc 等 Linux 专有）。
  - CMakeLists 测试注册在 macOS 下指向平台脚本，Linux 路径不变。
- **Acceptance Criteria Addressed**: FR-8, AC-3
- **Test Requirements**:
  - `rule` TR-21.1: 上述各脚本在 macOS 单独运行通过（debug/shlib 除外，其在 T19/T22 收口）（证据：脚本输出）。
  - `rule` TR-21.2: skip 清单成文且每条可复现实证（证据：skip 清单文件/注释 + 复核记录）。

## Task 22: lldb 调试端到端（e2e_gdb 平台驱动）
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T20, T21
- **Description**:
  - run_gdb_e2e.sh 增加 Darwin/lldb 驱动：把用例注解（break/expect/gdb_expect/gdb_reject）映射为 lldb batch 命令（breakpoint set -f -l、frame variable、p、bt），保持 -O0/-O1 两档与 -g 字节一致性检查；Linux 继续 gdb。
  - 按 lldb 表达能力校准既有断言（变量打印语法差异），不降低检查意图。
- **Acceptance Criteria Addressed**: FR-8, AC-4
- **Test Requirements**:
  - `rule` TR-22.1: cases/debug 全部用例 lldb 路径两档通过，reject 断言成立（证据：测试输出）。
  - `rule` TR-22.2: Linux gdb 路径行为不变（证据：VM 侧运行）。

## Task 23: ctest 36/36 macOS arm64 全绿收口
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T19, T21, T22
- **Description**:
  - 全新构建目录跑 ctest 1-36（-O0/-O1 全套）；清零全部失败；收口 skip 清单；核对 AC-9 分层质量（平台分支 grep、中立文件无 diff）。
- **Acceptance Criteria Addressed**: AC-3, AC-9, AC-11
- **Test Requirements**:
  - `rule` TR-23.1: `ctest` 汇总 100%（36/36 退出 0；skip 仅限冻结清单）（证据：完整 ctest 日志）。
  - `rubric` TR-23.2: 后端分层质量；scale 1-5；anchors 同 AC-9；threshold >=4；证据：中立层 diff、平台分支统计、目标接口审查。
  - `rubric` TR-23.3: 可维护性/验证可重复性；scale 1-5；anchors 同 AC-11；threshold >=4；证据：无告警构建、新增测试清单、复现命令。

## Task 24: macOS 自举固定点
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T23
- **Description**:
  - v0/translate.py 与 FAKECC_SELFHOST 路径 Darwin 适配：src/pkg.c 自举分支 syscall 双轨（open/close/getdirentries64 结构解析，d_reclen/name 布局）；自举源码中其余 raw syscall（暂无则免）平台化；stage2_check.sh 平台化（产物名、签名步骤：fakecc-1/2 生成后各做 ad-hoc 签名、file 校验）。
  - Stage0(clang 构建的 host fakecc)→fakecc-1→fakecc-2 字节一致。
- **Acceptance Criteria Addressed**: FR-7, AC-7
- **Test Requirements**:
  - `rule` TR-24.1: macOS 执行平台化 stage2 输出 FIXED POINT REACHED，cmp 为空（证据：脚本输出）。
  - `rule` TR-24.2: fakecc-1/fakecc-2 为已签名 arm64 Mach-O 且可运行（编译一个样例成功）（证据：file/codesign/运行）。
  - `rule` TR-24.3: Linux stage2 路径不回归（证据：VM 侧 stage2 通过）。

## Task 25: linux-x86_64 零回归验证
- **Status**: `pending`
- **Priority**: high
- **Depends On**: T24
- **Description**:
  - Alpine VM 全新构建 + ctest 全套（JOBS=2，参照既有 2h45m 预算）；与记忆基线（28/36 + 8 个环境性差异逐项同名）比对；关键对象/二进制与改动前字节比对（允许的差异仅限注释/调试行等可解释项）。
- **Acceptance Criteria Addressed**: FR-9, AC-8
- **Test Requirements**:
  - `rule` TR-25.1: unit 22/22；端到端失败集合与基线逐项一致，无新增（证据：基线对比表）。
  - `rule` TR-25.2: x86 关键产物字节无意外差异（证据：cmp 记录与差异解释清单）。

## Task 26: arm64 -O0 性能基线取证
- **Status**: `pending`
- **Priority**: medium
- **Depends On**: T23
- **Description**:
  - 确定 bench 套件 macOS 子集与计时方法（本机直接计时、多轮中位数），对比 fakecc -O0 与 clang -O0；记录几何总均值与紧凑循环/调用密集/除法/递归各形态；必要时为 arm64 后端补低成本优化（不改变正确性结构）。
- **Acceptance Criteria Addressed**: NFR-2, AC-10
- **Test Requirements**:
  - `rule` TR-26.1:  bench 脚本在 macOS 一键可跑并产出对照表（证据：脚本与数据表）。
  - `rubric` TR-26.2: 几何平均时间比；scale 1-5；anchors 同 AC-10；threshold >=3（≤2.0×）；证据：分用例数据与几何均值。

## 依赖与里程碑

- **M1 本机 hello（T1-T5）**：arm64 Mach-O return42 在 Mac 直接跑通。
- **M2 语言主线（T6-T15）**：全部语言特性 + 完整链接器，多数单模块用例本机可跑。
- **M3 平台能力（T16-T19）**：runtime/线程/dylib 完整。
- **M4 测试与调试（T20-T23）**：ctest 36/36。
- **M5 自举与回归（T24-T26）**：固定点 + Linux 零回归 + 性能基线，进入独立 Review。

## AC 覆盖矩阵

| AC | 覆盖任务 |
|---|---|
| AC-1 | T1, T4, T5 |
| AC-2 | T4, T5, T7, T16 |
| AC-3 | T1, T6-T15, T16-T18, T21-T23 |
| AC-4 | T20, T22 |
| AC-5 | T19 |
| AC-6 | T17 |
| AC-7 | T24 |
| AC-8 | T1, T14, T25 |
| AC-9 | T1, T2, T23 |
| AC-10 | T26 |
| AC-11 | T3, T4, T23 |
