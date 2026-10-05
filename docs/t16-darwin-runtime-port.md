# T16：arm64 的 runtime 移植方案

## 目标
让 `runtime/`（4599 行，11 个文件）在 Darwin arm64 上可用，从而给 arm64
用户程序提供 `printf` / `fopen` / `strtod` / `sscanf` 等 libc 服务。

**这是 arm64 剩余 e2e 失败的最大来源**：224 个失败里约 120 个是
`undefined function 'printf' / 'sprintf' / 'fopen' / ...`。

## 当前阻塞点：syscall 号码是 Linux 的
`runtime/*.c` 用 `__syscall(N, ...)` 直接发 Linux x86-64 号码，编译器原样透传。
Darwin arm64 的号码完全不同，于是**静默执行了错误的系统调用**：

| runtime 写的 N | Linux 含义 | Darwin arm64 实际含义 | 后果 |
|---|---|---|---|
| 1 | write | **exit** | printf 输出时直接退出进程 |
| 2 | open | 无效 | 失败 |
| 3 | close | 无效 | 失败 |
| 60 | exit | 无效 | 失败 |
| 9 | mmap | 无效 | malloc 全面失败 |
| 11 | munmap | 无效 | free 失败 |

实测证据：`printf("hello %d\n", 42)` 产生 rc=138，而 `exit(138)` 也正好是
rc=138 —— 确认 `__syscall(1)` 执行的是 Darwin 的 exit。

⚠️ **这类错误不会编译失败，也不会明显崩溃，而是静默产生错误行为。**
不要通过"解除 runtime 门控"来试���，那样会让所有程序输出错误结果。

## 号码对照表（Darwin arm64）
已确认的映射（Darwin arm64 syscall 编号）：

| syscall | Darwin | 备注 |
|---|---|---|
| read | 3 | cg64 已有 builtin |
| write | 4 | cg64 已有 builtin |
| open | 5 | cg64 已有 builtin |
| close | 6 | cg64 已有 builtin |
| exit | 1 | cg64 emit_exit_builtin 已实现 |
| getpid | 20 | |
| mmap | 197 | |
| munmap | 73 | |
| lseek | 199 | |
| unlink | 10 | runtime 87 |
| dup2 | 90 | 号码巧合相同 |
| gettid | 224 | |
| futex | — | **无等价**，需换实现 |
| clone | — | **无等价**，需换实现 |

## 需要改动的文件与工作量
| 文件 | syscall 用途 | 难度 |
|---|---|---|
| `stdio.c` | 0/1/2/3/8/87 | 低 —— 可改用 cg64 已有 builtin |
| `stdlib.c` | 0/2/3/90/231 | 低 |
| `malloc.c` | 9 (mmap) | 中 —— Darwin mmap 签名不同 |
| `sanitizer.c` | 1/9/231 | 中 |
| `thread.c` | 9/11/60/202 + clone | **高 —— 需重写线程层** |

### 推荐路径：优先让 stdio/stdlib 走具名函数
cg64 已经实现了 Darwin 版的 `read`/`write`/`open`/`close` builtin。
若 `stdio.c` 改为调用具名函数而非 `__syscall(1,...)`：
- arm64 自动走 cg64 的正确 Darwin builtin
- x86 侧没有这些 builtin，需要**同时**为 x86 提供，
  或保留 `__syscall` 写法并新增一份 `stdio_darwin.c`

fakecc 的模块系统没有条件编译（无 `#ifdef`），所以只能靠**按目标选文件**。
这与 runtime 现有的单文件结构冲突，是 T16 的主要设计工作量。

### thread.c 需要整体重写
Linux 的 `clone()` + `futex()` + `%fs:0` TLS 在 Darwin 都不存在。
Darwin 的路径是 `pthread_create`（libSystem）或 `bsdthread_create`（syscall 360）。
在没有 libSystem 的 freestanding 前提下，需要自己管理栈、TLS 和同步。
建议：在 T16 之前，arm64 上把 thread.c 整体排除出 runtime 包，
并让 `pthread_create` 返回 ENOSYS —— 线程用例失败，但其他 runtime 功能可用。

## 建议的推进顺序
1. **先做 syscall 号码转译层**：在 cg64 的 `emit_syscall` 里按目标把 Linux
   号码映射到 Darwin 号码。改动小（一个函数），收益最大（stdio/stdlib/malloc
   全部可用），且不触碰 runtime 源码。⚠️ 但要排除 futex/clone 这类无映射的。
2. thread.c 排除出 arm64 runtime 包，`pthread_create` 返回错误。
3. 之后再考虑 thread.c 的 Darwin 重写。

## 实测数据（2026-10-05）
解除 runtime 门控后的 e2e 演进（-O1，全量 2685）：

| 状态 | 通过 | 说明 |
|---|---|---|
| 门控关闭（当前 HEAD） | **2461** | runtime 未链接，printf 等不可用 |
| 门控开启（仅号码转译） | 1947 | 大量程序被全局指针 bug 拖垮 |
| 门控开启 + 指针全局修复 | 2366 | 仍低于基线 95 |

**结论：runtime 门控暂不能开。** 修复了多 TU 布局烘焙（`f10186a3`）后从
1947 回升到 2366，但仍差 95 —— runtime 内部还有未查清的问题（printf 仍跳到
垃圾地址 `0x3e02a1303f7`）。

⚠️ 不要再单独解除那个门控来"试试看"，它每次都要花 ~25 分钟 e2e 才能评估。

## 已查明并修复的前置 bug
1. **syscall 号码**（`109a7a53`）—— 已在 `emit_syscall` 里加 Linux→Darwin 映射。
2. **多 TU 布局烘焙**（`f10186a3`）—— 直接多 TU 链接时，cg64 把单模块的段偏移
   烘焙进 dyld rebase；链接器拼接段后偏移失效，导致 `stdout = &_rt_stdout`
   这类指针全局指向错误位置。修法与 `-c` 一致：走重定位路径。
3. **`__clone` 弱解析**（`4a973536`）—— runtime/thread.c 用 Linux clone。

## 下一个要查的
printf 崩溃点 `0x3e02a1303f7`（垃圾地址）说明还有一个指针/地址错误未修。
建议下一步：在多 TU 下打印各模块的段布局与符号地址，与链接器合并后的
`text_base/ro_base/data_base` 对照，定位是哪个地址算错。
可能与 `FILE` 结构体在 `__data` 里的布局、或 `ensure_stdio` 里的
`open_files[64]` 静态数组有关。

## 前置条件（已完成）
- ✅ 多 TU 直接编译能真正链接（`48ac392a`）—— runtime 是"第二个 TU"
- ✅ 允许引用兄弟 TU 的全局/TLS/函数地址（`a4b7d30b`）
- ✅ macho 能跳过 type 10 ADDEND（`a4b7d30b`）
- ✅ `__clone` 弱解析到 0（`4a973536`）
- ✅ BRANCH26_TAIL 在 weak 路径可用（`4a973536`）
- ✅ Linux→Darwin syscall 号码转译（`109a7a53`）
- ✅ 多 TU 不再烘焙单模块段偏移（`f10186a3`）
- ❌ runtime 内部仍有地址 bug（printf 跳垃圾地址）→ **门控必须继续关闭**
