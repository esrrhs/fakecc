#!/usr/bin/env bash
# LLDB end-to-end tests: drive a real lldb against fakecc output and check what
# it reports. This is the macOS equivalent of run_gdb_e2e.sh.
#
# Cases live in cases/debug/*.c and are annotated:
#
#     // expect: 42                 program's own exit code
#     // lldb: breakpoint set -f {src} -l {line}   an lldb command
#     // lldb: run                   run the program
#     // lldb: frame variable s      print variable s
#     // lldb_expect: \(int\) s = 42 extended regex the output must contain
#     // lldb_reject: optimized out  extended regex the output must NOT contain
#
# For compatibility, we also support the gdb: annotations and translate them
# to lldb commands automatically.
set -uo pipefail
_COMPAT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
. "$_COMPAT_DIR/compat.sh" || { echo "cannot source compat.sh from $_COMPAT_DIR" >&2; exit 2; }
export -f timeout nproc 2>/dev/null || true


ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FAKECC="${1:-$ROOT/build/fakecc}"
shift || true
CC_EXTRA="${CC_FLAGS:-}"
CASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/cases/debug"
CC_TIMEOUT=${CC_TIMEOUT:-30}
RUN_TIMEOUT=${RUN_TIMEOUT:-20}

if ! command -v lldb >/dev/null 2>&1; then
  echo "skip: lldb not installed"
  exit 0
fi

# Skip on Linux - we use gdb there
HOST_OS=$(uname -s | tr 'A-Z' 'a-z')
if [ "$HOST_OS" = "linux" ]; then
  echo "skip: lldb suite is macOS-only (host $HOST_OS)"
  exit 0
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

FAIL=0
n_pass=0 n_fail=0

read_annotations() {
  local src="$1" key="$2"
  sed -n "s|^// ${key}: \{0,1\}\(.*\)\$|\1|p" "$src"
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

  local expect got=0
  expect=$(read_annotations "$src" "expect" | head -1)
  if [ -n "$expect" ]; then
    if ! timeout "$RUN_TIMEOUT" "$bin" >"$TMP/run.out" 2>"$TMP/run.err"; then
      got=$?
    fi
    if [ "$got" != "$expect" ]; then
      echo "FAIL $name [$label] (exit $got, expected $expect)"
      n_fail=$((n_fail + 1)); FAIL=1; return
    fi
  fi

  # Build lldb command script
  local lldb_script="$TMP/lldb.cmd"
  > "$lldb_script"
  echo "settings set auto-confirm true" >> "$lldb_script"

  # Translate gdb: annotations to lldb commands, or use lldb: annotations directly
  local has_lldb_cmds=0
  while IFS= read -r cmd; do
    has_lldb_cmds=1
    # Translate GDB commands to LLDB
    case "$cmd" in
      "break {brk}")
        local brk_line
        brk_line=$(grep -n '// BRK' "$src" | head -1 | cut -d: -f1)
        echo "breakpoint set -f $src -l $brk_line" >> "$lldb_script"
        ;;
      "run")
        echo "run" >> "$lldb_script"
        ;;
      "print "*)
        local var="${cmd#print }"
        echo "frame variable $var" >> "$lldb_script"
        ;;
      "continue")
        echo "continue" >> "$lldb_script"
        ;;
      "backtrace"|"bt")
        echo "thread backtrace" >> "$lldb_script"
        ;;
      *)
        echo "$cmd" >> "$lldb_script"
        ;;
    esac
  done < <(read_annotations "$src" "gdb")

  # Also process lldb: annotations
  while IFS= read -r cmd; do
    has_lldb_cmds=1
    echo "$cmd" >> "$lldb_script"
  done < <(read_annotations "$src" "lldb")

  if [ "$has_lldb_cmds" = "0" ]; then
    echo "PASS $name [$label] (no debugger commands)"
    n_pass=$((n_pass + 1)); return
  fi

  echo "quit" >> "$lldb_script"

  # Run lldb with the script
  if ! timeout "$RUN_TIMEOUT" lldb -b -s "$lldb_script" "$bin" >"$TMP/lldb.out" 2>"$TMP/lldb.err"; then
    echo "FAIL $name [$label] (lldb failed to run)"
    n_fail=$((n_fail + 1)); FAIL=1; return
  fi

  # Check lldb_expect annotations
  local expect_ok=1
  while IFS= read -r pattern; do
    if ! grep -qE "$pattern" "$TMP/lldb.out"; then
      echo "FAIL $name [$label] (expected pattern not found: $pattern)"
      echo "--- lldb output ---"
      cat "$TMP/lldb.out"
      echo "--- end ---"
      expect_ok=0
      break
    fi
  done < <(read_annotations "$src" "gdb_expect"; read_annotations "$src" "lldb_expect")

  # Check lldb_reject annotations
  while IFS= read -r pattern; do
    if grep -qE "$pattern" "$TMP/lldb.out"; then
      echo "FAIL $name [$label] (rejected pattern found: $pattern)"
      echo "--- lldb output ---"
      cat "$TMP/lldb.out"
      echo "--- end ---"
      expect_ok=0
      break
    fi
  done < <(read_annotations "$src" "gdb_reject"; read_annotations "$src" "lldb_reject")

  if [ "$expect_ok" = "1" ]; then
    echo "PASS $name [$label]"
    n_pass=$((n_pass + 1))
  else
    n_fail=$((n_fail + 1)); FAIL=1
  fi
}

if [ ! -d "$CASE_DIR" ] || [ -z "$(ls -A "$CASE_DIR"/*.c 2>/dev/null)" ]; then
  echo "skip: no debug test cases found"
  exit 0
fi

echo "=== lldb debug tests ==="
for src in "$CASE_DIR"/*.c; do
  run_case "$src" ""
  run_case "$src" "-O0"
  run_case "$src" "-O1"
done

echo "lldb: $n_pass passed, $n_fail failed"
exit $FAIL
