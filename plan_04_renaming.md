# Plan 04: mem2reg — SSA 变量重命名

## 目标

实现 mem2reg 的第二步：在支配树上 DFS 遍历，对每个 alloca 变量维护重命名栈，将 LOAD 替换为 COPY、STORE 标记为 dead，并填充 φ 节点的参数。

## 前置条件

Plan 01 (CFG) + Plan 02 (DomTree) + Plan 03 (φ placement) 已完成。

## 背景

当前 `src/opt.c` 行 688-848 有完整的重命名实现，但嵌在 `opt_mem2reg` 函数中。需要提取到 `src/mem2reg.c`。

重命名（renaming）是 SSA 构造的核心步骤：
1. 在支配树上前序 (preorder) DFS
2. 遇到块首的 φ 节点 → push φ 的 dst 到对应 alloca 的重命名栈
3. 遍历块内指令：
   - LOAD from alloca → 替换为 COPY dst=stack_top
   - STORE to alloca → push value 到栈，标记 store 为 dead
4. 为后继块的 φ 节点填充参数（从前驱块的角度）
5. 在支配树后序 (postorder) 时 pop 栈上的 pushes

## 任务

### 1. 实现重命名函数

**在 `src/mem2reg.c` 中添加：**

```c
#include "fakecc/mem2reg.h"

/* 重命名栈 — 每个 alloca 变量一个 */
typedef struct {
    IRValue *vals;
    size_t len, cap;
} RenameStack;

/* Rename all promotable alloca variables.
 *
 * Performs a dominator-tree DFS, replacing LOAD→COPY and STORE→dead,
 * pushing/poping from per-alloca rename stacks, and filling φ arguments.
 *
 * Parameters:
 *   fn              — the function being transformed (insts modified in-place)
 *   cfg             — control-flow graph
 *   dt              — dominator tree
 *   alloca_slots    — array of alloca dst values
 *   num_alloca      — count of alloca slots
 *   block_phi_info  — φ nodes per block (from Plan 03)
 *   dead            — dead instruction bitmap (output, caller frees)
 *                       dead[i]=1 if instruction i should be removed
 */
void mem2reg_rename(
    IRFunction *fn,
    const CFG *cfg,
    const DomTree *dt,
    const int *alloca_slots,
    size_t num_alloca,
    BlockPhiInfo *block_phi_info,
    char **dead /* output, calloc'd */);
```

### 2. 实现细节

#### 2a. 支配树前序遍历

需要计算支配树的 preorder。有两种方式：
- **方式 A**: 按 RPO 遍历（RPO = preorder 的近似）。简单，但必须确保 RPO 是有效的支配树 preorder。
- **方式 B**: 显式构建 children 数组（`domtree_children()`），做 DFS。

**推荐方式 B**——正确性更可靠。

```c
/* 从 dad 的 children 列表获取子节点 */
static void domtree_get_children(const DomTree *dt, int parent,
                                 int **children, size_t *nchildren);
```

#### 2b. 遍历逻辑（伪代码）

```
preorder = domtree_dfs_preorder(dt)
for each block b in preorder:
    // 1. push φ results
    for each φ in block_phi_info[b]:
        ai = find_alloca_index(φ.alloca_slot)
        push(stacks[ai], φ.dst)
        record_push_count(b, ai)

    // 2. process instructions
    for i in range(block.start, block.end):
        inst = &fn->insts[i]
        if inst is LOAD from alloca_slots[ai]:
            inst->op = COPY; inst->a = top(stacks[ai])
        if inst is STORE to alloca_slots[ai]:
            push(stacks[ai], inst->b)
            record_push_count(b, ai)
            dead[i] = 1

    // 3. fill φ args in successors
    for each successor s of b:
        for each φ in block_phi_info[s]:
            ai = find_alloca_index(φ.alloca_slot)
            val = top_or_undef(stacks[ai])
            phi_add_arg(φ, val, b)

// postorder pop
for each block b in reverse_preorder:
    pop_push_count(b) from stacks
```

#### 2c. 未初始化读取处理

当 `top(stacks[ai])` 为空时（变量在 LOAD 前未 STORE），属于 C 的 undefined behavior。替换为 `CONST 0` 并在 stderr 输出警告（匹配 `-Wall` 行为）。

### 3. 编写单元测试

**新建 `test/unit/test_renaming.c`**

测试用例（至少 3 个）：
- `test_rename_simple` — 单个 alloca: ALLOCA, STORE, LOAD, RETURN → COPY, RETURN（无 alloca/store/load）
- `test_rename_two_stores` — x=1; x=2; return x; — 验证 reaching value 是最新的 store
- `test_rename_undef_read` — int x; return x; （未初始化读取）→ 应替换为 CONST 0

### 4. 提取 + 集成

- 从 `src/opt.c` 行 688-848（rename DFS 代码）移到 `src/mem2reg.c`
- 更新 CMakeLists.txt
- 确保全量编译通过 + 全部已有测试通过

## 文件清单

| 操作 | 文件 |
|------|------|
| 修改 | `src/mem2reg.c`（新增 rename 函数）|
| 新建 | `test/unit/test_renaming.c` |
| 修改 | `test/CMakeLists.txt` |
| 修改 | `src/opt.c`（移除 rename 代码）|

## 验收标准

```bash
cd build && cmake .. && make -j$(nproc) && ctest --output-on-failure
# 所有测试通过，包括 test_cfg + test_domtree + test_phi + test_renaming
```

## 注意事项

- **不可以在同一个 DFS 中同时做 φ 插入和重命名**——它们是分开的步骤
- **block_phi_info 来自 Plan 03**——如果 plan 03 的数据结构不同，需要调整接口
- **dead[] 是输出而非即时的**——writer-back（Plan 05）负责从 IR 中删除 dead 指令

## 参考

- `src/opt.c` 行 567-580（RenameStack）、行 582-595（domtree_children）、行 688-848（rename DFS）
- Cytron et al. (1991) — Section 5 (SSA Renaming)
