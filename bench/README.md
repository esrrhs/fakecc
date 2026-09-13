# Benchmark

独立于 `test/` 自测体系：不进 CTest，也不挂在 CI 的功能回归 job 上。

比较的是 **fakecc 编出来的二进制** 和 **gcc 编出来的二进制** 的运行时间，不是编译器本身的耗时。有自举产物时优先用 `v0/fakecc-1`。

```
bench/
├── run_bench.sh
└── cases/
    ├── sieve.c     # 埃氏筛
    ├── matmul.c    # 整型矩阵乘
    ├── nbody.c     # 小 n-body（牛顿开方，不链 libm）
    └── chacha.c    # ChaCha 风格 ARX
```

```bash
# host fakecc
bash bench/run_bench.sh ./build/fakecc

# 自举后的编译器
bash v0/stage2_check.sh
bash bench/run_bench.sh v0/fakecc-1
```

每个 case 用 fakecc `-O0`/`-O1` 和 gcc `-O0`/`-O1`/`-O2` 各编一份，warmup 一次后取 3 次 median。stdout 校验和必须一致；快慢只打印，不当失败条件。

## 运行结果

机器：本机 x86-64。编译器：`./build/fakecc`（host 构建）、`gcc (GCC) 16.2.0`。  
四份程序的 stdout 校验和与 gcc 完全一致。

32 位 ALU / 立即数 / 就地计算 / 调用参数直传 / `[base+index]` 之后（同一套 case，绝对时间受机器负载影响，看比值）：

| 程序 | fakecc -O0 | gcc -O0 | O0 比值 | fakecc -O1 | gcc -O1 | O1 比值 | gcc -O2 |
|---|---:|---:|---:|---:|---:|---:|---:|
| nbody | 479.3 ms | 316.5 ms | 1.51× | 443.1 ms | 183.4 ms | 2.42× | 185.9 ms |
| sieve | 246.7 ms | 193.1 ms | 1.28× | 240.1 ms | 50.4 ms | 4.76× | 43.1 ms |
| matmul | 646.1 ms | 589.9 ms | 1.10× | 752.8 ms | 109.9 ms | 6.85× | 50.3 ms |
| chacha | 453.0 ms | 347.9 ms | 1.30× | 526.0 ms | 60.2 ms | 8.74× | 64.3 ms |

几何平均：**-O0 约 1.29×，-O1 约 5.1×**（>1 表示 fakecc 编出来的二进制更慢）。  
最初同一套 case 是 **-O0 ~2.0×，-O1 ~7.1×**；栈寻址折叠 + `rol` 之后是 **-O0 ~1.5×**；32 位 ALU / 立即数之后是 **-O0 ~1.39×**。

## 分析结论

1. **正确性没有问题。** 四个 case 在 fakecc `-O0`/`-O1` 与 gcc `-O0`/`-O1`/`-O2` 下打印同一校验和，差距来自生成代码质量，不是算错。

2. **`-O0` 原先慢约一倍，主要不是“少做优化”，而是指令选择比 gcc -O0 啰嗦。** gcc `-O0` 也把标量放栈上、不内联，但每条访存是一条 `mov eax, [rbp-4]`。fakecc 则是 `IR_ADDR` + `IR_LOAD_PTR`：`lea -8(%rbp), %rsi; mov %rsi, %rcx; mov (%rcx), %edi`，每个局部量读写多两到三条指令。参数还要 push/pop 倒手再存栈。ChaCha 的 `rotl` 没有匹配成 `rol`，而是 shl+shr+or。这三件事叠在热循环里，几何平均就到了 ~2×。

3. **已经做的 `-O0` 指令选择（gcc `-O0` 也会做的那种，不是开优化）：**
   - 把 pinned alloca 的 `ADDR`+load/store 收成 `[rbp+off]`，多余的 `lea` 不再发射。
   - 标量参数直接 `mov %edi, [rbp+off]`，绕开 push/pop dance。
   - `(x<<n)|(x>>(W-n))` 降成 `IR_ROL`，编码成 `rol %cl, %eax`。
   - 宽度 4 走 32 位 ALU/比较/除法，`int` 用 `movl` 而不是每条后面跟 `movsxd`。
   - 常量操作数收成 `addl $1` / `and $7` / `sar $3`；结果尽量写在目标寄存器里。
   - 无冲突的寄存器参数直接 `mov` 进 `rdi`/`rsi`，不再每条 call 都 push/pop。
   - 全局数组 `g[i]` 收成 `movzb (%rax,%rcx)` / `movl (%rax,%rcx)`，pinned 局部数组收成 `[rbp+off+index]`。RA 把 GADDR 和下标的活区间拉到 load/store；不跟 `COPY`（指针 select / `SEXT` of const 那种 GEP）。
   收口后 `-O0` 几何平均大约 **2.0× → 1.29×**（筛法 1.67× → 1.28×，矩阵乘 1.10×）。

4. **`-O1` 才是真正的差距，而且按负载类型分得很开。**
   - **n-body（~2.4×）最接近 gcc。** 标量 `double` 循环，带牛顿迭代开方。gcc `-O1` 和 `-O2` 接近，说明这块 gcc 也没有吃到向量化。
   - **筛法 / 矩阵乘 / ChaCha（5–9×）是短板。** gcc `-O1` 已经接近 `-O2`，典型是自动向量化、循环展开和更强的寄存器分配。fakecc 目前只有 SSA 提升、常量折叠和 DCE，没有向量化、没有循环展开。`rol` 让 ChaCha 好一些，但远远不够。

5. **下一步若要收口 `-O1` 比值，优先整型循环：** 下标乘 2/4/8 收进 SIB scale、循环展开 / 向量化。标量浮点（n-body 这一类）不是最急的。剩余的 `-O0` 零头主要是 RA 多出来的 `mov` 中转、指针形参的 `p[i]` 还没走 `[base+index]`、以及 gcc 那种 `movzbl bits(%rax)` 绝对地址。
