# Benchmark

[中文说明 (Chinese Documentation)](README_zh.md)

Separate from the `test/` self-check suite: not part of CTest, and not wired into CI functional-regression jobs.

What is compared is the **runtime of binaries built by fakecc** versus **binaries built by gcc**, not the compilers' own compile times. When a bootstrap artifact is available, prefer `v0/fakecc-1`.

```
bench/
├── run_bench.sh    # entry point; forwards to measure.py
├── measure.py      # timing and summary
└── cases/
    ├── sieve.c     # Eratosthenes sieve
    ├── matmul.c    # integer matrix multiply
    ├── nbody.c     # small n-body (Newton square root; does not link libm)
    ├── chacha.c    # ChaCha-style ARX
    ├── struct.c    # nested structs, pass-by-value, return-by-value
    ├── switch.c    # dense switch, sparse switch, case ranges
    ├── recur.c     # recursive quicksort
    ├── fptr.c      # function-pointer table
    ├── vararg.c    # va_list / va_arg
    ├── bitfield.c  # bit-fields
    ├── vla.c       # variable-length arrays
    ├── narrow.c    # char / short promotions
    ├── float.c     # float loop-carried dependence
    ├── pun.c       # union type punning
    ├── string.c    # hand-written copy, compare, hash
    ├── cgoto.c     # computed goto
    ├── stencil.c   # 3-D arrays
    ├── logic.c     # short-circuit, ternary, comma
    ├── sret.c      # multi-arg and struct returns
    ├── i64.c       # 64-bit multiply / divide
    ├── init.c      # designated initializers, compound literals
    ├── stmt.c      # statement expressions, typeof
    ├── vec.c       # integer vectors
    ├── i128.c      # __int128
    ├── complex.c   # _Complex float/double
    ├── bitops.c    # __builtin_clz/ctz/popcount/ffs/bswap
    ├── overflow.c  # __builtin_*_overflow
    ├── flex.c      # flexible array members
    ├── tls.c       # __thread TLS
    ├── sync.c      # __sync_* atomics
    ├── align.c     # aligned/packed structs, _Alignof
    └── ldouble.c   # long double (x87)
```

```bash
# host fakecc
bash bench/run_bench.sh ./build/fakecc

# bootstrap compiler
bash v0/stage2_check.sh
bash bench/run_bench.sh v0/fakecc-1
```

Each case is built once with fakecc `-O0`/`-O1` and once with gcc `-O0`/`-O1`/`-O2`. stdout must match exactly or the whole suite fails; speed differences are printed only and are never treated as failures.

