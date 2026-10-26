#!/usr/bin/env bash
# Build the libFuzzer targets: clang builds libfakecc.a with sanitizers and
# coverage instrumentation (fuzzer-no-link), then each target is linked against
# it with libFuzzer's own main.
#
#   fuzz/build_fuzz.sh          # build
#   fuzz/build_fuzz.sh test     # build, then 1000 runs of each target
#   fuzz/build_fuzz.sh clean    # remove build_fuzz/
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD=${BUILD_DIR:-"$ROOT/build_fuzz"}
CLANG=${CLANG:-clang}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}
CORPUS_MAX=${CORPUS_MAX:-300}   # seed files taken from each test suite

if [ "${1:-}" = clean ]; then
    rm -rf "$BUILD"
    exit 0
fi

command -v "$CLANG" >/dev/null || { echo "build_fuzz: clang not found (set CLANG=)"; exit 2; }

# MinGW/windows-gnu clang rejects -fsanitize=fuzzer{,-no-link} even when
# compiler-rt ships the runtime (CLANG64).  Use the coverage set modern
# libFuzzer expects and link libclang_rt.fuzzer-*.a explicitly.  Linux/macOS
# keep the driver flags unchanged.
TRIPLE=$("$CLANG" -dumpmachine 2>/dev/null || true)
WINDOWS_GNU=0
case "$TRIPLE" in
    *windows-gnu*|*w64-windows*|*mingw*) WINDOWS_GNU=1 ;;
esac

GEN_ARGS=()
if command -v ninja >/dev/null 2>&1; then
    # Windows CI always uses Ninja; elsewhere it is optional and equivalent.
    GEN_ARGS+=(-G Ninja)
fi

if [ "$WINDOWS_GNU" = 1 ]; then
    CORE_CFLAGS="-fsanitize=address,undefined -fsanitize-coverage=inline-8bit-counters,pc-table,trace-cmp -fno-omit-frame-pointer -g -O1"
    FUZZ_LINK_CFLAGS="-fsanitize=address,undefined -fsanitize-coverage=inline-8bit-counters,pc-table,trace-cmp -fno-omit-frame-pointer"
    RESOURCE_DIR=$("$CLANG" -print-resource-dir)
    FUZZER_RT=""
    for cand in \
        "$RESOURCE_DIR/lib/windows/libclang_rt.fuzzer-x86_64.a" \
        "$RESOURCE_DIR/lib/windows/libclang_rt.fuzzer-aarch64.a"
    do
        if [ -f "$cand" ]; then
            FUZZER_RT=$cand
            break
        fi
    done
    if [ -z "$FUZZER_RT" ]; then
        echo "build_fuzz: libclang_rt.fuzzer not found under $RESOURCE_DIR/lib/windows" >&2
        exit 2
    fi
    # libFuzzer's archive is C++ and needs the C++ runtime on MinGW.
    FUZZ_EXTRA_LIBS=("$FUZZER_RT" -lstdc++)
    # This link does not go through CMakeLists.txt, which raises the PE stack
    # to 16MB for ASan host binaries.  The default 1MB reserve is what makes
    # fuzz_differential die with AddressSanitizer: stack-overflow (exit 139).
    FUZZ_LINK_CFLAGS="$FUZZ_LINK_CFLAGS -Wl,--stack,16777216"
else
    CORE_CFLAGS="-fsanitize=address,undefined,fuzzer-no-link -fno-omit-frame-pointer -g -O1"
    FUZZ_LINK_CFLAGS="-fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer"
    FUZZ_EXTRA_LIBS=()
fi

# 1. libfakecc.a: ASan + UBSan for the bugs, fuzzer coverage for feedback.
#    -O1 keeps the fuzzer fast enough.
cmake -S "$ROOT" -B "$BUILD" \
    "${GEN_ARGS[@]}" \
    -DCMAKE_C_COMPILER="$CLANG" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_FLAGS="$CORE_CFLAGS" \
    >/dev/null
cmake --build "$BUILD" --parallel "$JOBS" --target fakecc_core >/dev/null

# 2. Seed corpus: the existing suites are far better starting points than random
#    bytes.  The differential target can only use plain C — the e2e cases are
#    written in fakecc's own dialect, which gcc cannot parse.
seed_corpus() {
    local dir=$1 src=$2 n f
    mkdir -p "$dir"
    n=$(find "$dir" -name '*.c' | wc -l)
    while IFS= read -r f; do
        cp "$f" "$dir/$(printf '%04d' $n)_$(basename "$f")"
        n=$((n + 1))
    done < <(find "$src" -name '*.c' -size -8k | sort | awk 'NR % 8 == 1' | head -"$CORPUS_MAX")
    echo "  $dir: $n files"
}
echo "seeding corpus from the existing test suites"
seed_corpus "$BUILD/corpus/compile" "$ROOT/test/e2e/cases"
seed_corpus "$BUILD/corpus/compile" "$ROOT/test/compile/gcc_compile"
seed_corpus "$BUILD/corpus/differential" "$ROOT/test/compile/gcc_compile"

# 3. The targets themselves
mkdir -p "$BUILD/bin"
for t in fuzz_compile fuzz_differential; do
    OUT="$BUILD/bin/$t"
    # shellcheck disable=SC2086
    "$CLANG" -std=c99 -Wall -Wextra -Wno-unused-parameter -O1 -g \
        -I"$ROOT/include" -I"$ROOT/src" \
        $FUZZ_LINK_CFLAGS \
        "$ROOT/fuzz/$t.c" "$BUILD/libfakecc.a" \
        ${FUZZ_EXTRA_LIBS[@]+"${FUZZ_EXTRA_LIBS[@]}"} \
        -o "$OUT"
    # MinGW clang appends .exe when -o has no extension.
    if [ ! -e "$OUT" ] && [ -e "${OUT}.exe" ]; then
        OUT="${OUT}.exe"
    fi
    echo "built $OUT"
done

if [ "${1:-}" = test ]; then
    for t in fuzz_compile fuzz_differential; do
        echo "--- $t: 1000 runs ---"
        BIN="./bin/$t"
        if [ ! -x "$BUILD/$BIN" ] && [ -x "$BUILD/$BIN.exe" ]; then
            BIN="./bin/$t.exe"
        fi
        (cd "$BUILD" && ASAN_OPTIONS=detect_leaks=0 "$BIN" \
            "corpus/${t#fuzz_}" -dict="$ROOT/fuzz/c_keywords.dict" \
            -max_len=2048 -runs=1000 2>&1 | tail -8)
    done
fi
