# 第三方软件手工移植（app_ports）

把 **真实多文件 C 程序** 改成 FakeCC 可编译的形态（`package`、无预处理器宏/`#include`），放在本目录做端到端测试。

这不是 `clang_torture` / `gcc_torture` 那种单文件语言测试，而是「FakeCC 能不能编起来一个完整小软件」。

## 布局（约定）

```
test/app_ports/
├── README.md
├── CANDIDATES.txt          # 候选清单（来自 llvm-test-suite MultiSource）
└── <name>/                 # 每个已移植软件一个子目录（尚未开始）
    ├── ORIGIN.txt          # 上游名称、版本、原路径
    └── …                   # FakeCC 源码（package 化后的 .c）
```

移植完成后再接到 `test/e2e/run_multi_e2e.sh` 或单独的驱动脚本；未接入前本目录 **不参与 CI**。

## 候选从哪来

`CANDIDATES.txt` 列出 llvm-test-suite 里的 **C 多文件工程**（已去掉 C++/ObjC）。原文不再进本仓库，需要时按 `ORIGIN` 从上游按 commit 取出再手工移植。
