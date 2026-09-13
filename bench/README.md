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

| 程序 | fakecc -O0 | gcc -O0 | O0 比值 | fakecc -O1 | gcc -O1 | O1 比值 | gcc -O2 |
|---|---:|---:|---:|---:|---:|---:|---:|
| nbody | 468.3 ms | 308.3 ms | 1.52× | 458.1 ms | 171.5 ms | 2.67× | 169.3 ms |
| sieve | 379.9 ms | 245.6 ms | 1.55× | 250.8 ms | 37.4 ms | 6.71× | 32.5 ms |
| matmul | 1289.9 ms | 518.6 ms | 2.49× | 1022.8 ms | 93.3 ms | 10.96× | 43.5 ms |
| chacha | 844.4 ms | 329.4 ms | 2.56× | 681.3 ms | 52.8 ms | 12.90× | 50.9 ms |

几何平均：**-O0 约 2.0×，-O1 约 7.1×**（>1 表示 fakecc 编出来的二进制更慢）。

## 分析结论

1. **正确性没有问题。** 四个 case 在 fakecc `-O0`/`-O1` 与 gcc `-O0`/`-O1`/`-O2` 下打印同一校验和，差距来自生成代码质量，不是算错。

2. **`-O0` 大约慢一倍，符合预期。** 这一档两边都接近“按语句翻译、标量放内存”。剩余的 ~2× 主要是 fakecc 后端还没有 gcc 那种窥孔/寻址折叠，以及默认链的是自带 runtime 而不是 glibc。n-body / 筛法已经到 1.5×，说明朴素代码路径并没有数量级上的问题。

3. **`-O1` 才是真正的差距，而且按负载类型分得很开。**
   - **n-body（2.7×）最接近 gcc。** 这是标量 `double` 循环，带牛顿迭代开方。gcc `-O1` 和 `-O2` 几乎一样（171 ms vs 169 ms），说明这块 gcc 也没有吃到向量化，fakecc 的 SSA + 标量分配已经能跟上“非向量化的 gcc”。
   - **筛法（6.7×）** 是字节数组上的步进清位。gcc 更容易把内层 `i += p` 收成紧凑的寻址与位运算；fakecc `-O1` 相对 `-O0` 有加速（380 ms → 251 ms），但远不如 gcc 把 246 ms 收到 37 ms。
   - **矩阵乘 / ChaCha（11–13×）是短板。** 都是规则的整型热循环。gcc `-O1` 已经接近 `-O2`（chacha 53 ms vs 51 ms），典型是自动向量化、循环展开和更强的寄存器分配。fakecc 目前只有 SSA 提升、常量折叠和 DCE，没有向量化、没有循环展开，整型 MAC / rotate-xor 只能走标量。

4. **fakecc `-O1` 相对自己的 `-O0` 收益很小**（n-body 几乎没动，matmul 只快约 20%），而 gcc `-O0` → `-O1` 普遍快 4–6 倍。这说明当前 `-O1` 管线还没有碰到这些循环的主要成本：访存形状、指令选择和 SIMD。

5. **下一步若要收口 `-O1` 比值，优先整型循环：** 识别 rotate、把嵌套循环的地址计算收进寻址模式、再考虑循环展开 / 向量化。标量浮点（n-body 这一类）不是最急的。
