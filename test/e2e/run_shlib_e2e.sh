#!/usr/bin/env bash
# Shared-library link tests: -l / -l: / -nostdlib / -nodefaultlibs / .so path.
# Default link is freestanding (builtin runtime/, no DT_NEEDED).  -lc is opt-in.
set -uo pipefail

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

# Locale-independent helpers for dynamic tags.
has_needed() {
    local bin="$1" soname="$2"
    LANG=C readelf -d "$bin" 2>/dev/null | grep '(NEEDED)' | grep -F "[$soname]" >/dev/null
}
has_runpath() {
    local bin="$1" needle="$2"
    LANG=C readelf -d "$bin" 2>/dev/null | grep '(RUNPATH)' | grep -F "$needle" >/dev/null
}

# Build a tiny shared library with gcc (oracle object code; fakecc only records
# DT_NEEDED and resolves via the dynamic linker at run time).
cat > "$TMP/add.c" <<'EOF'
int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }
EOF
gcc -shared -fPIC -Wl,-soname,libadd.so -o "$TMP/libadd.so" "$TMP/add.c" \
    || { echo "FAIL could not build libadd.so with gcc"; exit 1; }

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

# 1) Opt-in libc via -nostdlib -lc (cgo-style escape hatch).
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_printf.c" -nostdlib -lc -o "$TMP/p" 2>"$TMP/err" \
    || { fail "nostdlib -lc compile: $(head -1 "$TMP/err")"; }
if [ -x "$TMP/p" ]; then
    got=0; timeout "$RUN_TIMEOUT" "$TMP/p" >/dev/null || got=$?
    if [ "$got" = "0" ]; then pass "nostdlib -lc printf"; else fail "nostdlib -lc run (exit $got)"; fi
    if has_needed "$TMP/p" "libc.so.6"; then
        pass "nostdlib -lc DT_NEEDED libc.so.6"
    else
        fail "nostdlib -lc missing DT_NEEDED libc.so.6"
    fi
fi

# 2) Default freestanding: printf via runtime/, no DT_NEEDED.
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_fmt.c" -o "$TMP/p" 2>"$TMP/err" \
    || { fail "default rt compile: $(head -1 "$TMP/err")"; }
if [ -x "$TMP/p" ]; then
    got=0; out=$(timeout "$RUN_TIMEOUT" "$TMP/p" 2>/dev/null) || got=$?
    if [ "$got" = "0" ] && [ "$out" = "hi" ]; then
        pass "default rt printf"
    else
        fail "default rt printf (exit $got out='$out')"
    fi
    if has_needed "$TMP/p" "libc.so.6"; then
        fail "default should not DT_NEEDED libc.so.6"
    else
        pass "default omits libc.so.6"
    fi
    if ! LANG=C readelf -l "$TMP/p" 2>/dev/null | grep -q 'INTERP'; then
        pass "default is static ELF"
    else
        fail "default should be static (has INTERP)"
    fi
fi

# 3) Custom shared library via -L + -ladd (DT_RUNPATH; no libc by default).
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_add.c" -L"$TMP" -ladd -o "$TMP/p" 2>"$TMP/err" \
    || { fail "-L -ladd compile: $(head -1 "$TMP/err")"; }
if [ -x "$TMP/p" ]; then
    if has_needed "$TMP/p" "libadd.so"; then
        pass "-L -ladd DT_NEEDED libadd.so"
    else
        fail "-L -ladd missing DT_NEEDED libadd.so"
    fi
    if has_needed "$TMP/p" "libc.so.6"; then
        fail "-L -ladd should not pull libc.so.6"
    else
        pass "-L -ladd omits libc.so.6"
    fi
    if has_runpath "$TMP/p" "$TMP"; then
        pass "-L -ladd DT_RUNPATH"
    else
        fail "-L -ladd missing DT_RUNPATH $TMP"
    fi
    got=0
    env -u LD_LIBRARY_PATH timeout "$RUN_TIMEOUT" "$TMP/p" >/dev/null || got=$?
    if [ "$got" = "42" ]; then pass "-L -ladd run via RUNPATH"; else fail "-L -ladd run (exit $got)"; fi
