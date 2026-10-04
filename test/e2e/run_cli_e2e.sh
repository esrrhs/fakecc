#!/usr/bin/env bash
# Exercise fakecc CLI error paths and less-used flags (coverage of src/main.c).
set -uo pipefail
# Portability helpers (timeout/nproc/readelf shims for non-GNU hosts).
_COMPAT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
# Abort if the shims could not be sourced: a silently missing `timeout` turns
# every case into a confusing "command not found" instead of a clear error.
. "$_COMPAT_DIR/compat.sh" || { echo "cannot source compat.sh from $_COMPAT_DIR" >&2; exit 2; }
# run_e2e fans cases out to `xargs bash -c` workers, which only see exported
# functions, so re-export the shims this script's workers rely on.
export -f timeout nproc 2>/dev/null || true


FAKECC=${1:-./build/fakecc}
shift || true
CC_EXTRA="${CC_FLAGS:-} $*"
pass=0
fail=0

ok() { echo "PASS $1"; pass=$((pass + 1)); }
bad() { echo "FAIL $1"; fail=$((fail + 1)); }

expect_fail() {
    local name="$1"; shift
    if "$@" >/dev/null 2>&1; then bad "$name (expected failure)"; else ok "$name"; fi
}

expect_ok() {
    local name="$1"; shift
    if "$@" >/dev/null 2>&1; then ok "$name"; else bad "$name"; fi
}

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/t.c" <<'EOF'
package main;
int main(void) { return 0; }
EOF

expect_fail "no args" "$FAKECC"
expect_fail "unknown option" "$FAKECC" -zzz "$TMP/t.c" -o "$TMP/out"
expect_fail "missing input file" "$FAKECC" "$TMP/no_such.c" -o "$TMP/out"
expect_fail "missing -o value" "$FAKECC" "$TMP/t.c" -o
expect_fail "missing -L value" "$FAKECC" "$TMP/t.c" -o "$TMP/out" -L
expect_fail "missing -l value" "$FAKECC" "$TMP/t.c" -o "$TMP/out" -l
expect_fail "-c two inputs" "$FAKECC" -c "$TMP/t.c" "$TMP/t.c" -o "$TMP/t.o"
expect_fail "-c with -l" "$FAKECC" -c -lfoo "$TMP/t.c" -o "$TMP/t.o"
expect_fail "empty -l" "$FAKECC" "$TMP/t.c" -l "" -o "$TMP/out"

expect_ok "-c -O2" "$FAKECC" $CC_EXTRA -c -O2 "$TMP/t.c" -o "$TMP/t.o"
expect_ok "-c -O3" "$FAKECC" $CC_EXTRA -c -O3 "$TMP/t.c" -o "$TMP/t.o"
# -mavx selects 32-byte YMM vectors, which only exist on the x86-64 backend.
# arm64 always uses 16-byte NEON vectors, so there the flag is correctly
# rejected; assert whichever contract the host target actually has instead of
# hard-coding the x86 answer.
if [ "$(uname -m)" = "arm64" ] || [ "$(uname -m)" = "aarch64" ]; then
    expect_fail "-c -mavx (rejected on arm64)" "$FAKECC" $CC_EXTRA -c -mavx "$TMP/t.c" -o "$TMP/t.o"
    expect_ok "-c -mno-avx (no-op on arm64)" "$FAKECC" $CC_EXTRA -c -mno-avx "$TMP/t.c" -o "$TMP/t.o"
else
    expect_ok "-c -mavx" "$FAKECC" $CC_EXTRA -c -mavx "$TMP/t.c" -o "$TMP/t.o"
fi
expect_ok "-c -fno-builtin" "$FAKECC" $CC_EXTRA -c -fno-builtin "$TMP/t.c" -o "$TMP/t.o"
expect_ok "-c -fsanitize=undefined,address" "$FAKECC" $CC_EXTRA -c -fsanitize=undefined,address "$TMP/t.c" -o "$TMP/t.o"
expect_ok "-c -g" "$FAKECC" $CC_EXTRA -c -g "$TMP/t.c" -o "$TMP/t.o"

# Duplicate -L should hit the paths_add already-present branch; compile-only
# still rejects -L, so use a full link with a dummy unused search path.
expect_ok "link -L twice" "$FAKECC" $CC_EXTRA "$TMP/t.c" -L/tmp -L/tmp -o "$TMP/out"
expect_ok "link -L DIR" "$FAKECC" $CC_EXTRA "$TMP/t.c" -L /tmp -o "$TMP/out2"

# FAKECC_RT override
if [ -f runtime/string.c ]; then
    RT=$(cd runtime && pwd)
    expect_ok "FAKECC_RT" env FAKECC_RT="$RT" "$FAKECC" $CC_EXTRA "$TMP/t.c" -o "$TMP/out_rt"
fi

# argv0-relative runtime: copy compiler next to a runtime tree.
mkdir -p "$TMP/root/bin" "$TMP/root/runtime"
cp "$FAKECC" "$TMP/root/bin/fakecc"
if [ -d runtime ]; then
    cp runtime/*.c "$TMP/root/runtime/" 2>/dev/null || true
fi
if [ -f "$TMP/root/runtime/string.c" ]; then
    expect_ok "argv0 ../runtime" "$TMP/root/bin/fakecc" $CC_EXTRA "$TMP/t.c" -o "$TMP/out_argv"
fi

# No runtime next to a copied binary, cwd without ./runtime.
mkdir -p "$TMP/bare"
cp "$FAKECC" "$TMP/bare/fakecc"
(cd "$TMP/bare" && env -u FAKECC_RT ./fakecc "$TMP/t.c" -o "$TMP/out_bare") >/dev/null 2>&1 \
    && bad "missing runtime" || ok "missing runtime"

echo "$pass passed, $fail failed"
test "$fail" = 0
