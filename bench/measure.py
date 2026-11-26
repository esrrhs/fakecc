#!/usr/bin/env python3
"""Compare binaries produced by fakecc and gcc.

The clock and the aggregates follow Google Benchmark
(https://github.com/google/benchmark, "User Guide" / repetition stats):

* Report time the process was scheduled (perf task-clock).  That is the
  high-resolution process CPU clock Google Benchmark reads with
  CLOCK_PROCESS_CPUTIME_ID.  getrusage on this kernel steps in 4 ms, which
  is coarser than the noise, so it is not the comparison clock.  Wall time
  is perf's own "seconds time elapsed" for the same child, so perf's
  startup is not counted.  `sched` means the two binaries spent different
  fractions of their wall time off-CPU.  Equal descheduling does not bias
  a task-clock ratio, so it is not flagged.
* Throw away one warmup.  Its CPU time picks the repetition count: enough
  samples that each binary accumulates --min-time seconds (their default
  is 0.5s), at least --min-reps, and never more than --max-reps.
* One repetition is one execution of the whole program.  Cases mutate
  their own globals, so looping the body inside the process would time a
  second pass over already-filled tables.  Repeating the process is the
  same idea as --benchmark_repetitions.
* At each -O level, fakecc and gcc alternate (fcc, gcc, fcc, gcc, ...).
  The published ratio is median(fcc) / median(gcc), which is how Google
  Benchmark compares two aggregates.  The median of the per-pair ratios
  is printed beside it.  A gap between those two numbers means the
  machine moved between the two binaries.
* mean, median, sample standard deviation (divide by n-1) and the
  coefficient of variation are the repetition aggregates Google Benchmark
  prints.

One untimed run of every binary must print the same stdout, and the selfhost
-O0 compiler pair must emit byte-identical compiler binaries.  When red-line
thresholds (--gate / --max-*) are given, exceeding a threshold fails too.
"""
import argparse
import math
import os
import statistics
import subprocess
import sys
import tempfile
import time

try:
    import resource
except ImportError:
    resource = None


DEFAULT_MAX_O0_GEOMEAN = 1.15
DEFAULT_MAX_O1_GEOMEAN = 4.00
DEFAULT_MAX_SELFHOST_O0 = 1.15


def parse_args(argv):
    p = argparse.ArgumentParser(description=__doc__.split("\n", 1)[0])
    p.add_argument("--fakecc", required=True)
    p.add_argument("--gcc", default="gcc")
    p.add_argument("--cases", default="")
    p.add_argument("--only", default="",
                   help="comma-separated case names (include 'selfhost' for the -O0 compiler self-compile)")
    p.add_argument("--no-selfhost", action="store_true",
                   help="skip the selfhost -O0 compiler self-compile benchmark")
    p.add_argument("--selfhost-reps", type=int, default=1,
                   help="measured repetitions for selfhost -O0 compile pair (default 1)")
    p.add_argument("--min-time", type=float, default=0.5,
                   help="seconds of CPU time to accumulate per binary (default 0.5)")
    p.add_argument("--min-reps", type=int, default=5)
    p.add_argument("--max-reps", type=int, default=20)
    p.add_argument("--repetitions", type=int, default=0,
                   help="if >0, measure exactly this many reps and ignore min-time")
    p.add_argument("--cv-warn", type=float, default=0.05,
                   help="flag a result when sample cv exceeds this (default 0.05)")
    p.add_argument("--gate", action="store_true",
                   help="enforce default red-line thresholds (-O0 geomean <= %.2fx, -O1 geomean <= %.2fx, selfhost -O0 <= %.2fx)"
                        % (DEFAULT_MAX_O0_GEOMEAN, DEFAULT_MAX_O1_GEOMEAN, DEFAULT_MAX_SELFHOST_O0))
    p.add_argument("--max-o0-geomean", type=float, default=0.0,
                   help="fail if microbench -O0 geomean ratio exceeds this")
    p.add_argument("--max-o1-geomean", type=float, default=0.0,
                   help="fail if microbench -O1 geomean ratio exceeds this")
    p.add_argument("--max-selfhost-o0", type=float, default=0.0,
                   help="fail if selfhost -O0 compiler ratio exceeds this")
    return p.parse_args(argv)