Timing follows [Google Benchmark](https://github.com/google/benchmark)-style repeated trials rather than a fixed three wall-clock runs:

- Comparison uses `perf stat -e task-clock`. That is the CPU time the process actually holds — the same class of clock as Google Benchmark's `CLOCK_PROCESS_CPUTIME_ID`. On this kernel `getrusage` jumps in 4 ms steps, coarser than the noise, so it is not used. Wall clock is taken from the same perf run's elapsed time (the child process's own wall clock, excluding perf startup). A `sched` flag is set only when the two sides' preemption fractions differ by 15 percentage points; when both are preempted together, the ratio still uses task-clock and is unaffected.
- One warmup is discarded. Its CPU time decides the repetition count: enough to fill `--min-time` (default 0.5 s), at least `--min-reps` (default 5), and at most `--max-reps` (default 20). `BENCH_RUNS=N` fixes the count at N.
- Within the same optimization level, fakecc and gcc alternate. The published ratio is the ratio of the two sides' CPU medians; a median of pairwise ratios is shown beside it. When those two numbers diverge, the machine drifted during that round.
- Summaries report mean, median, sample standard deviation (divide by n−1), and coefficient of variation. CV above 5% is marked `noisy`; a median-ratio vs pairwise-median gap above 5% is marked `drift`; a 15-point preemption-fraction gap is marked `sched`.

Each case mutates its own global data, so the function body cannot be re-looped inside one process (a second pass would measure already-filled tables). What is repeated is the whole process — the counterpart of `--benchmark_repetitions`.

## Results

The table below is from the old timer: discard one warmup, then take the median of three wall-clock runs. The new `measure.py` uses a different clock and repetition policy; do not compare against this table directly.

Machine: local x86-64. Compilers: `./build/fakecc` (host build), `gcc (GCC) 16.2.0`.  
Stdout checksums of all four builds match gcc exactly.

After 32-bit ALU / immediates / in-place compute / direct call-arg forwarding / `[base+index]` (same cases; absolute times depend on machine load — look at the ratios):

| Program | fakecc -O0 | gcc -O0 | O0 ratio | fakecc -O1 | gcc -O1 | O1 ratio | gcc -O2 |
|---|---:|---:|---:|---:|---:|---:|---:|
| nbody | 479.3 ms | 316.5 ms | 1.51× | 443.1 ms | 183.4 ms | 2.42× | 185.9 ms |
| sieve | 246.7 ms | 193.1 ms | 1.28× | 240.1 ms | 50.4 ms | 4.76× | 43.1 ms |
| matmul | 646.1 ms | 589.9 ms | 1.10× | 752.8 ms | 109.9 ms | 6.85× | 50.3 ms |
| chacha | 453.0 ms | 347.9 ms | 1.30× | 526.0 ms | 60.2 ms | 8.74× | 64.3 ms |

Geometric mean: **-O0 about 1.29×, -O1 about 5.1×** (>1 means the fakecc-built binary is slower).  
The same cases initially measured **-O0 ~2.0×, -O1 ~7.1×**; after stack-address folding + `rol` they were **-O0 ~1.5×**; after 32-bit ALU / immediates **-O0 ~1.39×**.

## Analysis

1. **Correctness is fine.** All four cases print the same checksum under fakecc `-O0`/`-O1` and gcc `-O0`/`-O1`/`-O2`. The gap is code quality, not wrong answers.

2. **`-O0` used to be about 2× slower mainly because instruction selection was wordier than gcc -O0, not because "fewer optimizations" were run.** gcc `-O0` also keeps scalars on the stack and does not inline, but each access is a single `mov eax, [rbp-4]`. fakecc used `IR_ADDR` + `IR_LOAD_PTR`: `lea -8(%rbp), %rsi; mov %rsi, %rcx; mov (%rcx), %edi` — two to three extra instructions per local read/write. Arguments also went through a push/pop dance before landing on the stack. ChaCha's `rotl` did not match to `rol`, becoming shl+shr+or instead. Stack those three in hot loops and the geometric mean hit ~2×.

3. **`-O0` instruction selection already done (the kind gcc `-O0` also does — not "turning on optimizations"):**
   - Fold pinned-alloca `ADDR`+load/store into `[rbp+off]`; stop emitting the extra `lea`.
   - Scalar parameters go straight `mov %edi, [rbp+off]`, skipping the push/pop dance.
   - Lower `(x<<n)|(x>>(W-n))` to `IR_ROL`, encoded as `rol %cl, %eax`.
   - Width-4 uses 32-bit ALU / compare / divide; `int` uses `movl` instead of a trailing `movsxd` on every op.
   - Fold constant operands into `addl $1` / `and $7` / `sar $3`; write results into the destination register when possible.
   - Conflict-free register arguments go straight `mov` into `rdi`/`rsi` instead of push/pop on every call.
   - Global arrays `g[i]` become `movzb (%rax,%rcx)` / `movl (%rax,%rcx)`; pinned local arrays become `[rbp+off+index]`. RA stretches GADDR and index live ranges to the load/store; it does not follow `COPY` (pointer select / `SEXT` of const-style GEP).
   After these, `-O0` geometric mean is about **2.0× → 1.29×** (sieve 1.67× → 1.28×, matmul 1.10×).

4. **`-O1` is where the real gap is, and it splits sharply by workload.**
   - **n-body (~2.4×) is closest to gcc.** Scalar `double` loop with Newton square-root iteration. gcc `-O1` and `-O2` are close, so gcc is not getting much from vectorization here either.
   - **sieve / matmul / ChaCha (5–9×) are the weak spots.** gcc `-O1` is already near `-O2` — typically auto-vectorization, loop unrolling, and stronger register allocation. fakecc today only has SSA promotion, constant folding, and DCE — no vectorization, no unrolling. `rol` helps ChaCha some, but nowhere near enough.

5. **To close the `-O1` ratio next, prioritize integer loops:** fold index×2/4/8 into SIB scale, loop unrolling / vectorization. Scalar floating point (n-body-class) is not the urgent target. Remaining `-O0` leftover is mostly extra RA `mov` shuttles, pointer-parameter `p[i]` not yet using `[base+index]`, and gcc-style absolute addressing like `movzbl bits(%rax)`.
