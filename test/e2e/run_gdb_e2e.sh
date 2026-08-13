#!/usr/bin/env bash
# GDB end-to-end tests: drive a real gdb against fakecc output and check what
# it reports.  Anything else only proves the DWARF parses, not that it says
# something true.
#
# Cases live in cases/debug/*.c and are annotated:
#
#     // expect: 42                 program's own exit code
#     // gdb: break {brk}           a gdb command; {brk} is the source path
#                                   plus the line tagged `// BRK`, and {src}
#                                   is the source path alone
#     // gdb_expect: a = 40         extended regex the output must contain
#     // gdb_reject: optimized out  extended regex the output must NOT contain
#
# `gdb_expect_O0` / `gdb_expect_O1` apply at one optimization level only, for
# the places where optimized code genuinely cannot answer as precisely.
#
# Breakpoints are placed with a `// BRK` tag on the target statement instead of
# a hard-coded line number, so editing a case cannot silently move the
# breakpoint somewhere the variables are not live yet.
#
# Commands run in the order written, after `set pagination off`.  Every case is
# run at both -O0 and -O1: at -O0 variables sit in stack slots, at -O1 they
# move between registers and spill slots and are described by location lists,
# so the two exercise entirely different code in the compiler.
#
# Each case is additionally compiled with and without -g and the .text
# compared byte for byte.  -g must be purely additive; fakecc regressed here
# once by pinning scalars to memory under -g, which cost ~30% runtime.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FAKECC="${1:-$ROOT/build/fakecc}"
CASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/cases/debug"
CC_TIMEOUT=${CC_TIMEOUT:-30}
RUN_TIMEOUT=${RUN_TIMEOUT:-20}

if ! command -v gdb >/dev/null 2>&1; then
  echo "skip: gdb not installed"
  exit 0
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

FAIL=0
n_pass=0 n_fail=0

# Read repeated `// key: value` annotations into the named array.
read_annotations() {
  local src="$1" key="$2"
  sed -n "s|^// ${key}: \{0,1\}\(.*\)\$|\1|p" "$src"
}

# -g may add debug sections but must never change the code it describes.
check_text_identical() {
  local src="$1" flags="$2" name="$3"
  local plain="$TMP/id_plain" dbg="$TMP/id_dbg"
  if ! timeout "$CC_TIMEOUT" "$FAKECC" $flags "$src" -o "$plain" 2>"$TMP/e1"; then
    echo "FAIL $name [$flags] (compile without -g failed: $(head -1 "$TMP/e1"))"
    return 1
  fi
  if ! timeout "$CC_TIMEOUT" "$FAKECC" $flags -g "$src" -o "$dbg" 2>"$TMP/e2"; then
    echo "FAIL $name [$flags] (compile with -g failed: $(head -1 "$TMP/e2"))"
    return 1
  fi
  objcopy -O binary --only-section=.text "$plain" "$plain.text" 2>/dev/null
  objcopy -O binary --only-section=.text "$dbg" "$dbg.text" 2>/dev/null
  if ! cmp -s "$plain.text" "$dbg.text"; then
    echo "FAIL $name [$flags] (-g changed .text)"
    return 1
  fi
  return 0
}

run_case() {
  local src="$1" flags="$2"
  local name="$(basename "$src")"
  local label="${flags:-default}"
  local bin="$TMP/prog"

  rm -f "$bin"
  if ! timeout "$CC_TIMEOUT" "$FAKECC" $flags -g "$src" -o "$bin" 2>"$TMP/cc.err"; then
    echo "FAIL $name [$label] (compile failed: $(head -1 "$TMP/cc.err"))"
    n_fail=$((n_fail + 1)); FAIL=1; return
  fi

  # The program must still behave correctly, not just be describable.
  local expect got=0
  expect=$(read_annotations "$src" "expect" | head -1)
  timeout "$RUN_TIMEOUT" "$bin" >/dev/null 2>&1 || got=$?
  if [ -n "$expect" ] && [ "$got" != "$expect" ]; then
    echo "FAIL $name [$label] (exit: expected $expect, got $got)"
    n_fail=$((n_fail + 1)); FAIL=1; return
  fi

  # Resolve the `// BRK` tag to "<path>:<line>".  Cases that break by function
  # name instead do not need the tag.
  local brk_line brk=""
  brk_line=$(grep -n '// BRK' "$src" | head -1 | cut -d: -f1)
  if [ -n "$brk_line" ]; then
    brk="$src:$brk_line"
  elif grep -q '{brk}' "$src"; then
    echo "FAIL $name [$label] (uses {brk} but has no '// BRK' tag)"
    n_fail=$((n_fail + 1)); FAIL=1; return
  fi

  # Build the gdb command line from the case's annotations.
  local -a args=(-batch -ex "set pagination off" -ex "set confirm off")
  local cmd
  while IFS= read -r cmd; do
    [ -z "$cmd" ] && continue
    cmd="${cmd//\{brk\}/$brk}"
    args+=(-ex "${cmd//\{src\}/$src}")
  done < <(read_annotations "$src" "gdb")

  local out
  out=$(timeout "$RUN_TIMEOUT" gdb "${args[@]}" "$bin" 2>&1)

  # Level-specific expectations use the flag as a suffix: gdb_expect_O0 etc.
  local level_key="gdb_expect${flags//-/_}"

  local pat bad=0
  while IFS= read -r pat; do
    [ -z "$pat" ] && continue
    if ! echo "$out" | grep -Eq "$pat"; then
      echo "FAIL $name [$label] (gdb output missing: $pat)"
      bad=1
    fi
  done < <(read_annotations "$src" "gdb_expect"; read_annotations "$src" "$level_key")

  while IFS= read -r pat; do
    [ -z "$pat" ] && continue
    if echo "$out" | grep -Eq "$pat"; then
      echo "FAIL $name [$label] (gdb output should not contain: $pat)"
      bad=1
    fi
  done < <(read_annotations "$src" "gdb_reject")

  if [ "$bad" != "0" ]; then
    echo "--- gdb output ---"; echo "$out" | sed 's/^/  /'; echo "---"
    n_fail=$((n_fail + 1)); FAIL=1; return
  fi

  if ! check_text_identical "$src" "$flags" "$name"; then
    n_fail=$((n_fail + 1)); FAIL=1; return
  fi

  echo "PASS $name [$label]"
  n_pass=$((n_pass + 1))
}

for flags in "-O1" "-O0"; do
  echo "=== gdb e2e: $flags ==="
  while IFS= read -r src; do
    run_case "$src" "$flags"
  done < <(find "$CASE_DIR" -name '*.c' | sort)
done

# Symbol-level debugging must work even without -g: a stripped-of-DWARF but
# not stripped-of-symbols binary is what `objdump`/`break main` rely on.
cat >"$TMP/sym.c" <<'EOF'
package main;
int helper(int v) { return v + 1; }
int main(void) { return helper(41); }
EOF
"$FAKECC" "$TMP/sym.c" -o "$TMP/sym"
out=$(timeout "$RUN_TIMEOUT" gdb -batch -ex "set pagination off" \
  -ex "break main" -ex "run" -ex "continue" "$TMP/sym" 2>&1)
if echo "$out" | grep -q "Breakpoint 1"; then
  echo "PASS break main without -g"
  n_pass=$((n_pass + 1))
else
  echo "FAIL break main without -g"; echo "$out" | sed 's/^/  /'
  n_fail=$((n_fail + 1)); FAIL=1
fi

echo "--- gdb e2e: $n_pass passed, $n_fail failed ---"
exit $FAIL