def to_gcc(src, dst):
    with open(src, "rb") as fh:
        text = fh.read().decode("latin-1")
    out = []
    for line in text.splitlines():
        stripped = line.lstrip(" ")
        if stripped.startswith("package ") and stripped.endswith(";"):
            continue
        if stripped.startswith("import ") and stripped.endswith(";"):
            continue
        out.append(line.replace("runtime.", ""))
    with open(dst, "w", encoding="latin-1") as fh:
        fh.write("#include <stdio.h>\n#include <stdarg.h>\n")
        fh.write("\n".join(out))
        fh.write("\n")


def run_plain(cmd, env=None):
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    return proc.returncode, proc.stdout, proc.stderr


def parse_perf_stat(text):
    """Parse `perf stat -e task-clock -o FILE`.

    task-clock is milliseconds.  "seconds time elapsed" is the child's wall
    time, which does not include the time spent starting perf itself.
    """
    cpu = None
    wall = None
    for line in text.splitlines():
        if "task-clock" in line and "msec" in line:
            field = line.split()[0]
            if field != "<not":
                cpu = float(field) / 1000.0
        elif "seconds time elapsed" in line:
            wall = float(line.split()[0])
    return cpu, wall


def perf_available():
    fd, path = tempfile.mkstemp(prefix="perf-stat-")
    os.close(fd)
    try:
        proc = subprocess.run(
            ["perf", "stat", "-e", "task-clock", "-o", path, "--", "true"],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        with open(path, "r", errors="replace") as fh:
            cpu, wall = parse_perf_stat(fh.read())
    except (OSError, ValueError):
        return False
    finally:
        try:
            os.unlink(path)
        except OSError:
            pass
    return proc.returncode == 0 and cpu is not None and wall is not None


USE_PERF = None


def run_timed(cmd, env=None):
    """Return (rc, wall_s, cpu_s, stdout, stderr).

    cpu_s is perf's task-clock: time the process was actually scheduled.
    That is the high-resolution process CPU clock Google Benchmark reads
    via CLOCK_PROCESS_CPUTIME_ID.  When perf is unavailable (e.g. cloud VMs
    without matching linux-tools), fall back to POSIX RUSAGE_CHILDREN user+sys
    CPU time before falling back to wall clock.
    """
    global USE_PERF
    if USE_PERF is None:
        USE_PERF = perf_available()
    if USE_PERF:
        fd, path = tempfile.mkstemp(prefix="perf-stat-")
        os.close(fd)
        try:
            proc = subprocess.run(
                ["perf", "stat", "-e", "task-clock", "-o", path, "--"] + list(cmd),
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
            with open(path, "r", errors="replace") as fh:
                cpu, wall = parse_perf_stat(fh.read())
        finally:
            try:
                os.unlink(path)
            except OSError:
                pass
        if cpu is None or wall is None:
            if proc.returncode != 0:
                return proc.returncode, 0.0, 0.0, proc.stdout, proc.stderr
            raise RuntimeError("perf stat produced no task-clock")
        return proc.returncode, wall, cpu, proc.stdout, proc.stderr
    if resource is not None:
        r0 = resource.getrusage(resource.RUSAGE_CHILDREN)
        t0 = time.perf_counter()
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
        wall = time.perf_counter() - t0
        r1 = resource.getrusage(resource.RUSAGE_CHILDREN)
        cpu = max(0.0, (r1.ru_utime - r0.ru_utime) + (r1.ru_stime - r0.ru_stime))
        return proc.returncode, wall, cpu, proc.stdout, proc.stderr
    t0 = time.perf_counter()
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    wall = time.perf_counter() - t0
    return proc.returncode, wall, wall, proc.stdout, proc.stderr


def compile_one(cmd, err_path, env=None):
    rc, _out, err = run_plain(cmd, env=env)
    if rc != 0:
        with open(err_path, "wb") as fh:
            fh.write(err)
        return False
    return True


def sample_std(xs):
    if len(xs) < 2:
        return 0.0
    m = statistics.fmean(xs)
    acc = sum((x - m) * (x - m) for x in xs)
    return math.sqrt(acc / (len(xs) - 1))


def cv_of(xs):
    m = statistics.fmean(xs)
    if m <= 0:
        return 0.0
    return sample_std(xs) / m


def choose_reps(pilot, args):
    if args.repetitions > 0:
        return args.repetitions
    if pilot <= 0:
        return args.min_reps
    need = int(math.ceil(args.min_time / pilot))
    return max(args.min_reps, min(args.max_reps, need))


def measure_binary(path, reps):
    """Warmup once, then `reps` measured runs.  Returns lists of CPU and wall seconds."""
    rc, _w, _c, _o, err = run_timed([path])
    if rc != 0:
        raise RuntimeError(err.decode("latin-1", "replace").splitlines()[:4])
    cpus = []
    walls = []
    for _ in range(reps):
        rc, wall, cpu, _o, err = run_timed([path])
        if rc != 0:
            raise RuntimeError(err.decode("latin-1", "replace").splitlines()[:4])
        cpus.append(cpu)
        walls.append(wall)
    return cpus, walls


def warmup(path):
    rc, _wall, cpu, _out, err = run_timed([path])
    if rc != 0:
        raise RuntimeError("%s: %s" % (path, err.decode("latin-1", "replace")[:400]))
    return cpu


def measure_pair(fcc, gcc, reps):
    """Interleave measured runs.  Caller already warmed both binaries up."""
    fcc_cpu, fcc_wall = [], []
    gcc_cpu, gcc_wall = [], []
    for _ in range(reps):
        rc, wall, cpu, _o, err = run_timed([fcc])
        if rc != 0:
            raise RuntimeError(err.decode("latin-1", "replace")[:400])
        fcc_cpu.append(cpu)
        fcc_wall.append(wall)
        rc, wall, cpu, _o, err = run_timed([gcc])
        if rc != 0:
            raise RuntimeError(err.decode("latin-1", "replace")[:400])
        gcc_cpu.append(cpu)
        gcc_wall.append(wall)
    return fcc_cpu, fcc_wall, gcc_cpu, gcc_wall


def fmt_ms(seconds):
    return "%.1f" % (seconds * 1000.0)


def fmt_cv(xs):
    return "%.1f%%" % (100.0 * cv_of(xs))


def ratio(a, b):
    if b <= 0:
        return 0.0
    return a / b


def summarize_pair(fcc_cpu, fcc_wall, gcc_cpu, gcc_wall, args):
    fcc_med = statistics.median(fcc_cpu)
    gcc_med = statistics.median(gcc_cpu)
    of_medians = ratio(fcc_med, gcc_med)
    pairs = [ratio(a, b) for a, b in zip(fcc_cpu, gcc_cpu)]
    pair_med = statistics.median(pairs)
    notes = []
    if cv_of(fcc_cpu) > args.cv_warn or cv_of(gcc_cpu) > args.cv_warn:
        notes.append("noisy")
    if of_medians > 0 and abs(of_medians - pair_med) / of_medians > args.cv_warn:
        notes.append("drift")
    # Both binaries lose the same off-CPU time under load; the ratio uses
    # task-clock, so that does not bias it.  Flag only when the two sides
    # were scheduled differently.
    def off_frac(wall, cpu):
        if wall <= 0:
            return 0.0
        return max(0.0, (wall - cpu) / wall)
    off_fcc = off_frac(statistics.median(fcc_wall), fcc_med)
    off_gcc = off_frac(statistics.median(gcc_wall), gcc_med)
    if abs(off_fcc - off_gcc) > 0.15:
        notes.append("sched")
    return {
        "fcc_med": fcc_med,
        "gcc_med": gcc_med,
        "fcc_cv": cv_of(fcc_cpu),
        "gcc_cv": cv_of(gcc_cpu),
        "ratio": of_medians,
        "pair": pair_med,
        "notes": notes,
    }


def geomean(xs):
    if not xs:
        return 0.0
    acc = 0.0
    for x in xs:
        if x <= 0:
            return 0.0
        acc += math.log(x)
    return math.exp(acc / len(xs))


def case_names(case_dir, only, no_selfhost):
    c_names = []
    for fn in sorted(os.listdir(case_dir)):
        if fn.endswith(".c"):
            c_names.append(fn[:-2])
    valid = set(c_names) | {"selfhost"}
    if only:
        want = [s.strip() for s in only.split(",") if s.strip()]
        missing = [s for s in want if s not in valid]
        if missing:
            sys.stderr.write("measure: unknown case %s\n" % ", ".join(missing))
            return None, False, len(c_names)
        names = [n for n in c_names if n in want]
        want_selfhost = ("selfhost" in want) and not no_selfhost
        return names, want_selfhost, len(c_names)
    return c_names, (not no_selfhost), len(c_names)


def first_diff_offset(a, b):
    limit = min(len(a), len(b))
    for i in range(limit):
        if a[i] != b[i]:
            return i
    if len(a) != len(b):
        return limit
    return -1


def run_selfhost_case(root, work, args, env):
    """Build fakecc -O0 and gcc -O0 compiler binaries, then time each compiling
    the full translated compiler source tree and verify byte-identical outputs.
    """
    reps = args.selfhost_reps
    v0_dir = os.path.join(work, "v0")
    tr_env = dict(env)
    tr_env["BENCH_GCC"] = args.gcc
    rc, _out, err = run_plain(
        [sys.executable, os.path.join(root, "v0", "translate.py"), v0_dir],
        env=tr_env)
    if rc != 0:
        print("FAIL selfhost: translate.py")
        sys.stderr.write(err.decode("latin-1", "replace")[:500] + "\n")
        return None

    drop = "macho" if os.environ.get("FAKECC_NO_MACHO", "") in ("1", "ON") else "hoststub"
    src_dir = os.path.join(root, "src")
    modules = sorted(
        fn[:-2] for fn in os.listdir(src_dir)
        if fn.endswith(".c") and fn[:-2] != drop
    )
    src_files = [os.path.join(src_dir, m + ".c") for m in modules]
    v0_files = [os.path.join(v0_dir, "ir.c")] + [
        os.path.join(v0_dir, m + ".c") for m in modules if m != "ir"
    ]

    exe = ".exe" if sys.platform == "win32" else ""
    gcc_cc = os.path.join(work, "gcc-o0-fakecc" + exe)
    fcc_cc = os.path.join(work, "fcc-o0-fakecc" + exe)

    gcc_cmd = [args.gcc, "-std=c99", "-O0"]
    if sys.platform == "darwin":
        gcc_cmd.append("-D_DARWIN_C_SOURCE")
    gcc_cmd += ["-I", os.path.join(root, "include")] + src_files + ["-o", gcc_cc]
    if sys.platform != "win32":
        gcc_cmd.append("-lm")

    fcc_cmd = [args.fakecc, "-O0"] + v0_files + ["-o", fcc_cc]

    print("--- selfhost (-O0 compilers compiling %d modules, reps %d) ---"
          % (len(modules), reps), flush=True)
    for key, cmd in (("gccO0-cc", gcc_cmd), ("fccO0-cc", fcc_cmd)):
        dest = cmd[-2] if cmd[-1] == "-lm" else cmd[-1]
        if not compile_one(cmd, dest + ".err", env=env):
            print("FAIL selfhost: %s build" % key)
            err_txt = open(dest + ".err", "rb").read().decode("latin-1", "replace")
            sys.stderr.write(err_txt[:500] + "\n")
            return None

    fcc_cpu, fcc_wall = [], []
    gcc_cpu, gcc_wall = [], []
    out_size = 0
    for r in range(reps):
        out_fcc = os.path.join(work, "selfhost.fcc.%d%s" % (r, exe))
        out_gcc = os.path.join(work, "selfhost.gcc.%d%s" % (r, exe))
        rc, wall, cpu, _o, err = run_timed([fcc_cc] + v0_files + ["-o", out_fcc], env=env)
        if rc != 0:
            print("FAIL selfhost: fcc -O0 compiler run")
            sys.stderr.write(err.decode("latin-1", "replace")[:500] + "\n")
            return None
        fcc_cpu.append(cpu)
        fcc_wall.append(wall)

        rc, wall, cpu, _o, err = run_timed([gcc_cc] + v0_files + ["-o", out_gcc], env=env)
        if rc != 0:
            print("FAIL selfhost: gcc -O0 compiler run")
            sys.stderr.write(err.decode("latin-1", "replace")[:500] + "\n")
            return None
        gcc_cpu.append(cpu)
        gcc_wall.append(wall)

        with open(out_fcc, "rb") as fh:
            b_fcc = fh.read()
        with open(out_gcc, "rb") as fh:
            b_gcc = fh.read()
        if b_fcc != b_gcc:
            off = first_diff_offset(b_fcc, b_gcc)
            print("MISMATCH selfhost: fcc output (%d B) != gcc output (%d B) at byte %d"
                  % (len(b_fcc), len(b_gcc), off))
            return None
        out_size = len(b_fcc)
        for p in (out_fcc, out_gcc):
            try:
                os.unlink(p)
            except OSError:
                pass

    summary = summarize_pair(fcc_cpu, fcc_wall, gcc_cpu, gcc_wall, args)
    summary["bytes"] = out_size
    summary["reps"] = reps
    note = ("  " + ",".join(summary["notes"])) if summary["notes"] else ""
    print("  O0  fcc %s ms (cv %s)   gcc %s ms (cv %s)   ratio %.2fx   pair %.2fx   (%d B identical)%s"
          % (fmt_ms(summary["fcc_med"]), fmt_cv(fcc_cpu),
             fmt_ms(summary["gcc_med"]), fmt_cv(gcc_cpu),
             summary["ratio"], summary["pair"], out_size, note), flush=True)
    return summary


def main(argv):
    args = parse_args(argv)
    if args.min_reps < 1 or args.max_reps < args.min_reps:
        sys.stderr.write("measure: need 1 <= min-reps <= max-reps\n")
        return 2
    if args.selfhost_reps < 1:
        sys.stderr.write("measure: need selfhost-reps >= 1\n")
        return 2
    if not os.access(args.fakecc, os.X_OK):
        sys.stderr.write("measure: fakecc not executable: %s\n" % args.fakecc)
        return 2

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    case_dir = os.path.join(root, "bench", "cases")
    names, want_selfhost, total_micro = case_names(case_dir, args.only, args.no_selfhost)
    if names is None:
        return 2

    full_micro = (len(names) == total_micro and total_micro > 0)
    max_o0 = args.max_o0_geomean or (DEFAULT_MAX_O0_GEOMEAN if (args.gate and full_micro) else 0.0)
    max_o1 = args.max_o1_geomean or (DEFAULT_MAX_O1_GEOMEAN if (args.gate and full_micro) else 0.0)
    max_sh = args.max_selfhost_o0 or (DEFAULT_MAX_SELFHOST_O0 if (args.gate and want_selfhost) else 0.0)

    env = dict(os.environ)
    if not env.get("FAKECC_RT"):
        env["FAKECC_RT"] = os.path.join(root, "runtime")

    gcc_ver = subprocess.run([args.gcc, "--version"], stdout=subprocess.PIPE, text=True)
    gcc_line = gcc_ver.stdout.splitlines()[0] if gcc_ver.stdout else args.gcc
    kind = "self-hosted" if os.path.basename(args.fakecc) in ("fakecc-1", "fakecc-2") else "host"
    global USE_PERF
    USE_PERF = perf_available()
    if USE_PERF:
        clock_desc = "perf task-clock (time scheduled on CPU)"
    elif resource is not None:
        clock_desc = "rusage child CPU time (perf task-clock unavailable)"
    else:
        clock_desc = "wall clock; perf task-clock is unavailable"
    print("=== fakecc vs gcc  (task-clock, interleaved repetitions) ===")
    print("fakecc: %s  (%s)" % (args.fakecc, kind))
    print("gcc:    %s" % gcc_line)
    print("clock:  %s" % clock_desc)
    if args.repetitions > 0:
        print("reps:   warmup discarded, then exactly %d" % args.repetitions)
    else:
        print("reps:   warmup discarded; clamp(ceil(%.2fs / pilot), %d, %d)"
              % (args.min_time, args.min_reps, args.max_reps))
    print("ratio:  median CPU(fakecc) / median CPU(gcc);  pair = median of per-rep ratios")
    print("cv:     sample stddev / mean.  flagged above %.0f%%" % (100 * args.cv_warn))
    gates = []
    if max_o0 > 0:
        gates.append("-O0 geomean <= %.2fx" % max_o0)
    if max_o1 > 0:
        gates.append("-O1 geomean <= %.2fx" % max_o1)
    if max_sh > 0:
        gates.append("selfhost -O0 <= %.2fx" % max_sh)
    if gates:
        print("gates:  %s" % ", ".join(gates))
    print(flush=True)

    fail = 0
    rows = []
    selfhost_summary = None
    # levels compared as pairs, plus gcc -O2 as a reference column
    levels = ("O0", "O1")

    work = tempfile.mkdtemp(prefix="fakecc-bench-")
    try:
        for name in names:
            src = os.path.join(case_dir, name + ".c")
            gcc_src = os.path.join(work, name + ".gcc.c")
            to_gcc(src, gcc_src)
            bins = {}
            ok = True
            builds = [
                ("fccO0", [args.fakecc, "-O0", src, "-o", os.path.join(work, name + ".fcc.O0")]),
                ("fccO1", [args.fakecc, "-O1", src, "-o", os.path.join(work, name + ".fcc.O1")]),
                ("gccO0", [args.gcc, "-std=gnu99", "-ffp-contract=off", "-O0", gcc_src, "-o", os.path.join(work, name + ".gcc.O0")]),
                ("gccO1", [args.gcc, "-std=gnu99", "-ffp-contract=off", "-O1", gcc_src, "-o", os.path.join(work, name + ".gcc.O1")]),
                ("gccO2", [args.gcc, "-std=gnu99", "-ffp-contract=off", "-O2", gcc_src, "-o", os.path.join(work, name + ".gcc.O2")]),
            ]
            for key, cmd in builds:
                dest = cmd[-1]
                if not compile_one(cmd, dest + ".err", env=env):
                    print("FAIL %s: %s compile" % (name, key))
                    err = open(dest + ".err", "rb").read().decode("latin-1", "replace")
                    sys.stderr.write(err[:500] + "\n")
                    ok = False
                    break
                bins[key] = dest
            if not ok:
                fail = 1
                continue

            outputs = {}
            for key, path in bins.items():
                rc, out, err = run_plain([path], env=env)
                if rc != 0:
                    print("FAIL %s: %s run" % (name, key))
                    sys.stderr.write(err.decode("latin-1", "replace")[:400] + "\n")
                    ok = False
                    break
                outputs[key] = out
            if not ok:
                fail = 1
                continue
            base = outputs["gccO0"]
            if any(outputs[k] != base for k in outputs):
                print("MISMATCH %s" % name)
                for k in ("fccO0", "fccO1", "gccO0", "gccO1", "gccO2"):
                    print("  %s: %s" % (k, outputs[k].decode("latin-1", "replace").strip()))
                fail = 1
                continue

            # The -O0 warmup is the pilot that picks the repetition count.
            # It is not one of the measured samples.
            try:
                pilot_fcc = warmup(bins["fccO0"])
                pilot_gcc = warmup(bins["gccO0"])
            except RuntimeError as exc:
                print("FAIL %s: pilot: %s" % (name, exc))
                fail = 1
                continue
            reps = choose_reps(max(pilot_fcc, pilot_gcc), args)
            print("--- %s  (reps %d) ---" % (name, reps))

            row = {"name": name, "reps": reps, "match": "ok"}
            try:
                first = True
                for lvl in levels:
                    if not first:
                        warmup(bins["fcc" + lvl])
                        warmup(bins["gcc" + lvl])
                    first = False
                    fcc_cpu, fcc_wall, gcc_cpu, gcc_wall = measure_pair(
                        bins["fcc" + lvl], bins["gcc" + lvl], reps)
                    summary = summarize_pair(fcc_cpu, fcc_wall, gcc_cpu, gcc_wall, args)
                    row[lvl] = summary
                    note = ("  " + ",".join(summary["notes"])) if summary["notes"] else ""
                    print("  %s  fcc %s ms (cv %s)   gcc %s ms (cv %s)   ratio %.2fx   pair %.2fx%s"
                          % (lvl,
                             fmt_ms(summary["fcc_med"]), fmt_cv(fcc_cpu),
                             fmt_ms(summary["gcc_med"]), fmt_cv(gcc_cpu),
                             summary["ratio"], summary["pair"], note))
                o2_cpu, _o2_wall = measure_binary(bins["gccO2"], reps)
                row["gccO2"] = statistics.median(o2_cpu)
                row["gccO2_cv"] = cv_of(o2_cpu)
                print("  O2  gcc %s ms (cv %s)" % (fmt_ms(row["gccO2"]), fmt_cv(o2_cpu)))
            except RuntimeError as exc:
                print("FAIL %s: %s" % (name, exc))
                fail = 1
                continue
            rows.append(row)

        if want_selfhost:
            selfhost_summary = run_selfhost_case(root, work, args, env)
            if selfhost_summary is None:
                fail = 1
    finally:
        # Keep the tree small; binaries are not part of the result.
        subprocess.run(["rm", "-rf", work], check=False)

    g0 = 0.0
    g1 = 0.0
    if rows:
        print()
        print("--------------------------------------------------------------------------------")
        print("%-8s %4s %10s %6s %10s %6s %6s %6s %10s %6s %8s %s"
              % ("bench", "reps", "fcc O0", "cv", "gcc O0", "cv", "O0x", "pair",
                 "fcc O1", "O1x", "gcc O2", "note"))
        print("--------------------------------------------------------------------------------")
        for row in rows:
            o0 = row["O0"]
            o1 = row["O1"]
            notes = []
            notes.extend(o0["notes"])
            if o1["notes"]:
                notes.extend("O1:" + n for n in o1["notes"])
            print("%-8s %4d %8s ms %6s %8s ms %6s %5.2fx %5.2fx %8s ms %5.2fx %7s ms %s"
                  % (row["name"], row["reps"],
                     fmt_ms(o0["fcc_med"]), "%.0f%%" % (100 * o0["fcc_cv"]),
                     fmt_ms(o0["gcc_med"]), "%.0f%%" % (100 * o0["gcc_cv"]),
                     o0["ratio"], o0["pair"],
                     fmt_ms(o1["fcc_med"]), o1["ratio"],
                     fmt_ms(row["gccO2"]),
                     ",".join(notes)))
        print("--------------------------------------------------------------------------------")
        g0 = geomean([r["O0"]["ratio"] for r in rows])
        g1 = geomean([r["O1"]["ratio"] for r in rows])
        gp = geomean([r["O0"]["pair"] for r in rows])
        print("geometric mean of median-CPU ratios   -O0: %.2fx   -O1: %.2fx" % (g0, g1))
        print("geometric mean of paired -O0 ratios   -O0: %.2fx" % gp)
        print("(>1 means fakecc's binary is slower; cv is sample stddev/mean)")

    if selfhost_summary is not None:
        print()
        print("selfhost -O0 compiler self-compile    fcc: %s ms   gcc: %s ms   ratio: %.2fx   (%d B identical)"
              % (fmt_ms(selfhost_summary["fcc_med"]),
                 fmt_ms(selfhost_summary["gcc_med"]),
                 selfhost_summary["ratio"],
                 selfhost_summary["bytes"]))

    redline_fails = []
    if rows and max_o0 > 0 and g0 > max_o0:
        redline_fails.append("microbench -O0 geomean %.2fx > max %.2fx" % (g0, max_o0))
    if rows and max_o1 > 0 and g1 > max_o1:
        redline_fails.append("microbench -O1 geomean %.2fx > max %.2fx" % (g1, max_o1))
    if selfhost_summary is not None and max_sh > 0 and selfhost_summary["ratio"] > max_sh:
        redline_fails.append("selfhost -O0 ratio %.2fx > max %.2fx"
                             % (selfhost_summary["ratio"], max_sh))
    for msg in redline_fails:
        print("REDLINE FAIL: %s" % msg)
    if redline_fails:
        fail = 1

    if fail:
        print("run_bench: FAILED")
        return 1
    if gates:
        print("run_bench: PASS (outputs match; all red-line gates satisfied)")
    else:
        print("run_bench: outputs match; ratios are informational")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