fi

# 3b) -L with a missing library must fail at link time.
rm -f "$TMP/p"
if timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_add.c" -L"$TMP/does-not-exist" -ladd -o "$TMP/p" 2>"$TMP/err"; then
    fail "-L missing dir should reject -ladd"
else
    pass "-L missing dir rejects -ladd"
fi

# 4) Exact soname form -l:libadd.so with -L, and passing the .so path.
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_add.c" -L"$TMP" -l:libadd.so -o "$TMP/p" 2>"$TMP/err" \
    || { fail "-l:libadd.so compile"; }
if [ -x "$TMP/p" ]; then
    got=0
    env -u LD_LIBRARY_PATH timeout "$RUN_TIMEOUT" "$TMP/p" >/dev/null || got=$?
    if [ "$got" = "42" ]; then pass "-L -l:libadd.so run"; else fail "-L -l:libadd.so run (exit $got)"; fi
fi

rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_add.c" "$TMP/libadd.so" -o "$TMP/p" 2>"$TMP/err" \
    || { fail ".so path compile"; }
if [ -x "$TMP/p" ]; then
    if has_needed "$TMP/p" "libadd.so"; then
        pass ".so path DT_NEEDED"
    else
        fail ".so path missing DT_NEEDED libadd.so"
    fi
    if has_runpath "$TMP/p" "$TMP"; then
        pass ".so path DT_RUNPATH from dirname"
    else
        fail ".so path missing DT_RUNPATH"
    fi
    got=0
    env -u LD_LIBRARY_PATH timeout "$RUN_TIMEOUT" "$TMP/p" >/dev/null || got=$?
    if [ "$got" = "42" ]; then pass ".so path run via RUNPATH"; else fail ".so path run (exit $got)"; fi
fi

# 5) -nodefaultlibs + -L -ladd: still no libc (alias / compatibility).
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_add.c" -nodefaultlibs -L"$TMP" -ladd -o "$TMP/p" 2>"$TMP/err" \
    || { fail "-nodefaultlibs -L -ladd compile"; }
if [ -x "$TMP/p" ]; then
    if has_needed "$TMP/p" "libc.so.6"; then
        fail "-nodefaultlibs still has libc.so.6"
    else
        pass "-nodefaultlibs omits libc.so.6"
    fi
    if has_needed "$TMP/p" "libadd.so"; then
        pass "-nodefaultlibs keeps libadd.so"
    else
        fail "-nodefaultlibs -ladd missing libadd.so"
    fi
    got=0
    env -u LD_LIBRARY_PATH timeout "$RUN_TIMEOUT" "$TMP/p" >/dev/null || got=$?
    if [ "$got" = "42" ]; then pass "-nodefaultlibs -L -ladd run"; else fail "-nodefaultlibs -L -ladd run (exit $got)"; fi
fi

# 6) -nostdlib with an unresolved symbol: runtime must fail (no rt, no libc).
rm -f "$TMP/p"
timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$TMP/main_missing.c" -nostdlib -o "$TMP/p" 2>"$TMP/err"
cc_rc=$?
if [ "$cc_rc" != "0" ]; then
    pass "-nostdlib missing-sym rejected at link"
elif [ -x "$TMP/p" ]; then
    if has_needed "$TMP/p" "libc.so.6"; then
        fail "-nostdlib missing-sym still linked libc"
    else
        pass "-nostdlib missing-sym omits libc"
    fi
    got=0
    timeout "$RUN_TIMEOUT" "$TMP/p" >/dev/null 2>"$TMP/run.err" || got=$?
    if [ "$got" = "0" ]; then
        fail "-nostdlib missing-sym unexpectedly ran OK"
    else
        pass "-nostdlib missing-sym fails at runtime (exit $got)"
    fi
else
    fail "-nostdlib missing-sym: no binary and no error"
fi

exit $FAIL
