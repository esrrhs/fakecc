#!/usr/bin/env bash
# Dynamic library tests for Mach-O: -dynamiclib, -install_name, LC_LOAD_DYLIB,
# LC_RPATH, and .dylib paths. This is the macOS equivalent of run_shlib_e2e.sh.
set -uo pipefail
# Portability helpers (timeout/nproc/otool shims for non-GNU hosts).
_COMPAT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
. "$_COMPAT_DIR/compat.sh" || { echo "cannot source compat.sh from $_COMPAT_DIR" >&2; exit 2; }
export -f timeout nproc 2>/dev/null || true


FAKECC=${1:-./build/fakecc}
shift || true
# Extra compiler flags (e.g. -O0) so the suite can run once per opt level.
CC_EXTRA="${CC_FLAGS:-} $*"
CC_TIMEOUT=${CC_TIMEOUT:-30}
RUN_TIMEOUT=${RUN_TIMEOUT:-10}
FAIL=0
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass() { echo "PASS $*"; }
fail() { echo "FAIL $*"; FAIL=1; }

# This suite is Mach-O only. Skip on Linux.
HOST_OS=$(uname -s | tr 'A-Z' 'a-z')
if [ "$HOST_OS" != "darwin" ]; then
    echo "SKIP dylib suite is Mach-O only (host $HOST_OS)"
    exit 0
fi

# Helper: check if a Mach-O binary has an LC_LOAD_DYLIB for a given dylib name
has_load_dylib() {
    local bin="$1"
    local name="$2"
    otool -L "$bin" 2>/dev/null | grep -q "$name"
}

# Helper: check if a Mach-O binary has an LC_RPATH for a given path
has_rpath() {
    local bin="$1"
    local path="$2"
    otool -l "$bin" 2>/dev/null | grep -A2 LC_RPATH | grep -q "$path"
}

# Build a tiny dynamic library with clang (oracle object code; fakecc only
# records LC_LOAD_DYLIB and resolves via dyld at run time).
cat > "$TMP/add.c" <<'EOF'
int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }
EOF
clang -dynamiclib -install_name @rpath/libadd.dylib -o "$TMP/libadd.dylib" "$TMP/add.c" \
    || { echo "FAIL could not build libadd.dylib with clang"; exit 1; }

cat > "$TMP/main_add.c" <<'EOF'
package main;
extern int add(int a, int b);
int main(void) { return add(20, 22); }
EOF

cat > "$TMP/main_printf.c" <<'EOF'
package main;
extern int printf(const char *fmt, ...);
int main(void) {
    int n = printf("hi\n");
    if (n != 3) return 1;
    return 0;
}
EOF

cat > "$TMP/main_fmt.c" <<'EOF'
package main;
import runtime;
int main(void) {
    int n = runtime.printf("hi\n");
    if (n != 3) return 1;
    return 0;
}
EOF

cat > "$TMP/main_missing.c" <<'EOF'
package main;
extern int definitely_missing_xyz(void);
int main(void) { return definitely_missing_xyz(); }
EOF

# 1) Default freestanding: printf via runtime/, no LC_LOAD_DYLIB for libSystem.
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_printf.c" -o "$TMP/p" 2>"$TMP/err" \
    || { fail "freestanding printf compile: $(head -1 "$TMP/err")"; }
if [ -x "$TMP/p" ]; then
    got=0; timeout "$RUN_TIMEOUT" "$TMP/p" >/dev/null || got=$?
    if [ "$got" = "0" ]; then pass "freestanding printf run"; else fail "freestanding printf run (exit $got)"; fi
    if has_load_dylib "$TMP/p" "libSystem"; then
        fail "freestanding should not load libSystem (has LC_LOAD_DYLIB libSystem)"
    else
        pass "freestanding no libSystem"
    fi
fi

# 2) Opt-in libc via -lc (cgo-style escape hatch).
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_printf.c" -lc -o "$TMP/p" 2>"$TMP/err" \
    || { fail "-lc compile: $(head -1 "$TMP/err")"; }
if [ -x "$TMP/p" ]; then
    got=0; timeout "$RUN_TIMEOUT" "$TMP/p" >/dev/null || got=$?
    if [ "$got" = "0" ]; then pass "-lc printf run"; else fail "-lc printf run (exit $got)"; fi
    if has_load_dylib "$TMP/p" "libSystem"; then
        pass "-lc LC_LOAD_DYLIB libSystem"
    else
        fail "-lc missing LC_LOAD_DYLIB libSystem"
    fi
fi

# 3) Link against a user .dylib by path.
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_add.c" "$TMP/libadd.dylib" -o "$TMP/p" 2>"$TMP/err" \
    || { fail "dylib path compile: $(head -1 "$TMP/err")"; }
if [ -x "$TMP/p" ]; then
    # Set DYLD_LIBRARY_PATH so dyld can find the dylib at runtime
    got=0; DYLD_LIBRARY_PATH="$TMP" timeout "$RUN_TIMEOUT" "$TMP/p" >/dev/null || got=$?
    if [ "$got" = "42" ]; then pass "dylib path run"; else fail "dylib path run (exit $got, expected 42)"; fi
    if has_load_dylib "$TMP/p" "libadd.dylib"; then
        pass "dylib path LC_LOAD_DYLIB"
    else
        fail "dylib path missing LC_LOAD_DYLIB libadd.dylib"
    fi
fi

# 4) Link with -L -l (search path).
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_add.c" -L"$TMP" -ladd -o "$TMP/p" 2>"$TMP/err" \
    || { fail "-L -l compile: $(head -1 "$TMP/err")"; }
if [ -x "$TMP/p" ]; then
    got=0; DYLD_LIBRARY_PATH="$TMP" timeout "$RUN_TIMEOUT" "$TMP/p" >/dev/null || got=$?
    if [ "$got" = "42" ]; then pass "-L -l run"; else fail "-L -l run (exit $got, expected 42)"; fi
    if has_load_dylib "$TMP/p" "libadd.dylib"; then
        pass "-L -l LC_LOAD_DYLIB"
    else
        fail "-L -l missing LC_LOAD_DYLIB libadd.dylib"
    fi
fi

# 5) Missing symbol should fail at link time.
rm -f "$TMP/p"
if timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_missing.c" -o "$TMP/p" 2>"$TMP/err"; then
    fail "missing symbol should not link"
else
    if grep -qiE "undefined|unresolved|missing" "$TMP/err"; then
        pass "missing symbol link error"
    else
        fail "missing symbol link error (wrong message: $(head -1 "$TMP/err"))"
    fi
fi

# 6) -rpath adds LC_RPATH.
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_add.c" "$TMP/libadd.dylib" -Wl,-rpath,/custom/path -o "$TMP/p" 2>"$TMP/err" \
    || { fail "-rpath compile: $(head -1 "$TMP/err")"; }
if [ -x "$TMP/p" ]; then
    if has_rpath "$TMP/p" "/custom/path"; then
        pass "-rpath LC_RPATH"
    else
        fail "-rpath missing LC_RPATH /custom/path"
    fi
fi

if [ "$FAIL" = "0" ]; then
    echo "dylib: all tests passed"
    exit 0
else
    echo "dylib: some tests failed"
    exit 1
fi
